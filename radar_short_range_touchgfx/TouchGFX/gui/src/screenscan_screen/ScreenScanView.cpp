#include <gui/screenscan_screen/ScreenScanView.hpp>
#include <touchgfx/Unicode.hpp>
#include <math.h>

extern "C"
{
#include "radar_app.h"
#include "radar_ui_bridge.h"
#include "radar_config.h"
#include "radar_types.h"
#include "radar_debug.h"
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ScreenScanView::ScreenScanView()
    : tickCounter(0)
{

}

void ScreenScanView::setupScreen()
{
    ScreenScanViewBase::setupScreen();

    tickCounter = 0;

    RadarDebug_Printf("[UI] ENTER ScreenScan -> RadarApp_Start\r\n");

    RadarApp_Start();

    RadarUiData_t dbg;
    RadarUiBridge_GetData(&dbg);
    RadarDebug_Printf("[UI] After Start: en=%u speed=%u mode=%u\r\n",
                      dbg.control.radar_enabled,
                      dbg.control.speed_mode,
                      dbg.control.scan_mode_deg);

    txtrSweepGreen.setVisible(true);
    txtrSweepRed.setVisible(false);
    imgTarget.setVisible(false);

    updateRadarUi();
}

void ScreenScanView::tearDownScreen()
{
    ScreenScanViewBase::tearDownScreen();
}

void ScreenScanView::onScreenStart() {
	RadarApp_Start();
}

void ScreenScanView::handleTickEvent()
{
    /*
     * TouchGFX thường tick ~60 FPS.
     * Không cần update text 60 lần/giây.
     * 3 tick update 1 lần là đủ mượt.
     */
    tickCounter++;

    if (tickCounter >= 3)
    {
        tickCounter = 0;
        updateRadarUi();
    }
}

void ScreenScanView::updateRadarUi()
{
    RadarUiData_t data;
    RadarUiBridge_GetData(&data);

    /*
     * Update angle text.
     */
    Unicode::snprintf(txtAngleValueBuffer,
                      TXTANGLEVALUE_SIZE,
                      "%03u",
                      data.core_data.angle_deg);
    txtAngleValue.invalidate();

    /*
     * Update distance text.
     */
    if (data.core_data.distance_valid)
    {
        Unicode::snprintf(txtDistanceValueBuffer,
                          TXTDISTANCEVALUE_SIZE,
                          "%03u cm",
                          data.core_data.distance_cm);
    }
    else
    {
        Unicode::snprintf(txtDistanceValueBuffer,
                          TXTDISTANCEVALUE_SIZE,
                          "--- cm");
    }
    txtDistanceValue.invalidate();

    /*
     * Update status text.
     */
    if (data.core_data.radar_status == RADAR_STATUS_ALERT)
    {
        Unicode::strncpy(txtStatusValueBuffer,
                         "ALERT",
                         TXTSTATUSVALUE_SIZE);
    }
    else if (data.core_data.radar_status == RADAR_STATUS_DETECT)
    {
        Unicode::strncpy(txtStatusValueBuffer,
                         "DETECT",
                         TXTSTATUSVALUE_SIZE);
    }
    else
    {
        Unicode::strncpy(txtStatusValueBuffer,
                         "SCAN",
                         TXTSTATUSVALUE_SIZE);
    }
    txtStatusValue.invalidate();

    /*
     * Sweep màu:
     * - xanh khi scan bình thường
     * - đỏ khi detect hoặc alert
     */
    if (data.core_data.object_detected || data.core_data.near_warning)
    {
        txtrSweepGreen.setVisible(false);
        txtrSweepRed.setVisible(true);
    }
    else
    {
        txtrSweepGreen.setVisible(true);
        txtrSweepRed.setVisible(false);
    }

    /*
     * Quay sweep theo góc servo.
     * 90 độ là hướng thẳng lên.
     *
     * TouchGFX TextureMapper thường dùng radian cho setAngles.
     */
    float zAngle = ((float)data.core_data.angle_deg - 90.0f) * ((float)M_PI / 180.0f);

    txtrSweepGreen.setAngles(0.0f, 0.0f, zAngle);
    txtrSweepRed.setAngles(0.0f, 0.0f, zAngle);

    txtrSweepGreen.invalidate();
    txtrSweepRed.invalidate();

    /*
     * Target chỉ hiện khi có vật.
     */
    if (data.core_data.object_detected && data.core_data.distance_valid)
    {
        updateTarget(data.core_data.angle_deg, data.core_data.distance_cm, 1);
    }
    else
    {
        updateTarget(data.core_data.angle_deg, data.core_data.distance_cm, 0);
    }
}

void ScreenScanView::updateTarget(uint16_t angleDeg, uint16_t distanceCm, uint8_t visible)
{
    if (!visible)
    {
        imgTarget.setVisible(false);
        imgTarget.invalidate();
        return;
    }

    /*
     * Map distance cm -> radius pixel.
     * distance càng xa thì target càng ra sát viền radar.
     */
    if (distanceCm > RADAR_MAX_DISPLAY_CM)
    {
        distanceCm = RADAR_MAX_DISPLAY_CM;
    }

    float r = ((float)distanceCm * (float)RADAR_UI_RADIUS_PX) /
              (float)RADAR_MAX_DISPLAY_CM;

    /*
     * Quy ước:
     * angle 0   = trái
     * angle 90  = trên
     * angle 180 = phải
     */
    float theta = (float)angleDeg * ((float)M_PI / 180.0f);

    int16_t x = (int16_t)((float)RADAR_UI_CENTER_X - cosf(theta) * r);
    int16_t y = (int16_t)((float)RADAR_UI_CENTER_Y - sinf(theta) * r);

    imgTarget.setXY(x - RADAR_TARGET_HALF_SIZE_PX,
                    y - RADAR_TARGET_HALF_SIZE_PX);

    imgTarget.setVisible(true);
    imgTarget.invalidate();
}
