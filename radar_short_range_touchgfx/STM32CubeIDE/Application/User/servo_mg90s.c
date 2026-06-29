#include "servo_mg90s.h"
#include "main.h"

extern TIM_HandleTypeDef htim3;

#define SERVO_MIN_ANGLE_DEG   0U
#define SERVO_MAX_ANGLE_DEG   360U

#define SERVO_MIN_PULSE_US    500U
#define SERVO_MAX_PULSE_US    2500U
#define SERVO_STOP_PULSE_US   1500U

static uint16_t g_last_angle_deg = 90U;
static uint16_t g_last_pulse_us = SERVO_STOP_PULSE_US;

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    Servo_SetAngle(90U);
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
    Servo_SetAngle(90U);
}

uint16_t Servo_GetLastAngle(void)
{
    return g_last_angle_deg;
}

uint16_t Servo_GetLastPulseUs(void)
{
    return g_last_pulse_us;
}
