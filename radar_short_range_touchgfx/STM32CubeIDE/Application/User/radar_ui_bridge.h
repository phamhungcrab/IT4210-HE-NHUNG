#ifndef RADAR_UI_BRIDGE_H
#define RADAR_UI_BRIDGE_H

#include <stdint.h>
#include "radar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void RadarUiBridge_Init(void);

void RadarUiBridge_SetData(const RadarCoreData_t *data);
void RadarUiBridge_GetData(RadarUiData_t *data);

uint8_t RadarUiBridge_IsControlConfigChange(RadarControlConfig_t *ctrl_config);

void RadarUiBridge_SetRadarEnabled(uint8_t enabled);
uint8_t RadarUiBridge_GetRadarEnabled(void);

void RadarUiBridge_SetSpeedMode(uint8_t speed_mode);
uint8_t RadarUiBridge_GetSpeedMode(void);

void RadarUiBridge_SetScanMode(uint8_t scan_mode_deg);
uint8_t RadarUiBridge_GetScanMode(void);

void RadarUiBridge_NextSpeedMode(void);

#ifdef __cplusplus
}
#endif

#endif
