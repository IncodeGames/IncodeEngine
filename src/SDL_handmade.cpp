#include <string.h>
#include <cstdio>
#include <GLM/glm.hpp>

#include "handmade.cpp"

struct win32_game_code
{
    HMODULE gameCodeDLL;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    FILETIME DLLLastWriteTime;
    bool isValid;
};

inline FILETIME
Win32GetLastWriteTime(const char *filename)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }

    return (lastWriteTime);
}

inline bool
Win32TimeIsValid(FILETIME time)
{
    bool result = (time.dwLowDateTime != 0) || (time.dwHighDateTime != 0);
    return (result);
}

static win32_game_code Win32LoadGameCode(const char *srcFileName, const char *destFileName)
{
    win32_game_code result = {};

    result.DLLLastWriteTime = Win32GetLastWriteTime(srcFileName);

    CopyFile(srcFileName, destFileName, FALSE);
    result.gameCodeDLL = (HMODULE)SDL_LoadObject(destFileName);
    if (result.gameCodeDLL)
    {
        result.UpdateAndRender = (game_update_and_render *)SDL_LoadFunction(result.gameCodeDLL, "GameUpdateAndRender");
        result.GetSoundSamples = (game_get_sound_samples *)SDL_LoadFunction(result.gameCodeDLL, "GameGetSoundSamples");

        if (result.UpdateAndRender && result.GetSoundSamples)
        {
            result.isValid = true;
        }
    }

    if (!result.isValid)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load Error", "Was unable to load a valid copy of the gamecode.", NULL);
        result.UpdateAndRender = GameUpdateAndRenderStub;
        result.GetSoundSamples = GameGetSoundSamplesStub;
    }
    return result;
}

static void Win32UnloadGameCode(win32_game_code *gameCode)
{
    if (gameCode->gameCodeDLL)
    {
        SDL_UnloadObject(gameCode->gameCodeDLL);
    }

    gameCode->isValid = false;
    gameCode->UpdateAndRender = GameUpdateAndRenderStub;
    gameCode->GetSoundSamples = GameGetSoundSamplesStub;
}

static bool SDLCustom_OpenAudioContext(SDL_AudioSpec want, SDL_AudioSpec have, SDL_AudioDeviceID dev)
{
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 2;
    want.samples = 4096;
    want.callback = NULL;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (dev == 0)
    {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return false;
    }
    else
    {
        if (have.format != want.format)
        {
            SDL_Log("We didn't get Float32 audio format.");
            return false;
        }
        SDL_PauseAudioDevice(dev, 0);
        return true;
    }
}

static int SDLCustom_GetMonitorRefreshRate(SDL_Window *window)
{
    SDL_DisplayMode mode = {};
    int displayIndex = SDL_GetWindowDisplayIndex(window);
    const int defaultRefreshRate = 60;
    if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0)
    {
        return defaultRefreshRate;
    }
    if (mode.refresh_rate == 0)
    {
        return defaultRefreshRate;
    }
    return mode.refresh_rate;
}

static float SDLCustom_GetSecondsElapsed(Uint64 lastCounter, Uint64 endCounter, Uint64 perfFrequency)
{
    return (float)((endCounter - lastCounter) / (float)perfFrequency);
}

int main(int argc, char *args[])
{
    win32_game_code game = Win32LoadGameCode("handmade.dll", "handmade_temp.dll");

    int width = 1280;
    int height = 720;
    const char *title = "Game Window Title";

    game_state gameState = {};
    gameState.gameRunning = true;

    game_memory gameMemory = {};
    gameMemory.PermanentStorageSize = Gigabytes((Uint64)1);
    gameMemory.PermanentStorage = VirtualAlloc(0, gameMemory.PermanentStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    gameMemory.TemporaryStorageSize = Gigabytes((Uint64)4);
    gameMemory.TemporaryStorage = VirtualAlloc(0, gameMemory.TemporaryStorageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gameMemory.PermanentStorage && gameMemory.TemporaryStorage)
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Init Error", "SDL_Init call failed.", NULL);
            return -1;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

        SDL_Window *window = SDL_CreateWindow(title,
                                              SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED,
                                              width,
                                              height,
                                              SDL_WINDOW_OPENGL);

        if (window == nullptr)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Init Error", "SDL_CreateWindow call failed.", NULL);
            return -1;
        }

        SDL_GLContext glContext = SDL_GL_CreateContext(window);

        //Open SDL audio device
        SDL_AudioSpec want = {};
        SDL_AudioSpec have = {};
        SDL_AudioDeviceID dev = {};
        SDLCustom_OpenAudioContext(want, have, dev);

        SDL_Event inputEvent = {};

        Uint64 perfFrequency = SDL_GetPerformanceFrequency();
        Uint64 lastCounter = SDL_GetPerformanceCounter();

        float targetSecondsPerFrame = 1.0f / SDLCustom_GetMonitorRefreshRate(window);

        while (gameState.gameRunning)
        {
            FILETIME DLLNewWriteTime = Win32GetLastWriteTime("handmade.dll");
            if (CompareFileTime(&DLLNewWriteTime, &game.DLLLastWriteTime) != 0)
            {
                Win32UnloadGameCode(&game);
                game = Win32LoadGameCode("handmade.dll", "handmade_temp.dll");
            }

            while (SDL_PollEvent(&inputEvent))
            {
                if (inputEvent.type == SDL_KEYDOWN)
                {
                    SDL_Keysym key = inputEvent.key.keysym;
                    if (key.sym == SDLK_ESCAPE)
                    {
                        gameState.gameRunning = false;
                    }
                }
                if (inputEvent.type == SDL_QUIT)
                {
                    gameState.gameRunning = false;
                }
            }

            game.UpdateAndRender(&gameMemory, window);
            game.GetSoundSamples(&gameMemory);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //TODO: variable framerate?
            //Cap framerate at target rate, release processor for the rest of the time
            if (SDLCustom_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter(), perfFrequency) < targetSecondsPerFrame)
            {
                Uint32 timeToSleep = (Uint32)(((targetSecondsPerFrame -
                                                SDLCustom_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter(), perfFrequency)) *
                                               1000) -
                                              2);
                if (timeToSleep < 100)
                {
                    SDL_Delay(timeToSleep);
                }
                while (SDLCustom_GetSecondsElapsed(lastCounter, SDL_GetPerformanceCounter(), perfFrequency) < targetSecondsPerFrame)
                {
                }
            }

            //Query high resolution timestamp, and measure from the last frame.
            Uint64 endCounter = SDL_GetPerformanceCounter();
            Uint64 countElapsed = (endCounter - lastCounter);
            float millisecondsElapsed = ((1000.0f * (float)countElapsed) / (float)perfFrequency);
            float FPS = ((float)perfFrequency / (float)countElapsed);

            printf("ms: %.2f, FPS: %.2f \n", ((int)(100 * millisecondsElapsed)) / 100.0, FPS);

            lastCounter = endCounter;
        }
    }
    else
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Mem Error", "Unable to allocate permanent memory on startup.", NULL);
    }

    /*SDL_GL_DeleteContext(glContext);
 SDL_DestroyWindow(window);
 SDL_Quit();*/
    return 0;
}