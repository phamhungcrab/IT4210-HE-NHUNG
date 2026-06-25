#include "radar_app.h"

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include "radar_config.h"
#include "radar_ui_bridge.h"
#include "servo_mg90s.h"
#include "hcsr04.h"
#include "buzzer_led.h"
#include "radar_debug.h"
/*
 * radar_app.c
 *
 * Đây là logic lõi của radar:
 * - Điều khiển servo MG90S bằng PWM TIM5_CH2 PA1.
 * - Trigger HC-SR04 bằng PA2.
 * - Nhận Echo bằng TIM2_CH1 Input Capture PA5.
 * - Tính distance / detected / alert.
 * - Cập nhật LED3, LED4, buzzer.
 * - Đẩy dữ liệu sang radar_ui_bridge để TouchGFX đọc.
 *
 * Lưu ý:
 * - Không dùng HAL_Delay trong task.
 * - Không đọc TouchGFX ở đây.
 * - Không update UI trực tiếp ở đây.
 */

static int16_t g_angle = SERVO_CENTER_ANGLE_DEG;
static int8_t  g_direction = 1;

static uint8_t g_prev_detected = 0;
static uint8_t g_scan_led_state = 0;

/* ================= Internal helper functions ================= */

static uint16_t RadarApp_GetMinAngle(uint8_t scan_mode_deg)
{
    if (scan_mode_deg == RADAR_SCAN_MODE_90_DEG)
    {
        return RADAR_SCAN_90_MIN_ANGLE;      // 45 deg
    }

    return RADAR_SCAN_180_MIN_ANGLE;         // 0 deg
}

