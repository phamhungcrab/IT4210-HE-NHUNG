#include <gui/screeninfo_screen/ScreenInfoView.hpp>
#include <touchgfx/Unicode.hpp>

extern "C"
{
#include "radar_ui_bridge.h"
#include "radar_config.h"
#include "radar_types.h"
}

/*
 * ScreenInfoView.cpp
 *
 * Màn Info chỉ đọc dữ liệu từ radar_ui_bridge.
 * Không điều khiển servo.
 * Không đọc cảm biến.
 * Không bật/tắt buzzer trực tiếp.
 *
 * Ý nghĩa các chỉ số:
 *
 * MODE
 * = chế độ quét hiện tại: 90 deg hoặc 180 deg
 *
 * SPEED
 * = tốc độ quét hiện tại: SLOW / MED / FAST
 *
 * RANGE
 * = khoảng cách của lần phát hiện object gần nhất
 * = nếu vật đang bám theo liên tục thì RANGE vẫn update liên tục
 *
 * LAST ANGLE
 * = góc của lần phát hiện object gần nhất
 * = nếu vật đang bám theo liên tục thì LAST ANGLE vẫn update liên tục
 *
 * OBJECTS
 * = số lần xuất hiện object mới
 * = chỉ tăng khi trạng thái chuyển từ không có vật -> có vật
 */

ScreenInfoView::ScreenInfoView()
    : tickCounter(0)
{

}

void ScreenInfoView::setupScreen()
{
    ScreenInfoViewBase::setupScreen();

    tickCounter = 0;

    /*
     * Gán buffer runtime cho từng wildcard.
     *
     * TouchGFX Designer nên cấu hình:
     *
     * txtScanLabel:
     *   MODE: <scanMode>
     *
     * txtSpeedLabel:
     *   SPEED: <speedMode>
     *
     * txtRangeLabel:
     *   RANGE: <lastRange>
     *
     * txtLastAngleLabel:
     *   LAST ANGLE: <lastAngle>
     *
     * txtObjectCountLabel:
     *   OBJECTS: <objectCount>
     */
    txtScanLabel.setWildcard(scanModeBuffer);
    txtSpeedLabel.setWildcard(speedModeBuffer);
    txtRangeLabel.setWildcard(minRangeBuffer);
    txtLastAngleLabel.setWildcard(lastAngleBuffer);
    txtObjectCountLabel.setWildcard(objectCountBuffer);

    updateInfoText();
}

void ScreenInfoView::tearDownScreen()
{
    ScreenInfoViewBase::tearDownScreen();
}

void ScreenInfoView::handleTickEvent()
{
    /*
     * Màn Info chỉ cần update định kỳ.
     * 10 tick khoảng 0.16s nếu TouchGFX chạy 60 FPS.
     */
    tickCounter++;

    if (tickCounter >= 10)
    {
        tickCounter = 0;
        updateInfoText();
    }
}

void ScreenInfoView::updateInfoText()
{
    RadarUiData_t data;
    RadarUiBridge_GetData(&data);

    /*
     * MODE: 90 deg / 180 deg
     */
    Unicode::snprintf(scanModeBuffer,
                      INFO_BUF_SIZE,
                      "%u deg",
                      data.control.scan_mode_deg);

    /*
     * SPEED: SLOW / MED / FAST
     */
    if (data.control.speed_mode == RADAR_SPEED_SLOW)
    {
        Unicode::strncpy(speedModeBuffer, "SLOW", INFO_BUF_SIZE);
    }
    else if (data.control.speed_mode == RADAR_SPEED_FAST)
    {
        Unicode::strncpy(speedModeBuffer, "FAST", INFO_BUF_SIZE);
    }
    else
    {
        Unicode::strncpy(speedModeBuffer, "MED", INFO_BUF_SIZE);
    }

    /*
     * RANGE:
     * Khoảng cách của lần phát hiện object gần nhất.
     *
     * Nếu chưa từng phát hiện object thì hiện "--- cm".
     */
    if (data.core_data.last_object_distance_cm == 0)
    {
        Unicode::strncpy(minRangeBuffer, "--- cm", INFO_BUF_SIZE);
    }
    else
    {
        Unicode::snprintf(minRangeBuffer,
                          INFO_BUF_SIZE,
                          "%u cm",
                          data.core_data.last_object_distance_cm);
    }

    /*
     * LAST ANGLE:
     * Góc của lần phát hiện object gần nhất.
     *
     * Nếu chưa từng phát hiện object thì hiện "--- deg".
     */
    if (data.core_data.last_object_distance_cm == 0)
    {
        Unicode::strncpy(lastAngleBuffer, "--- deg", INFO_BUF_SIZE);
    }
    else
    {
        Unicode::snprintf(lastAngleBuffer,
                          INFO_BUF_SIZE,
                          "%u deg",
                          data.core_data.last_object_angle_deg);
    }

    /*
     * OBJECTS:
     * Số lần object mới xuất hiện.
     * Không tăng liên tục nếu cùng một vật vẫn đang nằm trong vùng quét.
     */
    Unicode::snprintf(objectCountBuffer,
                      INFO_BUF_SIZE,
                      "%u",
                      data.core_data.object_count);

    txtScanLabel.invalidate();
    txtSpeedLabel.invalidate();
    txtRangeLabel.invalidate();
    txtLastAngleLabel.invalidate();
    txtObjectCountLabel.invalidate();
}
