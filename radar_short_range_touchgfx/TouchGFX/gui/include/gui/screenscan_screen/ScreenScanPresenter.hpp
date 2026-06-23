#ifndef SCREENSCANPRESENTER_HPP
#define SCREENSCANPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenScanView;

class ScreenScanPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenScanPresenter(ScreenScanView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~ScreenScanPresenter() {}

private:
    ScreenScanPresenter();

    ScreenScanView& view;
};

#endif // SCREENSCANPRESENTER_HPP
