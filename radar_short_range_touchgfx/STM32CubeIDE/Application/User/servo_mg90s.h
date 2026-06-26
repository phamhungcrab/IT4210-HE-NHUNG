#ifndef SERVO_MG90S_H
#define SERVO_MG90S_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Servo_Init(void);

void Servo_SetPulseUs(uint16_t pulse_us);

/*
 * Với servo 360:
 * Servo_SetAngle(90) = stop.
 * Hàm này giữ lại để radar_app cũ không lỗi compile.
 */
void Servo_SetAngle(uint16_t angle_deg);

/*
 * direction:
 *   >0: quay thuận
 *   <0: quay nghịch
 *    0: dừng
 *
 * speed_mode:
 *   RADAR_SPEED_SLOW / MED / FAST
 */
void Servo_SetContinuous(int8_t direction, uint8_t speed_mode);

void Servo_Stop(void);

uint16_t Servo_GetLastAngle(void);
uint16_t Servo_GetLastPulseUs(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_MG90S_H */
