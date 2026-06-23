#ifndef SCREENHOMEPRESENTER_HPP
#define SCREENHOMEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenHomeView;

class ScreenHomePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenHomePresenter(ScreenHomeView& v);

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

    virtual ~ScreenHomePresenter() {}

private:
    ScreenHomePresenter();

    ScreenHomeView& view;
};

#endif // SCREENHOMEPRESENTER_HPP
