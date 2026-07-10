#ifndef SCREENSCANVIEW_HPP
#define SCREENSCANVIEW_HPP

#include <gui_generated/screenscan_screen/ScreenScanViewBase.hpp>
#include <gui/screenscan_screen/ScreenScanPresenter.hpp>
#include <stdint.h>

class ScreenScanView : public ScreenScanViewBase
{
public:
    ScreenScanView();
    virtual ~ScreenScanView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void onScreenStart();

    virtual void handleTickEvent();

protected:
    uint8_t tickCounter;

    void updateRadarUi();
    void updateTarget(uint16_t angleDeg, uint16_t distanceCm, uint8_t visible);
};

#endif // SCREENSCANVIEW_HPP
