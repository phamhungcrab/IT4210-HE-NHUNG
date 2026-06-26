#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HCSR04_STATE_IDLE = 0,
    HCSR04_STATE_WAIT_RISING,
    HCSR04_STATE_WAIT_FALLING,
    HCSR04_STATE_DONE,
    HCSR04_STATE_TIMEOUT,
    HCSR04_STATE_ERROR
} HCSR04_State_t;

void HCSR04_Init(void);
void HCSR04_StartMeasure(void);

uint8_t HCSR04_IsBusy(void);
uint8_t HCSR04_IsDone(void);
uint8_t HCSR04_IsTimeout(void);

uint8_t HCSR04_GetDistanceCm(uint16_t *distance_cm);
uint32_t HCSR04_GetEchoUs(void);

void HCSR04_ProcessTimeout(void);

/*
 * Gọi hàm này trong HAL_TIM_IC_CaptureCallback().
 */
void HCSR04_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

/*
 * Debug counters for Hercules.
 */
HCSR04_State_t HCSR04_GetState(void);
uint32_t HCSR04_GetStartCount(void);
uint32_t HCSR04_GetRisingCount(void);
uint32_t HCSR04_GetFallingCount(void);
uint32_t HCSR04_GetTimeoutCount(void);
uint32_t HCSR04_GetLastEchoUs(void);

#ifdef __cplusplus
}
#endif

#endif /* HCSR04_H */
