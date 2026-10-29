#ifndef MOVIEPLAYER_H_
#define MOVIEPLAYER_H_

#include "../SharedDefines.h"
#include <SDL2/SDL.h>
#include <stdio.h>

struct smk_t;

// Plays the Smacker movies stored in the original game's CLAW.FEC, which the player copies from
// their CD. Based on the MoviePlayer of captain-claw-s60 (https://github.com/Dante-Leoncini/captain-claw-s60,
// GPL-3.0), an OpenClaw port to Symbian phones. Movie order in the file: 0 = Monolith logo,
// 1 = introduction, 2..9 = the story scenes between levels and the ending
class MoviePlayer
{
public:
    MoviePlayer();
    ~MoviePlayer();

    // isInterlaced: the original's "Movies: interlaced" (each line of the movie followed by a black
    // one, at the original's 640x480) instead of stretched to fill the screen. volume: 0..1
    bool Open(const std::string& fecPath, int movieIndex, SDL_Renderer* pRenderer, bool isInterlaced, float volume);
    void Close();

    bool IsPlaying() const { return m_pSmk != NULL; }

    // Advances the movie by the elapsed time; closes it after the last frame
    void Update(uint32 msDiff);
    void Render(SDL_Renderer* pRenderer, int windowWidth, int windowHeight);

    // Mixer callback (audio thread)
    void MixAudio(Uint8* pStream, int len);

    static bool FindMovieOffset(FILE* pFile, int movieIndex, long& offset);

private:
    bool DecodeNextFrame();
    void ConvertFrame();
    void QueueFrameAudio();
    void QueueSample(int left, int right);

    struct smk_t* m_pSmk;
    SDL_Texture* m_pTexture;

    unsigned long m_Width;
    unsigned long m_Height;
    // Stored at half height (Smacker's interlaced/doubled flag, e.g. the 640x240 Monolith logo)
    bool m_bHalfHeight;
    bool m_bInterlaced;
    // The texture has two lines per movie line: half-height movies, and all movies when interlaced
    bool m_bTextureLinesDoubled;
    unsigned long m_FrameCount;
    unsigned long m_CurrentFrame;
    double m_FrameDurationUs;
    double m_ElapsedUs;
    std::vector<uint16_t> m_Pixels;

    // Movie sound converted to the mixer's rate and channel count, 16 bit. Written by the main
    // thread, read by the audio thread: each side only moves its own counter
    bool m_bHasAudio;
    int m_SourceChannels;
    int m_SourceBits;
    double m_ResampleStep;
    double m_ResamplePos;
    int m_MixerChannels;
    float m_Gain;
    std::vector<int16_t> m_AudioRing;
    SDL_atomic_t m_AudioWritten;
    SDL_atomic_t m_AudioRead;
};

#endif
