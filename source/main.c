// Copyright: 2007  Philip Rideout.  All rights reserved.
// License: see bsd-license.txt

#include "os.h"
#include "game.h"

int main(int argc, char** argv)
{
    unsigned int drawDelay = (unsigned int) (1000.0f / MAX_FPS);
    unsigned int currentTime;
    unsigned int previousDrawTime;
    int winx, winy, startx, starty;
    OS_Event event;
    Game *game;
    GameState state;

    osInit("Tetrita" , VIEW_WIDTH, VIEW_HEIGHT, OS_OVERLAY, 0);
    osWaitVsync(1);
    srand((unsigned) time(0));
    game = game_create();

    currentTime = osGetMilliseconds();
    previousDrawTime = currentTime;

    while (game_state(game) != EDone)
    {
        int moved = 0;
        while (osPollEvent(&event))
        {
            state = game_state(game);
            switch(event.type)
            {
                case OS_PAINT:
                    if (state != EPaused)
                        break;
                    game_draw(game);
                    osSwapBuffers();
                    break;

                case OS_DEACTIVATE:
                    if (state == EPaused)
                        break;
                    game_release(game, EPause);
                    game_draw(game);
                    osSwapBuffers();
                    break;

                case OS_MOUSEBUTTONDOWN:
                    if (event.mouse.button & OS_BUTTON_LEFT)
                    {
                        osGetWindowPos(&winx, &winy);
                        startx = winx + event.mouse.x;
                        starty = winy + event.mouse.y;
                    }
                    break;

                case OS_MOUSEMOTION:
                    if (!moved && (event.mouse.button & OS_BUTTON_LEFT))
                    {
                        int newx, newy;
                        osGetWindowPos(&winx, &winy);
                        newx = winx + event.mouse.x;
                        newy = winy + event.mouse.y;
                        osMoveWindow(winx + newx - startx, winy + newy - starty);
                        startx = newx;
                        starty = newy;
                        moved = 1;
                    }
                    break;

                case OS_ACTIVATE:
                    game_press(game, EPause);
                    break;

                case OS_KEYDOWN:
                    switch (event.key.key)
                    {
                        case OSK_DOWN:
                        case OSK_NUMPAD2:
                        case '2':
                            game_press(game, EAccelerate);
                            break;
                        case OSK_NEXT:
                        case OSK_NUMPAD3:
                        case ' ':
                        case '3':
                            game_press(game, ESlam);
                            break;
                        case OSK_NUMPAD4:
                        case OSK_LEFT:
                        case '4':
                            game_press(game, ELeft);
                            break;
                        case OSK_NUMPAD6:
                        case OSK_RIGHT:
                        case '6':
                            game_press(game, ERight);
                            break;
                        case OSK_NUMPAD8:
                        case OSK_NUMPAD5:
                        case OSK_CLEAR:
                        case OSK_UP:
                        case '8':
                        case '5':
                            game_press(game, ERotate);
                            break;
                        case 'x': case 'X': case 'q': case 'Q':
                        case OSK_ESCAPE:
                            game_press(game, EQuit);
                            break;
                    }
                    break;

                case OS_KEYUP:
                    game_release(game, EAny);
                    switch (event.key.key)
                    {
                        case OSK_DOWN:
                        case OSK_NUMPAD2:
                        case '2':
                            game_release(game, EAccelerate);
                            break;
                        case OSK_NUMPAD4:
                        case OSK_LEFT:
                        case '4':
                            game_release(game, ELeft);
                            break;
                        case OSK_NUMPAD6:
                        case OSK_RIGHT:
                        case '6':
                            game_release(game, ERight);
                            break;
                        case OSK_NUMPAD8:
                        case OSK_NUMPAD5:
                        case OSK_CLEAR:
                        case OSK_UP:
                        case '8':
                        case '5':
                            game_release(game, ERotate);
                            break;
                        case 'y': case 'Y':
                            game_release(game, EYes);
                            break;
                        case 'n': case 'N':
                            if (state == EEndQuery)
                                game_press(game, EQuit);
                            break;
                    }
                    break;

                case OS_QUIT:
                    game_press(game, EQuit);
                    break;

            }
        }

        state = game_state(game);
        currentTime = osGetMilliseconds();
        if (state != EPaused && state != EDone && currentTime - previousDrawTime > drawDelay)
        {
            game_update(game);
            game_draw(game);
            osSwapBuffers();
            previousDrawTime = currentTime;
        }
    }

    game_destroy(game);
    osQuit();
    return 0;
}
