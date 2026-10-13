/**
  ******************************************************************************
  * @file    led_effects.c
  * @brief   Cac hieu ung 8 LED, toc do, bat/tat he thong
  *
  *  Moi hieu ung la 1 ham sinh khung hinh: frame = fn(step), voi
  *  step = 0 .. frames-1. LedFx_Task() tang step theo chu ky toc do hien tai
  *  (khong dung HAL_Delay -> khong chan CPU, nut nhan luon dap ung ngay).
  ******************************************************************************
  */
#include "led_effects.h"
#include "stm32f1xx_hal.h"

/* Bang toc do (ms / khung hinh) ---------------------------------------------*/
static const uint16_t k_speed_ms[] = { 800u, 500u, 300u, 150u, 70u };
#define SPEED_LEVELS        ((uint8_t)(sizeof(k_speed_ms) / sizeof(k_speed_ms[0])))
#define SPEED_DEFAULT_IDX   2u
#define FX_DEFAULT          FX_BLINK_ALL

/* Ham sinh khung hinh -------------------------------------------------------*/
typedef uint8_t (*FrameFn_t)(uint16_t step);

typedef struct
{
  const char *name;
  uint16_t    frames;
  FrameFn_t   fn;
} Effect_t;

/* Nhay don dong loat: sang het / tat het */
static uint8_t fx_blink_all(uint16_t s)
{
  return ((s & 1u) == 0u) ? 0xFFu : 0x00u;
}

/* Chay duoi: 1 LED chay tu LED0 -> LED7 */
static uint8_t fx_chase(uint16_t s)
{
  return (uint8_t)(1u << s);
}

/* Sang don: 1 diem sang chay tu LED0 len, cham dinh thi "xep chong" lai,
 * lan sau chay ngan hon 1 o ... cho den khi sang het.
 * Tong so khung: 8+7+...+1 = 36, them 1 khung giu sang het = 37.            */
static uint8_t fx_stack(uint16_t s)
{
  uint8_t k = 0u;                 /* So LED da xep o dinh */
  uint8_t stack;

  if (s >= 36u)
  {
    return 0xFFu;
  }
  while (s >= (uint16_t)(8u - k))
  {
    s = (uint16_t)(s - (8u - k));
    k++;
  }
  stack = (uint8_t)(0xFF00u >> k);              /* k bit cao nhat */
  return (uint8_t)(stack | (uint8_t)(1u << s));
}

/* Sang dan len 0x01..0xFF roi tat dan 0xFE..0x00 (16 khung) */
static uint8_t fx_fill(uint16_t s)
{
  if (s < 8u)
  {
    return (uint8_t)((2u << s) - 1u);
  }
  return (uint8_t)(0xFFu << (s - 7u));
}

/* Knight Rider: 0,1,..,7,6,..,1 (14 khung) */
static uint8_t fx_knight_rider(uint16_t s)
{
  uint16_t pos = (s < 8u) ? s : (uint16_t)(14u - s);
  return (uint8_t)(1u << pos);
}

/* Xen ke chan / le */
static uint8_t fx_alternate(uint16_t s)
{
  return ((s & 1u) == 0u) ? 0xAAu : 0x55u;
}

/* Tu giua toa ra hai ben roi thu vao */
static const uint8_t k_center_frames[] = { 0x18u, 0x24u, 0x42u, 0x81u, 0x42u, 0x24u };
static uint8_t fx_center_bounce(uint16_t s)
{
  return k_center_frames[s];
}

static const Effect_t k_effects[FX_COUNT] =
{
  [FX_BLINK_ALL]     = { "Nhay dong loat",  2u,  fx_blink_all     },
  [FX_CHASE]         = { "Chay duoi",       8u,  fx_chase         },
  [FX_STACK]         = { "Sang don",        37u, fx_stack         },
  [FX_FILL]          = { "Sang/tat dan",    16u, fx_fill          },
  [FX_KNIGHT_RIDER]  = { "Knight Rider",    14u, fx_knight_rider  },
  [FX_ALTERNATE]     = { "Xen ke",          2u,  fx_alternate     },
  [FX_CENTER_BOUNCE] = { "Giua ra ngoai",   (uint16_t)sizeof(k_center_frames), fx_center_bounce },
};

