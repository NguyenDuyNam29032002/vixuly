/**
  ******************************************************************************
  * @file    button_ic.c
  * @brief   Do thoi gian nhan nut bang TIM2 Input Capture (CH1 - PA0)
  *
  *  Viet o muc thanh ghi (CMSIS) de khong phu thuoc file stm32f1xx_hal_tim.c
  *  (project hien chua sinh driver HAL TIM). Van dung HAL cho clock/NVIC.
  *
  *  LUU Y: KHONG bat TIM2 trong file .ioc (CubeMX) vi ham TIM2_IRQHandler()
  *         da duoc dinh nghia trong file nay.
  ******************************************************************************
  */
#include "button_ic.h"
#include "stm32f1xx_hal.h"

/* Cau hinh noi bo -----------------------------------------------------------*/
#define BTN_TICK_HZ          10000u                     /* 1 tick = 0.1 ms     */
#define BTN_TICKS_PER_MS     (BTN_TICK_HZ / 1000u)
#define BTN_EVQ_SIZE         8u                         /* Hang doi su kien    */
#define BTN_IRQ_PRIORITY     1u

#define BTN_PIN_IS_LOW()     ((GPIOA->IDR & GPIO_IDR_IDR0) == 0u)

/* Bien trang thai (dung chung giua ngat va vong lap chinh) ------------------*/
static volatile uint32_t s_ovf_count;      /* So lan TIM2 tran (16 bit cao)   */
static volatile uint8_t  s_pressed;        /* 1: dang nhan                     */
static volatile uint32_t s_press_ts;       /* Moc thoi gian luc nhan (tick)   */

static volatile uint32_t s_evq[BTN_EVQ_SIZE];   /* Thoi gian nhan (ms)        */
static volatile uint8_t  s_evq_head;            /* Ghi boi ngat               */
static volatile uint8_t  s_evq_tail;            /* Doc boi vong lap chinh     */

/* Ham noi bo ----------------------------------------------------------------*/

/* Tan so clock cap cho TIM2 (APB1). Neu APB1 prescaler != 1 thi x2. */
static uint32_t tim2_clock_hz(void)
{
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1)
  {
    return pclk1;
  }
  return pclk1 * 2u;
}

/**
  * Ghep so lan tran + gia tri CNT 16 bit thanh moc thoi gian 32 bit.
  * Xu ly truong hop tran vua xay ra nhung ngat Update chua duoc phuc vu:
  * neu co UIF va gia tri dem nho (< nua chu ky) -> lan tran do xay ra
  * TRUOC thoi diem chot -> phai cong them 1.
  */
static uint32_t make_timestamp(uint16_t cnt)
{
  uint32_t ovf = s_ovf_count;
  if (((TIM2->SR & TIM_SR_UIF) != 0u) && (cnt < 0x8000u))
  {
    ovf++;
  }
  return (ovf << 16) | cnt;
}

/* Chon canh bat tiep theo: true = canh xuong (cho nhan), false = canh len. */
static void arm_edge(bool wait_falling)
{
  TIM2->CCER &= ~TIM_CCER_CC1E;
  if (wait_falling)
  {
    TIM2->CCER |= TIM_CCER_CC1P;
  }
  else
  {
    TIM2->CCER &= ~TIM_CCER_CC1P;
  }
  TIM2->CCER |= TIM_CCER_CC1E;
}

static void evq_push(uint32_t ms)
{
  uint8_t next = (uint8_t)((s_evq_head + 1u) % BTN_EVQ_SIZE);
  if (next != s_evq_tail)            /* Day thi bo su kien moi */
  {
    s_evq[s_evq_head] = ms;
    s_evq_head = next;
  }
}

/* May trang thai: cap nhat theo muc logic hien tai cua chan nut. */
static void process_level(bool is_low, uint32_t ts)
{
  if (is_low && (s_pressed == 0u))
  {
    /* Bat dau nhan */
    s_pressed  = 1u;
    s_press_ts = ts;
  }
  else if ((!is_low) && (s_pressed != 0u))
  {
    /* Nha nut -> tinh thoi gian nhan */
    uint32_t ms;
    s_pressed = 0u;
    ms = (ts - s_press_ts) / BTN_TICKS_PER_MS;   /* Tru khong dau: an toan khi tran 32 bit */
    if (ms >= BTN_DEBOUNCE_MS)
    {
      evq_push(ms);
    }
  }
  else
  {
    /* Khong doi trang thai (rung phim / canh gia) */
  }
}

/* API -----------------------------------------------------------------------*/

