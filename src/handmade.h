#pragma once
#include "SDL2/SDL.h"

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)

struct game_state
{
    bool gameRunning;
};

struct game_memory
{
    bool isInitialized;
    Uint64 PermanentStorageSize;
    void *PermanentStorage;

    Uint64 TemporaryStorageSize;
    void *TemporaryStorage;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, SDL_Window *window)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub) {}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *memory)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub) {}