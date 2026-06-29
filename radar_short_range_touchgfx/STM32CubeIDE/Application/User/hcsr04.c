#include "hcsr04.h"
#include "main.h"
#include "radar_config.h"

/*
 * HC-SR04 EXTI + DWT VERSION
 *
 * TRIG = PG2
 * ECHO = PG3 / EXTI3 rising + falling
 *
 * Không polling chờ Echo nữa.
 * Không dùng TIM2 input capture nữa.
 *
 * Luồng:
 * - HCSR04_StartMeasure(): phát TRIG 10us rồi thoát.
 * - PG3 rising interrupt: lưu thời điểm bắt đầu Echo.
 * - PG3 falling interrupt: lưu thời điểm kết thúc Echo, tính echo_us.
 */

#define HCSR04_TRIG_PORT   HCSR04_TRIG_GPIO_Port
#define HCSR04_TRIG_PIN    HCSR04_TRIG_Pin

#define HCSR04_ECHO_PORT   HCSR04_ECHO_GPIO_Port
#define HCSR04_ECHO_PIN    HCSR04_ECHO_Pin

static volatile HCSR04_State_t g_state = HCSR04_STATE_IDLE;

static volatile uint32_t g_rising_cycle = 0;
static volatile uint32_t g_falling_cycle = 0;

static volatile uint32_t g_rising_tick = 0;
static volatile uint32_t g_falling_tick = 0;
static volatile uint32_t g_echo_us = 0;
static volatile uint32_t g_measure_start_ms = 0;

static volatile uint32_t g_start_count = 0;
static volatile uint32_t g_rising_count = 0;
static volatile uint32_t g_falling_count = 0;
static volatile uint32_t g_timeout_count = 0;

static volatile uint32_t g_dbg_trig_high_read = 0;
static volatile uint32_t g_dbg_echo_high_read = 0;

static uint32_t g_cycles_per_us = 0;

static void HCSR04_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;

    g_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
}

static void HCSR04_DelayUs(uint32_t us)
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
        /* busy wait only for TRIG 10us */
    }
}

void HCSR04_Init(void)
{
    HCSR04_DWT_Init();

    g_state = HCSR04_STATE_IDLE;

    g_rising_cycle = 0;
    g_falling_cycle = 0;
    g_rising_tick = 0;
    g_falling_tick = 0;
    g_echo_us = 0;
    g_measure_start_ms = 0;

    g_start_count = 0;
    g_rising_count = 0;
    g_falling_count = 0;
    g_timeout_count = 0;

    g_dbg_trig_high_read = 0;
    g_dbg_echo_high_read = 0;

    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
    __HAL_GPIO_EXTI_CLEAR_IT(HCSR04_ECHO_PIN);
}

void HCSR04_StartMeasure(void)
{
    if (g_state == HCSR04_STATE_WAIT_RISING ||
        g_state == HCSR04_STATE_WAIT_FALLING)
    {
        return;
    }

    if (g_cycles_per_us == 0U)
    {
        g_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
    }

    g_rising_cycle = 0;
    g_falling_cycle = 0;
    g_rising_tick = 0;
    g_falling_tick = 0;
    g_echo_us = 0;

    g_dbg_trig_high_read = 0;
    g_dbg_echo_high_read = 0;

    g_measure_start_ms = HAL_GetTick();
    g_state = HCSR04_STATE_WAIT_RISING;
    g_start_count++;

    __HAL_GPIO_EXTI_CLEAR_IT(HCSR04_ECHO_PIN);

    /*
     * Phát TRIG 10us.
     */
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
    HCSR04_DelayUs(2);

    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);

    g_dbg_trig_high_read =
        (HAL_GPIO_ReadPin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN) == GPIO_PIN_SET) ? 1U : 0U;

    g_dbg_echo_high_read =
        (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET) ? 1U : 0U;

    HCSR04_DelayUs(HCSR04_TRIGGER_PULSE_US);

    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
}

