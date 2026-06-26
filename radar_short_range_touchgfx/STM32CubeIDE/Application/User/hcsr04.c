#include "hcsr04.h"
#include "main.h"
#include "radar_config.h"

/*
 * HC-SR04:
 * - TRIG: PA2 GPIO_Output
 * - ECHO: PA5 TIM2_CH1 Input Capture
 *
 * Logic:
 * 1. HCSR04_StartMeasure() phát TRIG 10us.
 * 2. TIM2 bắt cạnh lên Echo -> lưu start.
 * 3. Đổi polarity sang falling.
 * 4. TIM2 bắt cạnh xuống Echo -> lưu end.
 * 5. echo_us = end - start.
 * 6. distance_cm = echo_us / 58.
 */

extern TIM_HandleTypeDef htim2;

static volatile HCSR04_State_t g_state = HCSR04_STATE_IDLE;
static volatile uint32_t g_rising_tick = 0;
static volatile uint32_t g_falling_tick = 0;
static volatile uint32_t g_echo_us = 0;
static volatile uint32_t g_measure_start_ms = 0;

static volatile uint32_t g_start_count = 0;
static volatile uint32_t g_rising_count = 0;
static volatile uint32_t g_falling_count = 0;
static volatile uint32_t g_timeout_count = 0;


static void HCSR04_DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void HCSR04_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);

    while ((DWT->CYCCNT - start) < ticks)
    {
        /* delay 10us rất ngắn, chấp nhận được cho trigger */
    }
}

void HCSR04_Init(void)
{
    HCSR04_DWT_Init();

    g_state = HCSR04_STATE_IDLE;
    g_echo_us = 0;

    __HAL_TIM_SET_COUNTER(&htim2, 0);

    /*
     * Start input capture interrupt một lần.
     * Sau đó mỗi lần đo chỉ reset state và phát TRIG.
     */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
}

void HCSR04_StartMeasure(void)
{
    if (g_state == HCSR04_STATE_WAIT_RISING ||
        g_state == HCSR04_STATE_WAIT_FALLING)
    {
        return;
    }

    g_rising_tick = 0;
    g_falling_tick = 0;
    g_echo_us = 0;
    g_measure_start_ms = HAL_GetTick();

    g_state = HCSR04_STATE_WAIT_RISING;
    g_start_count++;

    __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);

    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_RESET);
    HCSR04_DelayUs(2);

    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_SET);
    HCSR04_DelayUs(HCSR04_TRIGGER_PULSE_US);
    HAL_GPIO_WritePin(HCSR04_TRIG_GPIO_Port, HCSR04_TRIG_Pin, GPIO_PIN_RESET);
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

    /*
     * Lọc nhiễu:
     * HC-SR04 có khoảng đo thực tế từ khoảng 2 cm trở lên.
     * 2 cm tương đương khoảng 2 * 58 = 116 us.
     *
     * Nếu echo_us chỉ 2us, 6us, 19us... thì đó là glitch/nhiễu,
     * không phải echo thật.
     */
    if (g_echo_us < (RADAR_MIN_DISTANCE_CM * 58U))
    {
        return 0;
    }

    /*
     * Lọc xung quá dài bất thường.
     * Timeout hiện là 35ms, nên echo lớn hơn mức này cũng bỏ.
     */
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

void HCSR04_ProcessTimeout(void)
{
    if (g_state == HCSR04_STATE_WAIT_RISING ||
        g_state == HCSR04_STATE_WAIT_FALLING)
    {
        if ((HAL_GetTick() - g_measure_start_ms) > HCSR04_TIMEOUT_MS)
        {
        	g_state = HCSR04_STATE_TIMEOUT;
        	g_echo_us = 0;
        	g_timeout_count++;
            __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
        }
    }
}

void HCSR04_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2)
    {
        return;
    }

    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
    {
        return;
    }

    if (g_state == HCSR04_STATE_WAIT_RISING)
    {
        g_rising_count++;

        g_rising_tick = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        g_state = HCSR04_STATE_WAIT_FALLING;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else if (g_state == HCSR04_STATE_WAIT_FALLING)
    {
        g_falling_count++;

        g_falling_tick = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        if (g_falling_tick >= g_rising_tick)
        {
            g_echo_us = g_falling_tick - g_rising_tick;
        }
        else
        {
            g_echo_us = (0xFFFFFFFFU - g_rising_tick) + g_falling_tick + 1U;
        }

        g_state = HCSR04_STATE_DONE;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    }
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
