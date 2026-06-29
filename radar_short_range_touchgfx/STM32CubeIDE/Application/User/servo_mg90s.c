#include "servo_mg90s.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

#include "radar_config.h"

static uint16_t g_last_angle_deg = SERVO_CENTER_ANGLE_DEG;
static uint16_t g_last_pulse_us = SERVO_CENTER_PULSE_US;

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    Servo_SetAngle(SERVO_CENTER_ANGLE_DEG);
}

void Servo_SetPulseUs(uint16_t pulse_us)
{
    if (pulse_us < SERVO_MIN_PULSE_US)
    {
        pulse_us = SERVO_MIN_PULSE_US;
    }

    if (pulse_us > SERVO_MAX_PULSE_US)
    {
        pulse_us = SERVO_MAX_PULSE_US;
    }

    g_last_pulse_us = pulse_us;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_us);
}

void Servo_SetAngle(uint16_t angle_deg)
{
    uint16_t pulse_us;

    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        angle_deg = SERVO_MAX_ANGLE_DEG;
    }

    pulse_us = (uint16_t)(
        SERVO_MIN_PULSE_US +
        (((uint32_t)angle_deg * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / SERVO_MAX_ANGLE_DEG)
    );

    g_last_angle_deg = angle_deg;
    Servo_SetPulseUs(pulse_us);
}

void Servo_Stop(void)
{
    Servo_SetAngle(SERVO_CENTER_ANGLE_DEG);
}

uint16_t Servo_GetLastAngle(void)
{
    return g_last_angle_deg;
}

uint16_t Servo_GetLastPulseUs(void)
{
    return g_last_pulse_us;
}