void Button_Init(void)
{
  /* 1. Clock cho GPIOA va TIM2 */
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;                         /* Doi clock on dinh */

  /* 2. PA0: Input pull-up  (CNF0 = 10, MODE0 = 00, ODR0 = 1) */
  GPIOA->CRL  = (GPIOA->CRL & ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0)) | GPIO_CRL_CNF0_1;
  GPIOA->BSRR = GPIO_BSRR_BS0;

  /* 3. Reset TIM2 ve trang thai mac dinh */
  RCC->APB1RSTR |=  RCC_APB1RSTR_TIM2RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;

  /* 4. Time base: 10 kHz, dem len, ARR = 0xFFFF
   *    CKD = /4 -> f_DTS cham hon de bo loc so loc tot hon
   *    URS = 1  -> chi tran moi sinh UIF (UG khong sinh)                    */
  TIM2->CR1 = TIM_CR1_CKD_1 | TIM_CR1_URS;
  TIM2->PSC = (uint16_t)((tim2_clock_hz() / BTN_TICK_HZ) - 1u);
  TIM2->ARR = 0xFFFFu;

  /* 5. Kenh 1 - Input Capture:
   *    CC1S   = 01   : IC1 noi voi TI1 (PA0)
   *    IC1PSC = 00   : bat moi canh
   *    IC1F   = 1111 : bo loc so manh nhat (chong nhieu)                    */
  TIM2->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_IC1F;
  TIM2->CCER  = TIM_CCER_CC1P;                /* Bat dau: cho canh xuong   */

  /* 6. Nap PSC, xoa co, reset bien */
  TIM2->EGR = TIM_EGR_UG;
  TIM2->SR  = 0u;
  s_ovf_count = 0u;
  s_pressed   = 0u;
  s_press_ts  = 0u;
  s_evq_head  = 0u;
  s_evq_tail  = 0u;

  /* 7. Cho phep capture + ngat CC1 & Update */
  TIM2->CCER |= TIM_CCER_CC1E;
  TIM2->DIER  = TIM_DIER_CC1IE | TIM_DIER_UIE;

  HAL_NVIC_SetPriority(TIM2_IRQn, BTN_IRQ_PRIORITY, 0u);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  /* 8. Chay timer */
  TIM2->CR1 |= TIM_CR1_CEN;
}

ButtonEvent_t Button_GetEvent(uint32_t *duration_ms)
{
  uint32_t ms;

  if (s_evq_tail == s_evq_head)
  {
    return BTN_EVT_NONE;
  }

  ms = s_evq[s_evq_tail];
  s_evq_tail = (uint8_t)((s_evq_tail + 1u) % BTN_EVQ_SIZE);

  if (duration_ms != NULL)
  {
    *duration_ms = ms;
  }

  if (ms < BTN_SHORT_MAX_MS)
  {
    return BTN_EVT_SHORT;
  }
  if (ms <= BTN_MEDIUM_MAX_MS)
  {
    return BTN_EVT_MEDIUM;
  }
  return BTN_EVT_LONG;
}

uint32_t Button_GetHoldTimeMs(void)
{
  uint32_t ms = 0u;

  NVIC_DisableIRQ(TIM2_IRQn);                 /* Doc nguyen tu */
  if (s_pressed != 0u)
  {
    uint16_t cnt = (uint16_t)TIM2->CNT;
    ms = (make_timestamp(cnt) - s_press_ts) / BTN_TICKS_PER_MS;
  }
  NVIC_EnableIRQ(TIM2_IRQn);

  return ms;
}

/* Trinh phuc vu ngat --------------------------------------------------------*/

void TIM2_IRQHandler(void)
{
  /* ---- Input Capture CH1: co canh tren PA0 ---- */
  if ((TIM2->SR & TIM_SR_CC1IF) != 0u)
  {
    uint16_t cap = (uint16_t)TIM2->CCR1;      /* Doc CCR1 tu dong xoa CC1IF */
    uint32_t ts  = make_timestamp(cap);
    uint8_t  i;

    TIM2->SR = ~TIM_SR_CC1OF;                 /* Xoa co overcapture (neu co) */

    /* Xac dinh nhan/nha theo muc thuc te cua chan roi dat canh tiep theo.
     * Lap lai neu chan da doi muc trong luc doi cuc tinh (rung phim rat
     * nhanh) de khong bao gio bi "lech pha" giua phan mem va phan cung.  */
    for (i = 0u; i < 4u; i++)
    {
      bool low = BTN_PIN_IS_LOW();
      process_level(low, ts);
      arm_edge(!low);                         /* Dang nha -> cho canh xuong */
      if (BTN_PIN_IS_LOW() == low)
      {
        break;                                /* On dinh */
      }
      ts = make_timestamp((uint16_t)TIM2->CNT);
    }
  }

  /* ---- Update: timer tran ---- */
  if ((TIM2->SR & TIM_SR_UIF) != 0u)
  {
    TIM2->SR = ~TIM_SR_UIF;                   /* rc_w0: ghi 0 de xoa       */
    s_ovf_count++;
  }
}
