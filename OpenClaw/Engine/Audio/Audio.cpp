#include <SDL2/SDL.h>
#include <fstream>
#include <vector>

#include "Audio.h"
#include "../Events/EventMgr.h"
#include "../Events/Events.h"

#include <iostream>

using namespace std;

//############################################
//################# API ######################
//############################################

Audio::Audio()
    :
    m_bIsAudioInitialized(false),
    m_SoundVolume(0),
    m_MusicVolume(0),
    m_bSoundOn(true),
    m_bMusicOn(true)
{

}

Audio::~Audio()
{
    Terminate();
}

bool Audio::Initialize(const GameOptions& config)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Attempted to initialize Audio subsystem before SDL2 was initialized");
        return false;
    }

    if (!config.timidityCfg.empty())
    {
#if SDL_MIXER_MAJOR_VERSION > 2 || (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 6)
        // Must be set before music is first opened
        Mix_SetTimidityCfg(config.timidityCfg.c_str());
#else
        // Older SDL_mixer versions only read it from the environment
        SDL_setenv("TIMIDITY_CFG", config.timidityCfg.c_str(), 1);
#endif
        LOG("Timidity config: " + config.timidityCfg);
    }

    // Setup audio mode
    if (Mix_OpenAudio(config.frequency, MIX_DEFAULT_FORMAT, config.soundChannels, config.chunkSize) != 0)
    {
        LOG_ERROR(std::string(Mix_GetError()));
        return false;
    }

    // The device may not open at the requested rate; SDL_mixer then resamples every sound to it
    int frequency = 0;
    Uint16 format = 0;
    int channels = 0;
    if (Mix_QuerySpec(&frequency, &format, &channels) != 0)
    {
        LOG("Audio device: " + ToStr(frequency) + " Hz, " + ToStr(channels) + " channel(s)");
    }

    Mix_AllocateChannels(config.mixingChannels);

    int reservedChannels = Mix_ReserveChannels(16);
    if (reservedChannels != 16)
    {
        LOG_ERROR(std::string(Mix_GetError()));
        return false;
    }

    Mix_GroupChannels(0, 15, 1);

    m_SoundVolume = config.soundVolume;
    m_MusicVolume = config.musicVolume;
    m_bSoundOn = config.soundOn;
    m_bMusicOn = config.musicOn;


    SetSoundVolume(m_SoundVolume);
    SetMusicVolume(m_MusicVolume);

    SetSoundActive(m_bSoundOn);
    SetMusicActive(m_bMusicOn);

    m_bIsAudioInitialized = true;

    return true;
}

void Audio::Terminate()
{
}

struct _MusicInfo
{
    _MusicInfo(const char* pData, size_t size, bool isLooping, int volume, int requestId)
    {
        pMusicData = pData;
        musicSize = size;
        looping = isLooping;
        musicVolume = volume;
        musicRequestId = requestId;
    }

    const char* pMusicData;
    size_t musicSize;
    bool looping;
    int musicVolume;
    int musicRequestId;
};

// Music tracks are loaded on their own threads (see PlayMusic), so a later request can finish
// loading before an earlier one. Only the latest request may start playing, and the track it
// replaces gets freed: a MIDI track holds its own copy of every instrument it uses.
static SDL_mutex* g_pMusicMutex = NULL;
static SDL_atomic_t g_LatestMusicRequestId;
static Mix_Music* g_pCurrentMusic = NULL;

static int SetupPlayMusicThread(void* pData)
{
    _MusicInfo* pMusicInfo = (_MusicInfo*)pData;

    SDL_RWops* pRWops = SDL_RWFromMem((void*)pMusicInfo->pMusicData, pMusicInfo->musicSize);
    Mix_Music* pMusic = Mix_LoadMUS_RW(pRWops, 1);
    if (!pMusic) {
        LOG_ERROR("Mix_LoadMUS_RW: " + std::string(Mix_GetError()));
        SAFE_DELETE(pMusicInfo);
        return 0;
    }

    SDL_LockMutex(g_pMusicMutex);
    if (pMusicInfo->musicRequestId != SDL_AtomicGet(&g_LatestMusicRequestId))
    {
        // A newer track was requested while this one was loading
        SDL_UnlockMutex(g_pMusicMutex);
        Mix_FreeMusic(pMusic);
        SAFE_DELETE(pMusicInfo);
        return 0;
    }

    Mix_PlayMusic(pMusic, pMusicInfo->looping ? -1 : 0);

    if (pMusicInfo->musicVolume == -1)
    {
        Mix_PauseMusic();
    }
    else
    {
        Mix_ResumeMusic();
    }

    // Mix_PlayMusic halted the previous track, so it can go now
    if (g_pCurrentMusic)
    {
        Mix_FreeMusic(g_pCurrentMusic);
    }
    g_pCurrentMusic = pMusic;
    SDL_UnlockMutex(g_pMusicMutex);

    SAFE_DELETE(pMusicInfo);

    return 0;
}

