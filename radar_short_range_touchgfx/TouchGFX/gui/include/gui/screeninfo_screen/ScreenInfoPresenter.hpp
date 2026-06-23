#ifndef SCREENINFOPRESENTER_HPP
#define SCREENINFOPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenInfoView;

class ScreenInfoPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenInfoPresenter(ScreenInfoView& v);

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

    virtual ~ScreenInfoPresenter() {}

private:
    ScreenInfoPresenter();

    ScreenInfoView& view;
};

#endif // SCREENINFOPRESENTER_HPP
