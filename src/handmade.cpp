#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "../src/lib/SDL2.lib")
#include <windows.h>
#include <gl/gl.h>

#include "handmade.h"

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    if (!memory->isInitialized)
    {
        memory->isInitialized = true;
    }
    GLsizei tempWidth = SDL_GetWindowSurface(window)->w;
    GLsizei tempHeight = SDL_GetWindowSurface(window)->h;

    glClearDepth(1.0f);
    glClearColor(0.0f, 0.15f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, tempWidth, tempHeight);
    SDL_SetWindowSize(window, 1280, 720);

    if (tempWidth == 1920 && tempHeight == 1080)
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }
    else
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    SDL_GL_SwapWindow(window);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    
}