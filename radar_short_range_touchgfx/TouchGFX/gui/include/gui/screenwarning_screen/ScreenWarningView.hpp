#ifndef SCREENWARNINGVIEW_HPP
#define SCREENWARNINGVIEW_HPP

#include <gui_generated/screenwarning_screen/ScreenWarningViewBase.hpp>
#include <gui/screenwarning_screen/ScreenWarningPresenter.hpp>

class ScreenWarningView : public ScreenWarningViewBase
{
public:
    ScreenWarningView();
    virtual ~ScreenWarningView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SCREENWARNINGVIEW_HPP
