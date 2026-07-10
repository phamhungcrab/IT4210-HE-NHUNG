/*
 * radar_ui_bridge.c
 *
 *  Created on: 24 Jun 2026
 *      Author: HUNG PHAM
 */


#include "radar_ui_bridge.h"
#include "radar_config.h"
#include "cmsis_os.h"
#include "main.h"
/*
 * Bridge này là vùng dữ liệu chung:
 * - RadarTask ở C ghi dữ liệu vào đây.
 * - TouchGFX C++ đọc dữ liệu từ đây để update UI.
 *
 * Dùng critical section ngắn để tránh đang copy struct thì bị ngắt/task khác sửa.
 */

extern osMessageQueueId_t RadarMessageHandle;
extern osMessageQueueId_t RadarControlQueueHandle;

static RadarCoreData_t     g_radar_core;
static RadarControlConfig_t g_radar_control;

static void RadarUiBridge_EnterCritical(void)
{
    __disable_irq();
}

static void RadarUiBridge_ExitCritical(void)
{
    __enable_irq();
}

static void RadarUiBridge_PushControlUpdate(void)
{
    if (RadarControlQueueHandle != NULL)
    {
        RadarControlConfig_t ctrl_snapshot;

        RadarUiBridge_EnterCritical();
        ctrl_snapshot = g_radar_control;
        RadarUiBridge_ExitCritical();

        osMessageQueuePut(RadarControlQueueHandle, &ctrl_snapshot, 0U, 0U);
    }
}

void RadarUiBridge_Init(void)
{
    RadarUiBridge_EnterCritical();

    g_radar_control.radar_enabled   = 0;
    g_radar_control.scan_mode_deg   = RADAR_SCAN_MODE_180_DEG;
    g_radar_control.speed_mode      = RADAR_SPEED_MED;

    g_radar_core.angle_deg       = SERVO_CENTER_ANGLE_DEG;
    g_radar_core.distance_cm     = 0;
    g_radar_core.distance_valid  = 0;
    g_radar_core.object_detected = 0;
    g_radar_core.near_warning    = 0;
    g_radar_core.radar_status    = RADAR_STATUS_SCAN;
    g_radar_core.object_count    = 0;
    g_radar_core.last_object_distance_cm = 0;
    g_radar_core.last_object_angle_deg   = 0;
    g_radar_core.buzzer_on       = 0;
    g_radar_core.led3_on         = 0;
    g_radar_core.led4_on         = 0;
    g_radar_core.oled_connected  = 0;

    RadarUiBridge_ExitCritical();
}

void RadarUiBridge_SetData(const RadarCoreData_t *data)
{
    if (data == NULL || RadarMessageHandle == NULL)
    {
        return;
    }

    osMessageQueuePut(RadarMessageHandle, data, 0U, 0U);
}

void RadarUiBridge_GetData(RadarUiData_t *data)
{
    if (data == NULL)
    {
        return;
    }

    RadarCoreData_t received_data;

    if (RadarMessageHandle != NULL && osMessageQueueGet(RadarMessageHandle, &received_data, NULL, 0U) == osOK)
    {
        RadarUiBridge_EnterCritical();
        g_radar_core = received_data;
        RadarUiBridge_ExitCritical();
    }

    RadarUiBridge_EnterCritical();
    data->core_data = g_radar_core;
    data->control   = g_radar_control;
    RadarUiBridge_ExitCritical();
}


uint8_t RadarUiBridge_IsControlConfigChange(RadarControlConfig_t *ctrl_config) {
    static RadarControlConfig_t incoming_ctrl;

    if (ctrl_config == NULL)
    {
        return -1;
    }

    if (RadarControlQueueHandle != NULL)
    {
        if (osMessageQueueGet(RadarControlQueueHandle, &incoming_ctrl, NULL, 0U) == osOK)
        {
            *ctrl_config = incoming_ctrl;
            return 1;
        }
    }

    return 0;
}


void RadarUiBridge_SetRadarEnabled(uint8_t enabled)
{
    RadarUiBridge_EnterCritical();
    g_radar_control.radar_enabled = enabled ? 1U : 0U;
    RadarUiBridge_ExitCritical();

    RadarUiBridge_PushControlUpdate();
}

uint8_t RadarUiBridge_GetRadarEnabled(void)
{
    uint8_t enabled;
    RadarUiBridge_EnterCritical();
    enabled = g_radar_control.radar_enabled;
    RadarUiBridge_ExitCritical();
    return enabled;
}

void RadarUiBridge_SetSpeedMode(uint8_t speed_mode)
{
    if (speed_mode > RADAR_SPEED_FAST)
    {
        speed_mode = RADAR_SPEED_MED;
    }
    RadarUiBridge_EnterCritical();
    g_radar_control.speed_mode = speed_mode;
    RadarUiBridge_ExitCritical();

    RadarUiBridge_PushControlUpdate();
}

uint8_t RadarUiBridge_GetSpeedMode(void)
{
    uint8_t mode;
    RadarUiBridge_EnterCritical();
    mode = g_radar_control.speed_mode;
    RadarUiBridge_ExitCritical();
    return mode;
}

void RadarUiBridge_SetScanMode(uint8_t scan_mode_deg)
{
    if ((scan_mode_deg != RADAR_SCAN_MODE_90_DEG) &&
        (scan_mode_deg != RADAR_SCAN_MODE_180_DEG))
    {
        scan_mode_deg = RADAR_SCAN_MODE_180_DEG;
    }
    RadarUiBridge_EnterCritical();
    g_radar_control.scan_mode_deg = scan_mode_deg;
    RadarUiBridge_ExitCritical();

    RadarUiBridge_PushControlUpdate();
}

uint8_t RadarUiBridge_GetScanMode(void)
{
    uint8_t mode;
    RadarUiBridge_EnterCritical();
    mode = g_radar_control.scan_mode_deg;
    RadarUiBridge_ExitCritical();
    return mode;
}

void RadarUiBridge_NextSpeedMode(void)
{
    RadarUiBridge_EnterCritical();
    if (g_radar_control.speed_mode == RADAR_SPEED_SLOW)
    {
        g_radar_control.speed_mode = RADAR_SPEED_MED;
    }
    else if (g_radar_control.speed_mode == RADAR_SPEED_MED)
    {
        g_radar_control.speed_mode = RADAR_SPEED_FAST;
    }
    else
    {
        g_radar_control.speed_mode = RADAR_SPEED_SLOW;
    }
    RadarUiBridge_ExitCritical();

    RadarUiBridge_PushControlUpdate();
}
