#ifndef SCREENSCANVIEW_HPP
#define SCREENSCANVIEW_HPP

#include <gui_generated/screenscan_screen/ScreenScanViewBase.hpp>
#include <gui/screenscan_screen/ScreenScanPresenter.hpp>

class ScreenScanView : public ScreenScanViewBase
{
public:
    ScreenScanView();
    virtual ~ScreenScanView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SCREENSCANVIEW_HPP
