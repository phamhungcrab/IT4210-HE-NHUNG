#include "servo_mg90s.h"
#include "main.h"
#include "radar_config.h"
#include "radar_types.h"

#include "FreeRTOS.h"
#include "task.h"

/*
 * SOFTWARE PWM SERVO 360 VERSION
 *
 * Không dùng CubeMX.
 * Không dùng TIM3/TIM5.
 *
 * Servo signal = PB4 GPIO output.
 *
 * Servo 360 continuous:
 * - 1500us: stop
 * - >1500us: quay một chiều
 * - <1500us: quay chiều ngược lại
 */

#define SERVO_GPIO_PORT   GPIOB
#define SERVO_GPIO_PIN    GPIO_PIN_4

#define SERVO_360_STOP_US        1500U
#define SERVO_360_SLOW_DELTA_US  70U
#define SERVO_360_MED_DELTA_US   110U
#define SERVO_360_FAST_DELTA_US  160U

static volatile uint16_t g_last_angle = SERVO_CENTER_ANGLE_DEG;
static volatile uint16_t g_last_pulse_us = SERVO_360_STOP_US;

static TaskHandle_t g_servo_pwm_task_handle = NULL;
static uint32_t g_cycles_per_us = 0;

static void Servo_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;

    g_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
}

static void Servo_DelayUs(uint32_t us)
{
    uint32_t start;
    uint32_t ticks;

    if (g_cycles_per_us == 0U)
    {
        g_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
    }

    start = DWT->CYCCNT;
    ticks = us * g_cycles_per_us;

    while ((DWT->CYCCNT - start) < ticks)
    {
        /*
         * Chỉ busy-wait 1.3ms - 1.7ms cho xung servo.
         */
    }
}

static void Servo_PWM_Task(void *argument)
{
    (void)argument;

    for (;;)
    {
        uint16_t pulse_us = g_last_pulse_us;
        uint32_t low_us;

        if (pulse_us < 1000U)
        {
            pulse_us = 1000U;
        }

        if (pulse_us > 2000U)
        {
            pulse_us = 2000U;
        }

        /*
         * Chu kỳ servo 20ms.
         */
        HAL_GPIO_WritePin(SERVO_GPIO_PORT, SERVO_GPIO_PIN, GPIO_PIN_SET);
        Servo_DelayUs(pulse_us);

        HAL_GPIO_WritePin(SERVO_GPIO_PORT, SERVO_GPIO_PIN, GPIO_PIN_RESET);

        low_us = 20000U - pulse_us;

        /*
         * Nhường CPU phần lớn thời gian.
         */
        vTaskDelay(pdMS_TO_TICKS(low_us / 1000U));

        /*
         * Bù phần lẻ us.
         */
        Servo_DelayUs(low_us % 1000U);
    }
}

void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    Servo_DWT_Init();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_DeInit(SERVO_GPIO_PORT, SERVO_GPIO_PIN);

    GPIO_InitStruct.Pin = SERVO_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SERVO_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SERVO_GPIO_PORT, SERVO_GPIO_PIN, GPIO_PIN_RESET);

    g_last_angle = SERVO_CENTER_ANGLE_DEG;
    g_last_pulse_us = SERVO_360_STOP_US;

    if (g_servo_pwm_task_handle == NULL)
    {
        BaseType_t ok;

        ok = xTaskCreate(
            Servo_PWM_Task,
            "servo_pwm",
            256,
            NULL,
            tskIDLE_PRIORITY + 1,
            &g_servo_pwm_task_handle
        );

        if (ok != pdPASS)
        {
            Error_Handler();
        }
    }
}

void Servo_SetPulseUs(uint16_t pulse_us)
{
    if (pulse_us < 1000U)
    {
        pulse_us = 1000U;
    }

    if (pulse_us > 2000U)
    {
        pulse_us = 2000U;
    }

    g_last_pulse_us = pulse_us;
}

void Servo_Stop(void)
{
    Servo_SetPulseUs(SERVO_360_STOP_US);
}

/*
 * Với servo 360, hàm SetAngle không còn điều khiển vị trí thật.
 * Ta giữ hàm này để code cũ gọi 90 thì servo dừng.
 */
void Servo_SetAngle(uint16_t angle_deg)
{
    if (angle_deg > SERVO_MAX_ANGLE_DEG)
    {
        angle_deg = SERVO_MAX_ANGLE_DEG;
    }

    g_last_angle = angle_deg;

    /*
     * Không map 0..180 sang 1000..2000 nữa.
     * Servo 360 không có khái niệm góc tuyệt đối.
     */
    Servo_Stop();
}

void Servo_SetContinuous(int8_t direction, uint8_t speed_mode)
{
    uint16_t delta;

    switch (speed_mode)
    {
    case RADAR_SPEED_SLOW:
        delta = SERVO_360_SLOW_DELTA_US;
        break;

    case RADAR_SPEED_FAST:
        delta = SERVO_360_FAST_DELTA_US;
        break;

    case RADAR_SPEED_MED:
    default:
        delta = SERVO_360_MED_DELTA_US;
        break;
    }

    if (direction > 0)
    {
        Servo_SetPulseUs((uint16_t)(SERVO_360_STOP_US + delta));
    }
    else if (direction < 0)
    {
        Servo_SetPulseUs((uint16_t)(SERVO_360_STOP_US - delta));
    }
    else
    {
        Servo_Stop();
    }
}

uint16_t Servo_GetLastAngle(void)
{
    return g_last_angle;
}

uint16_t Servo_GetLastPulseUs(void)
{
    return g_last_pulse_us;
}