/* Trang thai ----------------------------------------------------------------*/
static bool        s_enabled;
static LedEffect_t s_effect;
static uint16_t    s_step;
static uint8_t     s_speed_idx;
static int8_t      s_speed_dir;      /* +1: dang tang toc, -1: dang giam toc */
static uint32_t    s_last_ms;
static bool        s_redraw;         /* Ve ngay khung dau khi vua doi hieu ung */

/* Phan cung -----------------------------------------------------------------*/

static void led_gpio_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  (void)RCC->APB2ENR;

  /* PB8..PB15: Output push-pull 2 MHz (MODE = 10, CNF = 00) -> 0x2 moi chan */
  GPIOB->CRH = 0x22222222u;

  /* PC13: Output push-pull 2 MHz */
  GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13)) | GPIO_CRH_MODE13_1;
}

void LED_Write(uint8_t pattern)
{
#if (LED_ACTIVE_LOW != 0u)
  pattern = (uint8_t)~pattern;
#endif
  /* BSRR: bit 8..15 = set PB8..PB15, bit 24..31 = reset PB8..PB15
   * -> cap nhat ca 8 LED trong 1 lenh ghi, khong bi nhap nhay trung gian. */
  GPIOB->BSRR = ((uint32_t)(uint8_t)~pattern << 24) | ((uint32_t)pattern << 8);
}

void StatusLed_Set(bool on)
{
#if (STATUS_LED_ACTIVE_LOW != 0u)
  on = !on;
#endif
  GPIOC->BSRR = on ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13;
}

/* API -----------------------------------------------------------------------*/

static void load_defaults(void)
{
  s_effect    = FX_DEFAULT;
  s_step      = 0u;
  s_speed_idx = SPEED_DEFAULT_IDX;
  s_speed_dir = +1;
  s_redraw    = true;
}

void LedFx_Init(void)
{
  led_gpio_init();
  LED_Write(0x00u);
  StatusLed_Set(false);
  load_defaults();
  s_enabled = true;
  s_last_ms = HAL_GetTick();
}

void LedFx_Task(uint32_t now_ms)
{
  const Effect_t *fx;

  if (!s_enabled)
  {
    return;
  }

  if (s_redraw || ((now_ms - s_last_ms) >= k_speed_ms[s_speed_idx]))
  {
    fx = &k_effects[s_effect];
    s_last_ms = now_ms;
    s_redraw  = false;

    if (s_step >= fx->frames)
    {
      s_step = 0u;
    }
    LED_Write(fx->fn(s_step));
    s_step++;
  }
}

void LedFx_NextEffect(void)
{
  s_effect = (LedEffect_t)(((uint8_t)s_effect + 1u) % (uint8_t)FX_COUNT);
  s_step   = 0u;
  s_redraw = true;
}

void LedFx_ChangeSpeed(void)
{
  if (s_speed_dir > 0)
  {
    if ((uint8_t)(s_speed_idx + 1u) >= SPEED_LEVELS)
    {
      s_speed_dir = -1;            /* Da nhanh nhat -> quay dau, cham lai */
      s_speed_idx--;
    }
    else
    {
      s_speed_idx++;
    }
  }
  else
  {
    if (s_speed_idx == 0u)
    {
      s_speed_dir = +1;            /* Da cham nhat -> quay dau, nhanh len */
      s_speed_idx++;
    }
    else
    {
      s_speed_idx--;
    }
  }
}

void LedFx_SetEnabled(bool enable)
{
  if (enable == s_enabled)
  {
    return;
  }

  s_enabled = enable;
  if (enable)
  {
    load_defaults();               /* Reset ve trang thai ban dau */
    s_last_ms = HAL_GetTick();
  }
  else
  {
    LED_Write(0x00u);              /* Standby: tat het LED */
  }
}

bool LedFx_IsEnabled(void)
{
  return s_enabled;
}

LedEffect_t LedFx_GetEffect(void)
{
  return s_effect;
}

uint32_t LedFx_GetPeriodMs(void)
{
  return k_speed_ms[s_speed_idx];
}

const char *LedFx_GetEffectName(LedEffect_t fx)
{
  return ((uint8_t)fx < (uint8_t)FX_COUNT) ? k_effects[fx].name : "?";
}
