#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/Unicode.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    uint32_t tickCount;

    uint8_t roadFrame;
    uint8_t roadTickDiv;

    int8_t playerLane;
    int8_t obstacleLane;
    int16_t obstacleY;

    uint16_t score;
    uint16_t highScore;
    uint8_t gameSpeed;

    uint32_t rngState;

    enum { LANE_COUNT = 4 };
    int16_t laneCenterX[LANE_COUNT];
    int16_t carY;

    touchgfx::Unicode::UnicodeChar scoreBuffer[16];
    touchgfx::Unicode::UnicodeChar highBuffer[16];

    void updateRoad();
    void updateCar();
    void updateObstacle();
    void respawnObstacle();
    bool checkCollision() const;
    void movePlayer(int8_t dir);
    void updateTexts();
};

#endif // SCREEN1VIEW_HPP
