#include "radar_app.h"

#include "main.h"
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

static void RadarApp_FillStoppedData(RadarUiData_t *data,
                                      uint8_t speed_mode,
                                      uint8_t scan_mode)
{
    if (data == 0)
    {
        return;
    }

    data->radar_enabled   = 0;
    data->angle_deg       = SERVO_CENTER_ANGLE_DEG;
    data->distance_cm     = 0;
    data->distance_valid  = 0;

    data->object_detected = 0;
    data->near_warning    = 0;
    data->radar_status    = RADAR_STATUS_SCAN;

    data->speed_mode      = speed_mode;
    data->scan_mode_deg   = scan_mode;

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
    RadarUiData_t data;

    uint8_t radar_enabled;
    uint8_t speed_mode;
    uint8_t scan_mode;

    uint16_t distance_cm = 0;
    uint8_t distance_valid = 0;

    uint8_t detected = 0;
    uint8_t near_warning = 0;

    int8_t physical_direction;
    uint32_t move_ms;

    /*
     * Lấy data hiện tại từ bridge để giữ object_count,
     * last_object_distance_cm, last_object_angle_deg...
     */
    RadarUiBridge_GetData(&data);

    radar_enabled = RadarUiBridge_GetRadarEnabled();
    speed_mode    = RadarUiBridge_GetSpeedMode();
    scan_mode     = RadarUiBridge_GetScanMode();

    /*
     * Radar đang dừng:
     * - servo 360 phải STOP hẳn
     * - tắt buzzer/LED
     * - UI về trạng thái stopped
     */
    if (!radar_enabled)
    {
        Servo_Stop();
        RadarApp_SetSafeOutput();

        RadarApp_FillStoppedData(&data, speed_mode, scan_mode);
        RadarUiBridge_SetData(&data);

        static uint32_t debug_last_tick = 0;
        uint32_t debug_now = HAL_GetTick();

        if ((debug_now - debug_last_tick) >= 500U)
        {
            debug_last_tick = debug_now;

            RadarDebug_Printf("RADAR en=%u angle=%u valid=%u dist=%u pulse=%u\r\n",
                              (unsigned int)data.radar_enabled,
                              (unsigned int)data.angle_deg,
                              (unsigned int)data.distance_valid,
                              (unsigned int)data.distance_cm,
                              (unsigned int)Servo_GetLastPulseUs());

            RadarDebug_Printf("      detect=%u near=%u status=%u obj=%u lastDist=%u lastAng=%u\r\n",
                              (unsigned int)data.object_detected,
                              (unsigned int)data.near_warning,
                              (unsigned int)data.radar_status,
                              (unsigned int)data.object_count,
                              (unsigned int)data.last_object_distance_cm,
                              (unsigned int)data.last_object_angle_deg);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }

    /*
     * Đang scan:
     * g_angle là góc ảo cho UI.
     * Servo 360 chỉ quay theo hướng trong một khoảng thời gian.
     */
    RadarApp_ClampAngleToCurrentScanMode(scan_mode);

    /*
     * 1. Cho servo quay theo hướng UI.
     * Nếu servo quay ngược UI, đổi SERVO_360_DIRECTION_SIGN ở đầu file.
     */
    physical_direction = RadarApp_GetPhysicalServoDirection(g_direction);
    move_ms = RadarApp_GetServoMoveMs(speed_mode);

    Servo_SetContinuous(physical_direction, speed_mode);
    vTaskDelay(pdMS_TO_TICKS(move_ms));

    /*
     * 2. Dừng servo.
     */
    Servo_Stop();

    /*
     * 3. Cập nhật góc ảo sau khi servo đã quay.
     * UI sẽ chạy cùng hướng g_direction.
     */
    RadarApp_AdvanceAngle(scan_mode, speed_mode);

    /*
     * 4. Chờ cơ khí ổn định rồi đo SR04.
     */
    vTaskDelay(pdMS_TO_TICKS(SERVO_360_SETTLE_MS));

    /*
     * 5. Đo khoảng cách.
     */
    distance_valid = RadarApp_MeasureDistance(&distance_cm);

    /*
     * 6. Xác định phát hiện vật / cảnh báo gần.
     */
    if (distance_valid &&
        distance_cm >= RADAR_MIN_DISTANCE_CM &&
        distance_cm <= RADAR_OBJECT_DETECT_CM)
    {
        detected = 1;

        if (distance_cm <= RADAR_NEAR_WARNING_CM)
        {
            near_warning = 1;
        }
    }

    /*
     * 7. Đếm object: chỉ tăng khi chuyển từ không có vật -> có vật.
     */
    if (detected && !g_prev_detected)
    {
        data.object_count++;
    }

    /*
     * Update khoảng cách + góc ảo cuối cùng khi đang thấy vật.
     */
    if (detected)
    {
        data.last_object_distance_cm = distance_cm;
        data.last_object_angle_deg   = (uint16_t)g_angle;
    }

    g_prev_detected = detected;

    /*
     * 8. LED / buzzer.
     */
    g_scan_led_state = !g_scan_led_state;
    LedScan_Set(g_scan_led_state);

    Alert_Update(detected, near_warning);

    /*
     * 9. Ghi dữ liệu sang bridge cho TouchGFX.
     * Đọc lại enabled/speed/mode trước khi SetData để tránh ghi đè lựa chọn UI.
     */
    data.radar_enabled = RadarUiBridge_GetRadarEnabled();
    data.speed_mode    = RadarUiBridge_GetSpeedMode();
    data.scan_mode_deg = RadarUiBridge_GetScanMode();

    data.angle_deg      = (uint16_t)g_angle;
    data.distance_cm    = distance_cm;
    data.distance_valid = distance_valid;

    data.object_detected = detected;
    data.near_warning    = near_warning;

    if (near_warning)
    {
        data.radar_status = RADAR_STATUS_ALERT;
    }
    else if (detected)
    {
        data.radar_status = RADAR_STATUS_DETECT;
    }
    else
    {
        data.radar_status = RADAR_STATUS_SCAN;
    }

    data.buzzer_on = near_warning ? 1U : 0U;
    data.led3_on   = g_scan_led_state ? 1U : 0U;
    data.led4_on   = (detected || near_warning) ? 1U : 0U;

    data.oled_connected = 0;

    RadarUiBridge_SetData(&data);

    /*
     * 10. Debug UART.
     */
    static uint32_t debug_enabled_last_tick = 0;
    uint32_t debug_enabled_now = HAL_GetTick();

    if ((debug_enabled_now - debug_enabled_last_tick) >= 500U)
    {
        debug_enabled_last_tick = debug_enabled_now;

        RadarDebug_Printf(
            "RUN en=%u angle=%u uiDir=%d phyDir=%d move=%ums valid=%u dist=%u echo=%uus pulse=%u\r\n",
            (unsigned int)data.radar_enabled,
            (unsigned int)data.angle_deg,
            (int)g_direction,
            (int)physical_direction,
            (unsigned int)move_ms,
            (unsigned int)distance_valid,
            (unsigned int)distance_cm,
            (unsigned int)HCSR04_GetLastEchoUs(),
            (unsigned int)Servo_GetLastPulseUs()
        );

        RadarDebug_Printf(
            "HCSR04 st=%u start=%u rise=%u fall=%u tout=%u echo=%uus trig=%u echoPin=%u\r\n",
            (unsigned int)HCSR04_GetState(),
            (unsigned int)HCSR04_GetStartCount(),
            (unsigned int)HCSR04_GetRisingCount(),
            (unsigned int)HCSR04_GetFallingCount(),
            (unsigned int)HCSR04_GetTimeoutCount(),
            (unsigned int)HCSR04_GetLastEchoUs(),
            (unsigned int)HCSR04_DebugGetPA2HighRead(),
            (unsigned int)HCSR04_DebugGetPA5HighRead()
        );

        RadarDebug_Printf(
            "    detect=%u near=%u status=%u obj=%u lastDist=%u lastAng=%u\r\n",
            (unsigned int)data.object_detected,
            (unsigned int)data.near_warning,
            (unsigned int)data.radar_status,
            (unsigned int)data.object_count,
            (unsigned int)data.last_object_distance_cm,
            (unsigned int)data.last_object_angle_deg
        );
    }

    /*
     * 11. Delay nhỏ để tránh loop quá dày.
     */
    vTaskDelay(pdMS_TO_TICKS(RADAR_LOOP_IDLE_MS));
}
