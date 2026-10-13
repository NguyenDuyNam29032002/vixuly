/**
  ******************************************************************************
  * @file    button_ic.h
  * @brief   Do thoi gian nhan nut bang TIM2 Input Capture (CH1 - PA0)
  *
  *  Phan cung:
  *    - Nut nhan noi giua PA0 va GND (dung dien tro keo len noi cua chip).
  *    - Nha nut  : PA0 = 1
  *    - Nhan nut : PA0 = 0  -> canh xuong = bat dau nhan, canh len = nha ra.
  *
  *  Nguyen ly:
  *    - TIM2 dem voi tan so 10 kHz (1 tick = 0.1 ms), ARR = 0xFFFF.
  *    - Ngat tran (Update) dem so lan tran -> moc thoi gian 32 bit,
  *      do duoc thoi gian nhan dai tuy y (khong bi gioi han 6.5 s).
  *    - Input Capture CH1 chot gia tri CNT tai thoi diem co canh,
  *      sau moi lan bat duoc canh se dao cuc tinh (xuong <-> len).
  *    - Thoi gian nhan = t(canh len) - t(canh xuong).
  ******************************************************************************
  */
#ifndef BUTTON_IC_H
#define BUTTON_IC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Nguong phan loai (don vi: ms) ---------------------------------------------*/
#define BTN_DEBOUNCE_MS      50u    /* Xung ngan hon -> coi la rung phim, bo qua */
#define BTN_SHORT_MAX_MS     1000u  /* < 1 s            : nhan nhap            */
#define BTN_MEDIUM_MAX_MS    3000u  /* 1 s .. 3 s       : nhan giu vua         */
                                    /* > 3 s            : nhan giu lau         */

typedef enum
{
  BTN_EVT_NONE = 0,   /* Khong co su kien moi                 */
  BTN_EVT_SHORT,      /* Nhan nhap  (< 1 s)                    */
  BTN_EVT_MEDIUM,     /* Nhan giu vua (1 s .. 3 s)             */
  BTN_EVT_LONG        /* Nhan giu lau (> 3 s)                  */
} ButtonEvent_t;

/**
  * @brief  Cau hinh PA0 + TIM2 CH1 Input Capture + ngat NVIC.
  *         Goi sau SystemClock_Config().
  */
void Button_Init(void);

/**
  * @brief  Lay 1 su kien nhan nut tu hang doi (khong chan).
  * @param  duration_ms  [out] thoi gian nhan thuc te (ms), co the NULL.
  * @retval Loai su kien, BTN_EVT_NONE neu hang doi rong.
  */
ButtonEvent_t Button_GetEvent(uint32_t *duration_ms);

/**
  * @brief  Thoi gian nut DANG duoc giu (ms). Tra ve 0 neu nut dang nha.
  *         Dung de bao hieu truc quan cho nguoi dung khi dang giu nut.
  */
uint32_t Button_GetHoldTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_IC_H */
