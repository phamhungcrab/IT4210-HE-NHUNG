#include <gui/screensettings_screen/ScreenSettingsView.hpp>
#include <touchgfx/Unicode.hpp>

extern "C"
{
#include "radar_app.h"
#include "radar_ui_bridge.h"
#include "radar_config.h"
#include "radar_types.h"
}

ScreenSettingsView::ScreenSettingsView()
    : tickCounter(0)
{

}

void ScreenSettingsView::setupScreen()
{
    ScreenSettingsViewBase::setupScreen();

    tickCounter = 0;

    /*
     * Gán buffer riêng cho 2 wildcard:
     * Wildcard 1 = speedMode
     * Wildcard 2 = scanMode
     *
     * Làm kiểu này không phụ thuộc tên buffer do TouchGFX generate.
     */
    txtCurrentConfig.setWildcard1(speedModeBuffer);
    txtCurrentConfig.setWildcard2(scanModeBuffer);

    updateConfigText();
}

void ScreenSettingsView::tearDownScreen()
{
    ScreenSettingsViewBase::tearDownScreen();
}

void ScreenSettingsView::handleTickEvent()
{
    /*
     * Cập nhật chữ Settings chậm hơn ScreenScan.
     * 10 tick ~ 0.16s nếu 60 FPS.
     */
    tickCounter++;

    if (tickCounter >= 10)
    {
        tickCounter = 0;
        updateConfigText();
    }
}

bool ScreenSettingsView::isClicked(const touchgfx::Drawable& widget, const touchgfx::ClickEvent& evt)
{
    if (evt.getType() != touchgfx::ClickEvent::RELEASED)
    {
        return false;
    }

    int16_t x = evt.getX();
    int16_t y = evt.getY();

    return (x >= widget.getX() &&
            x <  widget.getX() + widget.getWidth() &&
            y >= widget.getY() &&
            y <  widget.getY() + widget.getHeight());
}

void ScreenSettingsView::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    /*
     * Gọi base trước để các interaction chuyển màn của TouchGFX vẫn hoạt động.
     */
    ScreenSettingsViewBase::handleClickEvent(evt);

    /*
     * Các button này là widget đã có sẵn trên màn Settings:
     * btnSpeedSlow
     * btnSpeedMedium
     * btnSpeedFast
     * btnMode90
     * btnMode180
     */

    if (isClicked(btnSpeedSlow, evt))
    {
        RadarApp_SetSpeedMode(RADAR_SPEED_SLOW);
        updateConfigText();
    }
    else if (isClicked(btnSpeedMedium, evt))
    {
        RadarApp_SetSpeedMode(RADAR_SPEED_MED);
        updateConfigText();
    }
    else if (isClicked(btnSpeedFast, evt))
    {
        RadarApp_SetSpeedMode(RADAR_SPEED_FAST);
        updateConfigText();
    }
    else if (isClicked(btnMode90, evt))
    {
        RadarApp_SetScanMode(RADAR_SCAN_MODE_90_DEG);
        updateConfigText();
    }
    else if (isClicked(btnMode180, evt))
    {
        RadarApp_SetScanMode(RADAR_SCAN_MODE_180_DEG);
        updateConfigText();
    }
}

void ScreenSettingsView::updateConfigText()
{
    RadarUiData_t data;
    RadarUiBridge_GetData(&data);

    /*
     * Wildcard 1: speedMode
     */
    if (data.control.speed_mode == RADAR_SPEED_SLOW)
    {
        Unicode::strncpy(speedModeBuffer, "SLOW", SETTING_SPEED_BUF_SIZE);
    }
    else if (data.control.speed_mode == RADAR_SPEED_FAST)
    {
        Unicode::strncpy(speedModeBuffer, "FAST", SETTING_SPEED_BUF_SIZE);
    }
    else
    {
        Unicode::strncpy(speedModeBuffer, "MED", SETTING_SPEED_BUF_SIZE);
    }

    /*
     * Wildcard 2: scanMode
     */
    Unicode::snprintf(scanModeBuffer,
                      SETTING_SCAN_BUF_SIZE,
                      "%u deg",
                      data.control.scan_mode_deg);

    txtCurrentConfig.invalidate();
}
