#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * DISTANCE CONFIG
 * ============================================================ */
#define RADAR_MIN_DISTANCE_CM             2U
#define RADAR_MAX_DISPLAY_CM              50U
#define RADAR_OBJECT_DETECT_CM            50U
#define RADAR_NEAR_WARNING_CM             5U

/* HC-SR04 */
#define HCSR04_TRIGGER_PULSE_US           10U
#define HCSR04_TIMEOUT_MS                 25U

#define HCSR04_MIN_MEASURE_INTERVAL_MS    120U
#define HCSR04_HOLD_LAST_VALID_MS         350U
#define HCSR04_MAX_JUMP_CM                12U


/* ============================================================
 * SERVO MG90S HARDWARE PWM CONFIG
 *
 * TIM3_CH1 -> PB4
 * TIM3:
 *   PSC = 89
 *   ARR = 19999
 *   PWM = 50Hz
 *   1 tick = 1us
 *
 * Góc đúng chỉnh MIN/CENTER/MAX_PULSE.
 * Tốc độ chỉnh STEP_DEG + DELAY_MS.
 * ============================================================ */
#define SERVO_MIN_ANGLE_DEG               0U
#define SERVO_CENTER_ANGLE_DEG            90U
#define SERVO_MAX_ANGLE_DEG               180U

/*
 * Bộ cân bằng:
 * - 500..2500 đủ 180 nhưng dễ khực/cạ biên.
 * - 600..2400 êm hơn nhưng có thể thiếu góc.
 * - 550..2450 là bộ trung gian nên thử trước.
 */
#define SERVO_MIN_PULSE_US                550U
#define SERVO_CENTER_PULSE_US             1500U
#define SERVO_MAX_PULSE_US                2450U

/*
 * Nếu vẫn khực ở hai đầu:
 *   MIN = 600, MAX = 2400
 *
 * Nếu thiếu góc:
 *   MIN = 520, MAX = 2480
 *
 * Nếu muốn mở hết:
 *   MIN = 500, MAX = 2500
 */


/* ============================================================
 * SCAN MODE CONFIG
 * ============================================================ */
#define RADAR_SCAN_MODE_90_DEG            90U
#define RADAR_SCAN_MODE_180_DEG           180U

/* 90 degree scan is centered: 45 -> 135 */
#define RADAR_SCAN_90_MIN_ANGLE           45U
#define RADAR_SCAN_90_MAX_ANGLE           135U

#define RADAR_SCAN_180_MIN_ANGLE          0U
#define RADAR_SCAN_180_MAX_ANGLE          180U


/* ============================================================
 * SERVO SCAN SPEED CONFIG
 *
 * Slow/Medium/Fast:
 * - STEP_DEG càng lớn -> nhảy góc nhanh hơn nhưng dễ giật hơn.
 * - DELAY_MS càng nhỏ -> chạy nhanh hơn.
 *
 * Bộ này nhanh hơn config cũ rất nhiều:
 * cũ: 180/150/120ms
 * mới: 20/12/8ms
 * ============================================================ */
#define RADAR_SPEED_SLOW_STEP_DEG         1U
#define RADAR_SPEED_MED_STEP_DEG          2U
#define RADAR_SPEED_FAST_STEP_DEG         3U

#define RADAR_SPEED_SLOW_DELAY_MS         20U
#define RADAR_SPEED_MED_DELAY_MS          12U
#define RADAR_SPEED_FAST_DELAY_MS         8U

#define SERVO_SCAN_END_DELAY_MS           180U

/* Alias nếu code mới dùng tên SERVO_SCAN_* */
#define SERVO_SCAN_STEP_SLOW_DEG          RADAR_SPEED_SLOW_STEP_DEG
#define SERVO_SCAN_STEP_MEDIUM_DEG        RADAR_SPEED_MED_STEP_DEG
#define SERVO_SCAN_STEP_FAST_DEG          RADAR_SPEED_FAST_STEP_DEG

#define SERVO_SCAN_DELAY_SLOW_MS          RADAR_SPEED_SLOW_DELAY_MS
#define SERVO_SCAN_DELAY_MEDIUM_MS        RADAR_SPEED_MED_DELAY_MS
#define SERVO_SCAN_DELAY_FAST_MS          RADAR_SPEED_FAST_DELAY_MS


/* ============================================================
 * TEST MODES
 *
 * Bình thường để tất cả = 0.
 * Khi cần test riêng servo thì bật SERVO_TEST_ONLY_MODE = 1.
 * ============================================================ */
#define SERVO_TEST_ONLY_MODE              0U

#define SERVO_TEST_STEP_DEG               1U
#define SERVO_TEST_DELAY_MS               15U
#define SERVO_TEST_END_DELAY_MS           300U

#define SERVO_TEST_PHASE_MS               2500U

#define SERVO_THRESHOLD_TEST_MODE         0U
#define SERVO_TEST_MIN_US                 500U
#define SERVO_TEST_MAX_US                 2500U
#define SERVO_TEST_STEP_US                50U
#define SERVO_TEST_HOLD_MS                1000U

#define SERVO_DIR_TEST_MODE               0U
#define SERVO_DIR_TEST_PHASE_MS           3000U


/* ============================================================
 * LEGACY SERVO 360 CONFIG
 *
 * Giữ lại để không lỗi compile nếu file cũ còn gọi.
 * Với MG90S 180 hiện tại: không dùng mấy thông số này.
 * ============================================================ */
#define SERVO_360_STOP_US                 1500U

#define SERVO_360_SLOW_DELTA_US           25U
#define SERVO_360_MED_DELTA_US            45U
#define SERVO_360_FAST_DELTA_US           65U


/* ============================================================
 * UI GEOMETRY FOR TARGET DOT
 * ============================================================ */
#define RADAR_UI_CENTER_X                 120
#define RADAR_UI_CENTER_Y                 184
#define RADAR_UI_RADIUS_PX                90
#define RADAR_TARGET_HALF_SIZE_PX         10


/* ============================================================
 * DEBUG
 * ============================================================ */
#define RADAR_DEBUG_PRINT_INTERVAL_MS     400U


#ifdef __cplusplus
}
#endif

#endif /* RADAR_CONFIG_H */
