#include <gui/screen1_screen/Screen1View.hpp>
#include "cmsis_os.h"

extern "C" {
    extern osMessageQueueId_t GameCtrlQueueHandle;
}
static uint16_t savedHighScore = 0;
Screen1View::Screen1View()
    : tickCount(0),
      roadFrame(0),
      roadTickDiv(0),
      playerLane(1),
      obstacleLane(1),
      obstacleY(0),
      score(0),
      highScore(0),
      gameSpeed(2),
      rngState(0x12345678),
      carY(235)
{
	laneCenterX[0] = 30;
	laneCenterX[1] = 90;
	laneCenterX[2] = 150;
	laneCenterX[3] = 210;
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    tickCount = 0;
    roadFrame = 0;
    roadTickDiv = 0;

    playerLane = 1;
    score = 0;
    highScore = savedHighScore;
    gameSpeed = 2;

    txtScore.setWildcard(scoreBuffer);
    txtHigh.setWildcard(highBuffer);

    updateRoad();
    updateCar();
    updateTexts();
    respawnObstacle();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
    Screen1ViewBase::handleTickEvent();
    tickCount++;

    uint8_t cmd = 0;
    while (osMessageQueueGetCount(GameCtrlQueueHandle) > 0)
    {
        if (osMessageQueueGet(GameCtrlQueueHandle, &cmd, NULL, 0) == osOK)
        {
            if (cmd == 'L')
            {
                movePlayer(-1);
            }
            else if (cmd == 'R')
            {
                movePlayer(1);
            }
        }
    }

    roadTickDiv++;
    if (roadTickDiv >= 3)
    {
        roadTickDiv = 0;
        roadFrame = (roadFrame + 1) % 5;
        updateRoad();
    }

    updateObstacle();

    if (checkCollision())
    {
        if (score > highScore)
        {
            highScore = score;
            savedHighScore = highScore;
        }

        score = 0;
        gameSpeed = 2;
        playerLane = 1;

        updateCar();
        updateTexts();
        respawnObstacle();
    }
}

void Screen1View::updateRoad()
{
    track0.setVisible(roadFrame == 0);
    track1.setVisible(roadFrame == 1);
    track2.setVisible(roadFrame == 2);
    track3.setVisible(roadFrame == 3);
    track4.setVisible(roadFrame == 4);

    track0.invalidate();
    track1.invalidate();
    track2.invalidate();
    track3.invalidate();
    track4.invalidate();
}

void Screen1View::updateCar()
{
    int16_t x = laneCenterX[playerLane] - car.getWidth() / 2;
    car.setXY(x, carY);
    car.invalidate();
}

void Screen1View::movePlayer(int8_t dir)
{
    int8_t newLane = playerLane + dir;

    if (newLane < 0)
    {
        newLane = 0;
    }
    if (newLane > LANE_COUNT - 1)
    {
        newLane = LANE_COUNT - 1;
    }

    if (newLane != playerLane)
    {
        playerLane = newLane;
        updateCar();
    }
}

void Screen1View::respawnObstacle()
{
    rngState = 1664525U * rngState + 1013904223U;
    obstacleLane = rngState % LANE_COUNT;

    obstacleY = -(int16_t)obstacle.getHeight();

    int16_t x = laneCenterX[obstacleLane] - obstacle.getWidth() / 2;
    obstacle.setXY(x, obstacleY);
    obstacle.invalidate();
}

void Screen1View::updateObstacle()
{
    obstacleY += gameSpeed;

    int16_t x = laneCenterX[obstacleLane] - obstacle.getWidth() / 2;
    obstacle.setXY(x, obstacleY);
    obstacle.invalidate();

    if (obstacleY > 320)
    {
        score++;

        if (score > highScore)
        {
            highScore = score;
            savedHighScore = highScore;
        }

        if ((score % 5 == 0) && (gameSpeed < 8))
        {
            gameSpeed++;
        }

        updateTexts();
        respawnObstacle();
    }
}

bool Screen1View::checkCollision() const
{
    if (playerLane != obstacleLane)
    {
        return false;
    }

    int16_t carTop = carY + 4;
    int16_t carBottom = carY + car.getHeight() - 4;

    int16_t obstacleTop = obstacleY + 4;
    int16_t obstacleBottom = obstacleY + obstacle.getHeight() - 4;

    if (obstacleBottom < carTop)
    {
        return false;
    }

    if (obstacleTop > carBottom)
    {
        return false;
    }

    return true;
}

void Screen1View::updateTexts()
{
    txtScore.invalidate();
    txtHigh.invalidate();

    touchgfx::Unicode::snprintf(scoreBuffer, 16, "%d", score);
    touchgfx::Unicode::snprintf(highBuffer, 16, "%d", highScore);

    txtScore.resizeToCurrentText();
    txtHigh.resizeToCurrentText();

    txtScore.invalidate();
    txtHigh.invalidate();
}