void Audio::PlayMusic(const char* musicData, size_t musicSize, bool looping)
{
    if (g_pMusicMutex == NULL)
    {
        g_pMusicMutex = SDL_CreateMutex();
    }
    int requestId = SDL_AtomicAdd(&g_LatestMusicRequestId, 1) + 1;

    _MusicInfo* pMusicInfo = new _MusicInfo(musicData, musicSize, looping, m_bMusicOn ? m_MusicVolume : -1, requestId);

    // Playing music track takes ALOT of time for some reason so play it in another thread
    SDL_Thread* pThread = SDL_CreateThread(SetupPlayMusicThread, "SetupPlayMusicThread", (void*)pMusicInfo);
    SDL_DetachThread(pThread);
}

void Audio::PauseMusic()
{
    Mix_PauseMusic();
}

void Audio::ResumeMusic()
{
    Mix_ResumeMusic();
}

void Audio::StopMusic()
{
    // A track still loading on its thread must not start once it's done
    SDL_AtomicAdd(&g_LatestMusicRequestId, 1);
    Mix_HaltMusic();
}

void Audio::SetMusicVolume(int volumePercentage)
{
    // Music has ~ 5x more potency than sound, so max is 20 instead of 100
    volumePercentage = min(volumePercentage, 20);
    if (volumePercentage < 0)
    {
        volumePercentage = 0;
    }
    m_MusicVolume = (int)((((float)volumePercentage) / 100.0f) * (float)MIX_MAX_VOLUME);

    Mix_VolumeMusic(m_MusicVolume);
}

int Audio::GetMusicVolume()
{
    return ceil(((float)m_MusicVolume / (float)MIX_MAX_VOLUME) * 100.0f);
}

bool Audio::PlaySound(const char* soundData, size_t soundSize, const SoundProperties& soundProperties)
{
    SDL_RWops* soundRwOps = SDL_RWFromMem((void*)soundData, soundSize);
    Mix_Chunk* soundChunk = Mix_LoadWAV_RW(soundRwOps, 1);

    return PlaySound(soundChunk, soundProperties);
}

bool Audio::PlaySound(Mix_Chunk* sound, const SoundProperties& soundProperties)
{
    int chunkVolume = (int)((((float)soundProperties.volume) / 100.0f) * (float)m_SoundVolume);

    Mix_VolumeChunk(sound, chunkVolume);

    int loops = soundProperties.loops;
    int channel = Mix_PlayChannel(-1, sound, loops);
    if (channel == -1)
    {
        LOG_ERROR("Failed to play chunk: " + std::string(Mix_GetError()));
        return false;
    }

    if (!Mix_SetPosition(channel, soundProperties.angle, soundProperties.distance))
    {
        LOG_ERROR("Mix_SetPosition: " + std::string(Mix_GetError()));
        return false;
    }

    if (!m_bSoundOn)
    {
        Mix_Pause(channel);
    }

    return true;
}

void Audio::SetSoundVolume(int volumePercentage)
{
    volumePercentage = min(volumePercentage, 100);
    if (volumePercentage < 0)
    {
        volumePercentage = 0;
    }
    m_SoundVolume = (int)((((float)volumePercentage) / 100.0f) * (float)MIX_MAX_VOLUME);

    if (m_bSoundOn)
    {
        Mix_Volume(-1, m_SoundVolume);
    }
}

int Audio::GetSoundVolume()
{
    return ceil(((float)m_SoundVolume / (float)MIX_MAX_VOLUME) * 100.0f);
}

void Audio::StopAllSounds()
{
    Mix_HaltChannel(-1);
    StopMusic();
}

void Audio::PauseAllSounds()
{
    Mix_Pause(-1);
}

void Audio::ResumeAllSounds()
{
    Mix_Resume(-1);
}

void Audio::SetSoundActive(bool active)
{ 
    if (m_bSoundOn == active)
    {
        return;
    }

    if (active)
    {
        Mix_Resume(-1);
        Mix_Volume(-1, m_SoundVolume);
    }
    else
    {
        Mix_Pause(-1);
    }

    m_bSoundOn = active; 
}

void Audio::SetMusicActive(bool active)
{ 
    if (m_bMusicOn == active)
    {
        return;
    }

    if (active)
    {
        ResumeMusic();
    }
    else
    {
        PauseMusic();
    }

    m_bMusicOn = active; 
}

