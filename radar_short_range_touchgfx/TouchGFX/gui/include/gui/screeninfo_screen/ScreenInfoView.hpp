#ifndef SCREENINFOVIEW_HPP
#define SCREENINFOVIEW_HPP

#include <gui_generated/screeninfo_screen/ScreenInfoViewBase.hpp>
#include <gui/screeninfo_screen/ScreenInfoPresenter.hpp>

class ScreenInfoView : public ScreenInfoViewBase
{
public:
    ScreenInfoView();
    virtual ~ScreenInfoView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SCREENINFOVIEW_HPP
