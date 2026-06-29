#include "buzzer_led.h"
#include "main.h"

/*
 * BUZZER: PC4
 * LED3_SCAN: PG13
 * LED4_ALERT: PG14
 */

void BuzzerLed_Init(void)
{
    Buzzer_Set(0);
    LedScan_Set(0);
    LedAlert_Set(0);
}

void Buzzer_Set(uint8_t on)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port,
                      BUZZER_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LedScan_Set(uint8_t on)
{
    HAL_GPIO_WritePin(LED3_SCAN_GPIO_Port,
                      LED3_SCAN_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LedAlert_Set(uint8_t on)
{
    HAL_GPIO_WritePin(LED4_ALERT_GPIO_Port,
                      LED4_ALERT_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Alert_Update(uint8_t detected, uint8_t near_warning)
{
    static uint32_t last_toggle_ms = 0U;
    static uint8_t buzzer_state = 0U;

    uint32_t now = HAL_GetTick();

    if (near_warning)
    {
        LedAlert_Set(1);

        if ((now - last_toggle_ms) >= 120U)
        {
            last_toggle_ms = now;
            buzzer_state = !buzzer_state;
        }

        Buzzer_Set(buzzer_state);
    }
    else if (detected)
    {
        LedAlert_Set(1);
        Buzzer_Set(0);
        buzzer_state = 0U;
        last_toggle_ms = now;
    }
    else
    {
        LedAlert_Set(0);
        Buzzer_Set(0);
        buzzer_state = 0U;
        last_toggle_ms = now;
    }
}
