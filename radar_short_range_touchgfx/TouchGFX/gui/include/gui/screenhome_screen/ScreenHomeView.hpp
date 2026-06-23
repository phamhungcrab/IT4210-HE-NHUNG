#ifndef SCREENHOMEVIEW_HPP
#define SCREENHOMEVIEW_HPP

#include <gui_generated/screenhome_screen/ScreenHomeViewBase.hpp>
#include <gui/screenhome_screen/ScreenHomePresenter.hpp>

class ScreenHomeView : public ScreenHomeViewBase
{
public:
    ScreenHomeView();
    virtual ~ScreenHomeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SCREENHOMEVIEW_HPP
