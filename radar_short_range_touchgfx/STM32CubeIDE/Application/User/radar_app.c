#include "radar_app.h"

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

#include "radar_config.h"
#include "radar_types.h"
#include "radar_ui_bridge.h"
#include "servo_mg90s.h"
#include "hcsr04.h"
#include "buzzer_led.h"
#include "radar_debug.h"

/*
 * radar_app.c
 *
 * Servo 360 continuous:
 * - Không có góc tuyệt đối 0/90/180.
 * - g_angle chỉ là góc ảo để UI hiển thị.
 * - Servo quay theo hướng UI trong một khoảng thời gian.
 * - Sau đó dừng, update góc ảo, đo SR04.
 *
 * Cần tự căn:
 * - SERVO_360_*_MOVE_MS: thời gian servo quay mỗi bước.
 * - SERVO_360_DIRECTION_SIGN: nếu servo quay ngược UI thì đổi -1.
 */

/* ===== Calib servo 360 ở đây ===== */

/*
 * Nếu servo quay ngược chiều UI:
 * đổi 1 thành -1.
 */
#ifndef SERVO_360_DIRECTION_SIGN
#define SERVO_360_DIRECTION_SIGN        1
#endif

/*
 * Thời gian quay cho mỗi bước góc ảo.
 * Nếu radar_config.h đã có thì dùng bên radar_config.h.
 */
#ifndef SERVO_360_SLOW_MOVE_MS
#define SERVO_360_SLOW_MOVE_MS          120U
#endif

#ifndef SERVO_360_MED_MOVE_MS
#define SERVO_360_MED_MOVE_MS           90U
#endif

#ifndef SERVO_360_FAST_MOVE_MS
#define SERVO_360_FAST_MOVE_MS          65U
#endif

#ifndef SERVO_360_SETTLE_MS
#define SERVO_360_SETTLE_MS             80U
#endif

#ifndef RADAR_LOOP_IDLE_MS
#define RADAR_LOOP_IDLE_MS              20U
#endif

static int16_t g_angle = SERVO_CENTER_ANGLE_DEG;
static int8_t  g_direction = 1;

static uint8_t g_prev_detected = 0;
static uint8_t g_scan_led_state = 0;

static uint32_t last_debug_ms = 0U;

static uint32_t prev_start = 0U;
static uint32_t prev_rise = 0U;
static uint32_t prev_fall = 0U;
static uint32_t prev_tout = 0U;

/* ================= Internal helper functions ================= */

static uint16_t RadarApp_GetMinAngle(uint8_t scan_mode_deg)
{
    if (scan_mode_deg == RADAR_SCAN_MODE_90_DEG)
    {
        return RADAR_SCAN_90_MIN_ANGLE;
    }

    return RADAR_SCAN_180_MIN_ANGLE;
}

static uint16_t RadarApp_GetMaxAngle(uint8_t scan_mode_deg)
{
    if (scan_mode_deg == RADAR_SCAN_MODE_90_DEG)
    {
        return RADAR_SCAN_90_MAX_ANGLE;
    }

    return RADAR_SCAN_180_MAX_ANGLE;
}

static uint16_t RadarApp_GetStep(uint8_t speed_mode)
{
    switch (speed_mode)
    {
    case RADAR_SPEED_SLOW:
        return RADAR_SPEED_SLOW_STEP_DEG;

    case RADAR_SPEED_FAST:
        return RADAR_SPEED_FAST_STEP_DEG;

    case RADAR_SPEED_MED:
    default:
        return RADAR_SPEED_MED_STEP_DEG;
    }
}

static uint32_t RadarApp_GetServoMoveMs(uint8_t speed_mode)
{
    switch (speed_mode)
    {
    case RADAR_SPEED_SLOW:
        return SERVO_360_SLOW_MOVE_MS;

    case RADAR_SPEED_FAST:
        return SERVO_360_FAST_MOVE_MS;

    case RADAR_SPEED_MED:
    default:
        return SERVO_360_MED_MOVE_MS;
    }
}

static int8_t RadarApp_GetPhysicalServoDirection(int8_t ui_direction)
{
#if (SERVO_360_DIRECTION_SIGN < 0)
    return (int8_t)(-ui_direction);
#else
    return ui_direction;
#endif
}

static void RadarApp_ClampAngleToCurrentScanMode(uint8_t scan_mode_deg)
{
    uint16_t min_angle = RadarApp_GetMinAngle(scan_mode_deg);
    uint16_t max_angle = RadarApp_GetMaxAngle(scan_mode_deg);

    if (g_angle < (int16_t)min_angle || g_angle > (int16_t)max_angle)
    {
        g_angle = SERVO_CENTER_ANGLE_DEG;
        g_direction = 1;
    }
}

