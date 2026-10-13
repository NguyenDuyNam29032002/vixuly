/**
  ******************************************************************************
  * @file    led_effects.h
  * @brief   Dieu khien 8 LED (PB8..PB15) va LED trang thai (PC13)
  *
  *  Phan cung:
  *    - 8 LED: PB8 (LED0) ... PB15 (LED7), moi LED noi tiep R 220..330 Ohm
  *      xuong GND (muc 1 = sang). Neu mac kieu nguoc (anode len 3V3) thi
  *      dat LED_ACTIVE_LOW = 1.
  *    - LED trang thai: PC13 (LED co san tren board Blue Pill, muc 0 = sang).
  ******************************************************************************
  */
#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define LED_ACTIVE_LOW        0u   /* 0: muc 1 = sang ; 1: muc 0 = sang     */
#define STATUS_LED_ACTIVE_LOW 1u   /* PC13 tren Blue Pill sang khi muc 0     */

typedef enum
{
  FX_BLINK_ALL = 0,   /* Nhay don dong loat                 */
  FX_CHASE,           /* Chay duoi 1 LED                     */
  FX_STACK,           /* Sang don                            */
  FX_FILL,            /* Sang dan roi tat dan                */
  FX_KNIGHT_RIDER,    /* Chay qua lai (ping-pong)            */
  FX_ALTERNATE,       /* Xen ke chan/le                      */
  FX_CENTER_BOUNCE,   /* Tu giua toa ra 2 ben roi thu vao    */
  FX_COUNT
} LedEffect_t;

/* Khoi tao GPIO + trang thai mac dinh (he thong BAT). */
void LedFx_Init(void);

/* Goi lien tuc trong vong lap chinh (khong chan). */
void LedFx_Task(uint32_t now_ms);

/* Nhan nhap: chuyen sang hieu ung ke tiep (vong tron). */
void LedFx_NextEffect(void);

/* Nhan giu vua: doi toc do theo kieu "con lac":
 * nhanh dan den muc nhanh nhat roi cham dan den muc cham nhat, lap lai. */
void LedFx_ChangeSpeed(void);

/* Nhan giu lau: bat/tat ca he thong. Khi BAT lai se reset ve mac dinh. */
void LedFx_SetEnabled(bool enable);
bool LedFx_IsEnabled(void);

/* Thong tin de debug (Live Expressions) */
LedEffect_t LedFx_GetEffect(void);
uint32_t    LedFx_GetPeriodMs(void);
const char *LedFx_GetEffectName(LedEffect_t fx);

/* Truy cap phan cung muc thap */
void LED_Write(uint8_t pattern);   /* bit0 -> PB8 ... bit7 -> PB15 */
void StatusLed_Set(bool on);

#ifdef __cplusplus
}
#endif

#endif /* LED_EFFECTS_H */
