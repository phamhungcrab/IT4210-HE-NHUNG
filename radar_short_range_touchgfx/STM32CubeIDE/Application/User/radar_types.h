#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#include <stdint.h>
#include "radar_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    RADAR_SPEED_SLOW = 0,
    RADAR_SPEED_MED  = 1,
    RADAR_SPEED_FAST = 2
} RadarSpeedMode_t;

typedef enum
{
    RADAR_STATUS_SCAN = 0,
    RADAR_STATUS_DETECT,
    RADAR_STATUS_ALERT
} RadarStatus_t;

typedef struct
{
    uint16_t angle_deg;
    uint16_t distance_cm;
    uint8_t  distance_valid;

    uint8_t  object_detected;
    uint8_t  near_warning;
    uint8_t  radar_status;

    uint16_t object_count;
    uint16_t last_object_distance_cm;
    uint16_t last_object_angle_deg;

    uint8_t  buzzer_on;
    uint8_t  led3_on;
    uint8_t  led4_on;
    uint8_t  oled_connected;
} RadarCoreData_t;

typedef struct
{
    uint8_t  radar_enabled;
    uint8_t  scan_mode_deg;
    uint8_t  speed_mode;
} RadarControlConfig_t;

typedef struct
{
    RadarCoreData_t     core_data;
    RadarControlConfig_t control;
} RadarUiData_t;
#ifdef __cplusplus
}
#endif

#endif