void HCSR04_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now_cycle;

    if (GPIO_Pin != HCSR04_ECHO_PIN)
    {
        return;
    }

    if (g_cycles_per_us == 0U)
    {
        g_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
        if (g_cycles_per_us == 0U)
        {
            return;
        }
    }

    now_cycle = DWT->CYCCNT;

    /*
     * Rising edge: Echo vừa lên.
     */
    if (g_state == HCSR04_STATE_WAIT_RISING)
    {
        if (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET)
        {
            g_rising_cycle = now_cycle;
            g_rising_tick = now_cycle / g_cycles_per_us;

            g_rising_count++;
            g_state = HCSR04_STATE_WAIT_FALLING;
        }

        return;
    }

    /*
     * Falling edge: Echo vừa xuống.
     */
    if (g_state == HCSR04_STATE_WAIT_FALLING)
    {
        if (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_RESET)
        {
            g_falling_cycle = now_cycle;
            g_falling_tick = now_cycle / g_cycles_per_us;

            g_falling_count++;

            g_echo_us = (g_falling_cycle - g_rising_cycle) / g_cycles_per_us;
            g_state = HCSR04_STATE_DONE;
        }

        return;
    }
}

void HCSR04_ProcessTimeout(void)
{
    if (g_state == HCSR04_STATE_WAIT_RISING ||
        g_state == HCSR04_STATE_WAIT_FALLING)
    {
        if ((HAL_GetTick() - g_measure_start_ms) > HCSR04_TIMEOUT_MS)
        {
            g_state = HCSR04_STATE_TIMEOUT;
            g_timeout_count++;
            g_echo_us = 0;
        }
    }
}

uint8_t HCSR04_IsBusy(void)
{
    return (g_state == HCSR04_STATE_WAIT_RISING ||
            g_state == HCSR04_STATE_WAIT_FALLING);
}

uint8_t HCSR04_IsDone(void)
{
    return (g_state == HCSR04_STATE_DONE);
}

uint8_t HCSR04_IsTimeout(void)
{
    return (g_state == HCSR04_STATE_TIMEOUT);
}

uint8_t HCSR04_GetDistanceCm(uint16_t *distance_cm)
{
    uint32_t distance;

    if (distance_cm == 0)
    {
        return 0;
    }

    *distance_cm = 0;

    if (g_state != HCSR04_STATE_DONE)
    {
        return 0;
    }

    if (g_echo_us < (RADAR_MIN_DISTANCE_CM * 58U))
    {
        return 0;
    }

    if (g_echo_us > (HCSR04_TIMEOUT_MS * 1000U))
    {
        return 0;
    }

    distance = g_echo_us / 58U;

    if (distance > 65535U)
    {
        distance = 65535U;
    }

    *distance_cm = (uint16_t)distance;
    return 1;
}

uint32_t HCSR04_GetEchoUs(void)
{
    return g_echo_us;
}

void HCSR04_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    /*
     * Không dùng TIM2 input capture nữa.
     */
    (void)htim;
}

HCSR04_State_t HCSR04_GetState(void)
{
    return g_state;
}

uint32_t HCSR04_GetStartCount(void)
{
    return g_start_count;
}

uint32_t HCSR04_GetRisingCount(void)
{
    return g_rising_count;
}

uint32_t HCSR04_GetFallingCount(void)
{
    return g_falling_count;
}

uint32_t HCSR04_GetTimeoutCount(void)
{
    return g_timeout_count;
}

uint32_t HCSR04_GetLastEchoUs(void)
{
    return g_echo_us;
}

uint32_t HCSR04_DebugGetPA2HighRead(void)
{
    /*
     * Giữ tên cũ cho radar_app.c.
     * Giá trị này hiện là TRIG PG2.
     */
    return g_dbg_trig_high_read;
}

uint32_t HCSR04_DebugGetPA5HighRead(void)
{
    /*
     * Giữ tên cũ cho radar_app.c.
     * Giá trị này hiện là ECHO PG3 tại lúc vừa phát TRIG.
     * Với SR04 thật thường = 0 là bình thường.
     */
    return g_dbg_echo_high_read;
}