static uint16_t RadarApp_GetMaxAngle(uint8_t scan_mode_deg)
{
    if (scan_mode_deg == RADAR_SCAN_MODE_90_DEG)
    {
        return RADAR_SCAN_90_MAX_ANGLE;      // 135 deg
    }

    return RADAR_SCAN_180_MAX_ANGLE;         // 180 deg
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

static uint32_t RadarApp_GetDelayMs(uint8_t speed_mode)
{
    switch (speed_mode)
    {
    case RADAR_SPEED_SLOW:
        return RADAR_SPEED_SLOW_DELAY_MS;

    case RADAR_SPEED_FAST:
        return RADAR_SPEED_FAST_DELAY_MS;

    case RADAR_SPEED_MED:
    default:
        return RADAR_SPEED_MED_DELAY_MS;
    }
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
     * StartMeasure chỉ phát TRIG 10us.
     * Echo đo bằng TIM2 Input Capture interrupt.
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

        /*
         * Không busy-wait cứng.
         * Delay 1ms để nhường CPU cho TouchGFX task và task khác.
         */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void RadarApp_FillStoppedData(RadarUiData_t *data, uint8_t speed_mode, uint8_t scan_mode)
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
}

/* ================= Public functions ================= */

void RadarApp_Init(void)
{
    RadarUiBridge_Init();

    Servo_Init();
    HCSR04_Init();
    BuzzerLed_Init();

    g_angle = SERVO_CENTER_ANGLE_DEG;
    g_direction = 1;

    g_prev_detected = 0;
    g_scan_led_state = 0;

    RadarUiBridge_SetRadarEnabled(0);
    RadarUiBridge_SetSpeedMode(RADAR_SPEED_MED);
    RadarUiBridge_SetScanMode(RADAR_SCAN_MODE_180_DEG);

    Servo_SetAngle(SERVO_CENTER_ANGLE_DEG);
    RadarApp_SetSafeOutput();
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

    Servo_SetAngle(SERVO_CENTER_ANGLE_DEG);
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
    /*
     * B1 bấm ngắn sẽ gọi hàm này:
     * SLOW -> MED -> FAST -> SLOW
     */
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
    /*
     * B1 giữ lâu sẽ gọi hàm này:
     * 180 deg <-> 90 deg
     */
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

    /*
     * Lấy dữ liệu hiện tại từ bridge.
     * data giữ object_count, min_distance, last_detect_angle...
     */
    RadarUiBridge_GetData(&data);

    /*
     * Đọc riêng control state từ bridge.
     * Quan trọng: Settings/B1 có thể đổi speed/mode bất cứ lúc nào.
     */
    radar_enabled = RadarUiBridge_GetRadarEnabled();
    speed_mode    = RadarUiBridge_GetSpeedMode();
    scan_mode     = RadarUiBridge_GetScanMode();

    /*
     * Nếu radar chưa bật:
     * - servo giữ 90 độ
     * - tắt buzzer/LED
     * - đẩy trạng thái stopped sang UI
     */
    if (!radar_enabled)
    {
        Servo_SetAngle(SERVO_CENTER_ANGLE_DEG);
        RadarApp_SetSafeOutput();

        RadarApp_FillStoppedData(&data, speed_mode, scan_mode);
        RadarUiBridge_SetData(&data);

        static uint32_t debug_last_tick = 0;

        uint32_t debug_now = HAL_GetTick();

        if ((debug_now - debug_last_tick) >= 500U)
        {
            debug_last_tick = debug_now;

            RadarDebug_Printf("RADAR en=%u angle=%u valid=%u dist=%u\r\n",
                              data.radar_enabled,
                              data.angle_deg,
                              data.distance_valid,
                              data.distance_cm);

            RadarDebug_Printf("      detect=%u near=%u status=%u obj=%u lastDist=%u lastAng=%u\r\n",
                              data.object_detected,
                              data.near_warning,
                              data.radar_status,
                              data.object_count,
                              data.last_object_distance_cm,
                              data.last_object_angle_deg);
        }


        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }

    /*
     * Nếu đang bật radar:
     * đảm bảo góc nằm đúng vùng quét hiện tại.
     */
    RadarApp_ClampAngleToCurrentScanMode(scan_mode);

    /*
     * 1. Điều khiển servo đến góc hiện tại.
     */
    Servo_SetAngle((uint16_t)g_angle);

    /*
     * 2. Chờ servo ổn định.
     * MG90S không quay tức thời, nên cần chờ ngắn.
     */
    vTaskDelay(pdMS_TO_TICKS(25));

    /*
     * 3. Đo khoảng cách bằng HC-SR04.
     */
    distance_valid = RadarApp_MeasureDistance(&distance_cm);

    /*
     * 4. Xác định có vật / cảnh báo gần.
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
     * 5. Đếm object.
     * Chỉ tăng khi trạng thái chuyển từ không phát hiện -> phát hiện.
     * Nếu cứ detected liên tục thì không cộng liên tục.
     */
    /*
     * OBJECTS:
     * Chỉ tăng khi chuyển từ không có vật -> có vật.
     *
     * Ví dụ vật bám theo liên tục trong vùng quét:
     * detected = 1 liên tục
     * => object_count KHÔNG tăng liên tục.
     */
    if (detected && !g_prev_detected)
    {
        data.object_count++;
    }

    /*
     * RANGE + LAST ANGLE:
     * Update MỖI LẦN đang phát hiện được vật.
     *
     * Nghĩa là nếu vật đang bám theo radar liên tục,
     * object_count không tăng, nhưng range và angle vẫn thay đổi theo object.
     */
    if (detected)
    {
        data.last_object_distance_cm = distance_cm;
        data.last_object_angle_deg   = (uint16_t)g_angle;
    }

    g_prev_detected = detected;

    /*
     * 7. LED / buzzer.
     */
    g_scan_led_state = !g_scan_led_state;
    LedScan_Set(g_scan_led_state);

    Alert_Update(detected, near_warning);

    /*
     * 8. Ghi dữ liệu mới sang bridge cho TouchGFX đọc.
     *
     * Chú ý:
     * Trước SetData, đọc lại enabled/speed/mode từ bridge.
     * Nếu không, data cũ có thể ghi đè lựa chọn mới từ B1 hoặc Settings.
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

    /*
     * OLED chưa ghép ở bước này.
     * Sau thêm oled_status.c thì biến này sẽ đổi theo trạng thái thật.
     */
    data.oled_connected = 0;
    RadarUiBridge_SetData(&data);

    /*
     * DEBUG khi radar đang chạy.
     * Đặt ở nhánh enabled để Hercules vẫn in sau khi vào ScreenScan.
     */
    static uint32_t debug_enabled_last_tick = 0;
    uint32_t debug_enabled_now = HAL_GetTick();

    if ((debug_enabled_now - debug_enabled_last_tick) >= 500U)
    {
        debug_enabled_last_tick = debug_enabled_now;

        RadarDebug_Printf("RUN en=%u angle=%u valid=%u dist=%u echo=%luus\r\n",
                          data.radar_enabled,
                          data.angle_deg,
                          data.distance_valid,
                          data.distance_cm,
                          HCSR04_GetEchoUs());

        RadarDebug_Printf("    detect=%u near=%u status=%u obj=%u lastDist=%u lastAng=%u\r\n",
                          data.object_detected,
                          data.near_warning,
                          data.radar_status,
                          data.object_count,
                          data.last_object_distance_cm,
                          data.last_object_angle_deg);
    }

    /*
     * 9. Tính góc tiếp theo.
     */
    RadarApp_AdvanceAngle(data.scan_mode_deg, data.speed_mode);

    /*
     * 10. Delay theo tốc độ servo.
     */
    vTaskDelay(pdMS_TO_TICKS(RadarApp_GetDelayMs(data.speed_mode)));
}