static void RadarApp_AdvanceAngle(uint8_t scan_mode_deg, uint8_t speed_mode)
{
    uint16_t min_angle = RadarApp_GetMinAngle(scan_mode_deg);
    uint16_t max_angle = RadarApp_GetMaxAngle(scan_mode_deg);
    uint16_t step = RadarApp_GetStep(speed_mode);

    if (g_direction > 0)
    {
        g_angle += (int16_t)step;

        if (g_angle >= (int16_t)max_angle)
        {
            g_angle = (int16_t)max_angle;
            g_direction = -1;
        }
    }
    else
    {
        g_angle -= (int16_t)step;

        if (g_angle <= (int16_t)min_angle)
        {
            g_angle = (int16_t)min_angle;
            g_direction = 1;
        }
    }
}

static void RadarApp_SetSafeOutput(void)
{
    Servo_Stop();
    Buzzer_Set(0);
    LedScan_Set(0);
    LedAlert_Set(0);
}

static uint16_t RadarApp_AbsDiffU16(uint16_t a, uint16_t b)
{
    if (a >= b)
    {
        return (uint16_t)(a - b);
    }

    return (uint16_t)(b - a);
}

static uint8_t RadarApp_MeasureDistance(uint16_t *distance_cm)
{
    uint32_t wait_start;

    if (distance_cm == 0)
    {
        return 0;
    }

    *distance_cm = 0;

    /*
     * HCSR04_StartMeasure() chỉ phát TRIG.
     * Echo được bắt bằng EXTI PG3 trong hcsr04.c.
     */
    HCSR04_StartMeasure();

    wait_start = HAL_GetTick();

    for (;;)
    {
        HCSR04_ProcessTimeout();

        if (HCSR04_IsDone())
        {
            if (HCSR04_GetDistanceCm(distance_cm))
            {
                return 1;
            }

            return 0;
        }

        if (HCSR04_IsTimeout())
        {
            return 0;
        }

        if ((HAL_GetTick() - wait_start) > HCSR04_TIMEOUT_MS)
        {
            return 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void RadarApp_FillStoppedData(RadarCoreData_t *data)
{
    if (data == NULL)
    {
        return;
    }

    data->angle_deg       = SERVO_CENTER_ANGLE_DEG;
    data->distance_cm     = 0;
    data->distance_valid  = 0;

    data->object_detected = 0;
    data->near_warning    = 0;
    data->radar_status    = RADAR_STATUS_SCAN;

    data->buzzer_on       = 0;
    data->led3_on         = 0;
    data->led4_on         = 0;
    data->oled_connected  = 0;
}
/* ================= Public functions ================= */

void RadarApp_Init(void)
{
    RadarUiBridge_Init();

    Servo_Init();
    Servo_Stop();

    RadarDebug_Printf("SERVO 360 INIT STOP\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    BuzzerLed_Init();
    HCSR04_Init();

    g_angle = SERVO_CENTER_ANGLE_DEG;
    g_direction = 1;

    g_prev_detected = 0;
    g_scan_led_state = 0;

    RadarUiBridge_SetRadarEnabled(0);
    RadarUiBridge_SetSpeedMode(RADAR_SPEED_MED);
    RadarUiBridge_SetScanMode(RADAR_SCAN_MODE_180_DEG);

    RadarApp_SetSafeOutput();

    RadarDebug_Printf("[RADAR_TASK] Init done\r\n");
}

void RadarApp_Start(void)
{
    RadarUiBridge_SetRadarEnabled(1);
}

void RadarApp_Stop(void)
{
    RadarUiBridge_SetRadarEnabled(0);

    g_angle = SERVO_CENTER_ANGLE_DEG;
    g_direction = 1;
    g_prev_detected = 0;

    RadarApp_SetSafeOutput();
}

void RadarApp_SetSpeedMode(RadarSpeedMode_t mode)
{
    if (mode > RADAR_SPEED_FAST)
    {
        mode = RADAR_SPEED_MED;
    }

    RadarUiBridge_SetSpeedMode((uint8_t)mode);
}

void RadarApp_NextSpeedMode(void)
{
    RadarUiBridge_NextSpeedMode();
}

void RadarApp_SetScanMode(uint8_t scan_mode_deg)
{
    if ((scan_mode_deg != RADAR_SCAN_MODE_90_DEG) &&
        (scan_mode_deg != RADAR_SCAN_MODE_180_DEG))
    {
        scan_mode_deg = RADAR_SCAN_MODE_180_DEG;
    }

    RadarUiBridge_SetScanMode(scan_mode_deg);
    RadarApp_ClampAngleToCurrentScanMode(scan_mode_deg);
}

void RadarApp_ToggleScanMode(void)
{
    uint8_t mode = RadarUiBridge_GetScanMode();

    if (mode == RADAR_SCAN_MODE_180_DEG)
    {
        RadarApp_SetScanMode(RADAR_SCAN_MODE_90_DEG);
    }
    else
    {
        RadarApp_SetScanMode(RADAR_SCAN_MODE_180_DEG);
    }
}

void RadarApp_TaskLoop(void)
{
    RadarUiData_t ui_view_data;
    RadarCoreData_t current_data;

    uint8_t radar_enabled;
    uint8_t speed_mode;
    uint8_t scan_mode;

    uint16_t raw_distance_cm = 0U;
    uint8_t raw_valid = 0U;
    uint32_t raw_echo_us = 0U;

    uint16_t distance_cm = 0U;
    uint8_t distance_valid = 0U;

    uint8_t detected = 0U;
    uint8_t near_warning = 0U;

    uint32_t step_delay_ms;

    uint32_t now_ms;
    uint32_t start_count;
    uint32_t rise_count;
    uint32_t fall_count;
    uint32_t tout_count;

    RadarUiBridge_GetData(&ui_view_data);
    current_data.object_count = ui_view_data.core_data.object_count;
    current_data.last_object_distance_cm =
        ui_view_data.core_data.last_object_distance_cm;
    current_data.last_object_angle_deg =
        ui_view_data.core_data.last_object_angle_deg;

    /* Đọc cấu hình từ UI */
	radar_enabled = RadarUiBridge_GetRadarEnabled();
	speed_mode = RadarUiBridge_GetSpeedMode();
	scan_mode = RadarUiBridge_GetScanMode();

    if (!radar_enabled)
    {
        Servo_Stop();
        RadarApp_SetSafeOutput();

        RadarApp_FillStoppedData(&current_data);
        RadarUiBridge_SetData(&current_data);

        osDelay(pdMS_TO_TICKS(100));
        return;
    }

    RadarApp_ClampAngleToCurrentScanMode(scan_mode);
    Servo_SetAngle((uint16_t)g_angle);

    switch (speed_mode)
    {
    case RADAR_SPEED_SLOW:
        step_delay_ms = RADAR_SPEED_SLOW_DELAY_MS;
        break;

    case RADAR_SPEED_FAST:
        step_delay_ms = RADAR_SPEED_FAST_DELAY_MS;
        break;

    case RADAR_SPEED_MED:
    default:
        step_delay_ms = RADAR_SPEED_MED_DELAY_MS;
        break;
    }

    osDelay(pdMS_TO_TICKS(step_delay_ms));

    /* Đo khoảng cách */
    raw_valid = RadarApp_MeasureDistance(&raw_distance_cm);
    raw_echo_us = HCSR04_GetLastEchoUs();

    distance_valid = raw_valid;
    distance_cm = raw_valid ? raw_distance_cm : 0U;

    if (distance_valid &&
        distance_cm >= RADAR_MIN_DISTANCE_CM &&
        distance_cm <= RADAR_OBJECT_DETECT_CM)
    {
        detected = 1U;

        if (distance_cm <= RADAR_NEAR_WARNING_CM)
        {
            near_warning = 1U;
        }
    }

    if (detected && !g_prev_detected)
    {
        current_data.object_count++;
    }

    if (detected)
    {
        current_data.last_object_distance_cm = distance_cm;
        current_data.last_object_angle_deg = (uint16_t)g_angle;
    }

    g_prev_detected = detected;

    g_scan_led_state = !g_scan_led_state;
    LedScan_Set(g_scan_led_state);

    Alert_Update(detected, near_warning);

    /* Điền dữ liệu mới */
    current_data.angle_deg = (uint16_t)g_angle;
    current_data.distance_cm = distance_cm;
    current_data.distance_valid = distance_valid;
    current_data.object_detected = detected;
    current_data.near_warning = near_warning;

    if (near_warning)
    {
        current_data.radar_status = RADAR_STATUS_ALERT;
    }
    else if (detected)
    {
        current_data.radar_status = RADAR_STATUS_DETECT;
    }
    else
    {
        current_data.radar_status = RADAR_STATUS_SCAN;
    }

    current_data.buzzer_on = near_warning ? 1U : 0U;
    current_data.led3_on = g_scan_led_state ? 1U : 0U;
    current_data.led4_on = (detected || near_warning) ? 1U : 0U;
    current_data.oled_connected = 0U;

    /* Gửi dữ liệu sang UI */
    RadarUiBridge_SetData(&current_data);

    now_ms = HAL_GetTick();
    if ((now_ms - last_debug_ms) >= 300U)
    {
        last_debug_ms = now_ms;

        start_count = HCSR04_GetStartCount();
        rise_count = HCSR04_GetRisingCount();
        fall_count = HCSR04_GetFallingCount();
        tout_count = HCSR04_GetTimeoutCount();

        RadarDebug_Printf(
            "SR04_RAW angle=%u rawValid=%u rawDist=%u echoUs=%lu st=%u\r\n",
            (unsigned int)g_angle,
            (unsigned int)raw_valid,
            (unsigned int)raw_distance_cm,
            (unsigned long)raw_echo_us,
            (unsigned int)HCSR04_GetState());

        RadarDebug_Printf(
            "UI angle=%u valid=%u dist=%u detect=%u near=%u pulse=%u\r\n",
            (unsigned int)current_data.angle_deg,
            (unsigned int)current_data.distance_valid,
            (unsigned int)current_data.distance_cm,
            (unsigned int)current_data.object_detected,
            (unsigned int)current_data.near_warning,
            (unsigned int)Servo_GetLastPulseUs());

        prev_start = start_count;
        prev_rise = rise_count;
        prev_fall = fall_count;
        prev_tout = tout_count;
    }

    RadarApp_AdvanceAngle(scan_mode, speed_mode);

    osDelay(pdMS_TO_TICKS(20));
}
