#include <gui/screenhome_screen/ScreenHomeView.hpp>

extern "C"
{
#include "radar_app.h"
#include "radar_debug.h"
}

ScreenHomeView::ScreenHomeView()
{

}

void ScreenHomeView::setupScreen()
{
    ScreenHomeViewBase::setupScreen();

    RadarDebug_Printf("[UI] ENTER ScreenHome -> RadarApp_Stop\r\n");

    RadarApp_Stop();
}

void ScreenHomeView::tearDownScreen()
{
    ScreenHomeViewBase::tearDownScreen();
}


void ScreenHomeView::onScreenStart()
{
	RadarApp_Stop();
}

