#ifndef RADAR_CONFIG_H
#define RADAR_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Distance config ===== */
#define RADAR_MIN_DISTANCE_CM            2U
#define RADAR_MAX_DISPLAY_CM             100U
#define RADAR_OBJECT_DETECT_CM           100U
#define RADAR_NEAR_WARNING_CM            10U

/* ===== Servo MG90S PWM config =====
 * TIM5_CH2 PA1 đã cấu hình 50Hz:
 * PSC = 89, ARR = 19999
 * => 1 tick = 1 us, period = 20000 us.
 */
#define SERVO_MIN_ANGLE_DEG              0U
#define SERVO_CENTER_ANGLE_DEG           90U
#define SERVO_MAX_ANGLE_DEG              180U

#define SERVO_MIN_PULSE_US               500U
#define SERVO_CENTER_PULSE_US            1500U
#define SERVO_MAX_PULSE_US               2500U

/* ===== Scan modes ===== */
#define RADAR_SCAN_MODE_90_DEG           90U
#define RADAR_SCAN_MODE_180_DEG          180U

/* 90 degree scan is centered: 45 -> 135 */
#define RADAR_SCAN_90_MIN_ANGLE          45U
#define RADAR_SCAN_90_MAX_ANGLE          135U

#define RADAR_SCAN_180_MIN_ANGLE         0U
#define RADAR_SCAN_180_MAX_ANGLE         180U

/* ===== Speed modes ===== */
#define RADAR_SPEED_SLOW_STEP_DEG        3U
#define RADAR_SPEED_MED_STEP_DEG         5U
#define RADAR_SPEED_FAST_STEP_DEG        8U

#define RADAR_SPEED_SLOW_DELAY_MS        120U
#define RADAR_SPEED_MED_DELAY_MS         80U
#define RADAR_SPEED_FAST_DELAY_MS        50U

/* ===== HC-SR04 ===== */
#define HCSR04_TRIGGER_PULSE_US          10U
#define HCSR04_TIMEOUT_MS                15U

/* ===== UI geometry for target dot ===== */
#define RADAR_UI_CENTER_X                120
#define RADAR_UI_CENTER_Y                184
#define RADAR_UI_RADIUS_PX               90
#define RADAR_TARGET_HALF_SIZE_PX        10

#ifdef __cplusplus
}
#endif

#endif
