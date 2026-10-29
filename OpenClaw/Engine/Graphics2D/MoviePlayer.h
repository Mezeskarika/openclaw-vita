#ifndef MOVIEPLAYER_H_
#define MOVIEPLAYER_H_

#include "../SharedDefines.h"
#include <SDL2/SDL.h>
#include <stdio.h>

struct smk_t;

// Plays the Smacker movies stored in the original game's CLAW.FEC, which the player copies from
// their CD. Based on the MoviePlayer of captain-claw-s60 (https://github.com/Dante-Leoncini/captain-claw-s60,
// GPL-3.0), an OpenClaw port to Symbian phones. Movie order in the file: 0 = Monolith logo,
// 1 = introduction, 2..9 = the story scenes between levels and the ending.
//
// A decoder thread runs up to FRAME_QUEUE_SIZE frames ahead of the picture, so frames that take long
// to decode (the Vita needs most of a frame's 67 ms for the busiest ones) use up the lead instead of
// stalling the picture and starving the sound
class MoviePlayer
{
public:
    MoviePlayer();
    ~MoviePlayer();

    // isInterlaced: the original's "Movies: interlaced" (each line of the movie followed by a black
    // one, at the original's 640x480) instead of stretched to fill the screen. volume: 0..1
    bool Open(const std::string& fecPath, int movieIndex, SDL_Renderer* pRenderer, bool isInterlaced, float volume);
    void Close();
    // Logs how playback went (reason: why it ended), then closes the movie
    void Stop(const std::string& reason);

    bool IsPlaying() const { return m_pSmk != NULL; }

    // Advances the movie by the elapsed time; stops it after the last frame
    void Update(uint32 msDiff);
    void Render(SDL_Renderer* pRenderer, int windowWidth, int windowHeight);

    // Mixer callback (audio thread)
    void MixAudio(Uint8* pStream, int len);

    static bool FindMovieOffset(FILE* pFile, int movieIndex, long& offset);

private:
    static int DecoderThread(void* pThis);
    void DecodeLoop();
    // Decodes the next frame into its queue slot and queues its sound (Open, then the decoder thread)
    bool DecodeNextFrame();
    void ConvertFrame(uint16_t* pDst);
    void QueueFrameAudio();
    void QueueSample(int left, int right);

    struct smk_t* m_pSmk;
    SDL_Texture* m_pTexture;
    SDL_Thread* m_pDecoderThread;

    unsigned long m_Width;
    unsigned long m_Height;
    // Stored at half height (Smacker's interlaced/doubled flag, e.g. the 640x240 Monolith logo)
    bool m_bHalfHeight;
    bool m_bInterlaced;
    // The texture has two lines per movie line: half-height movies, and all movies when interlaced
    bool m_bTextureLinesDoubled;
    unsigned long m_TextureHeight;
    unsigned long m_FrameCount;
    double m_FrameDurationUs;
    double m_ElapsedUs;

    // Decoded pictures, ready for the texture: frame i is in slot i % FRAME_QUEUE_SIZE. The decoder
    // thread writes a slot only while it is ahead by less than the queue size; the main thread
    // uploads a frame, then releases the slots up to it by raising m_FramesTaken
    std::vector<std::vector<uint16_t> > m_FrameSlots;
    SDL_atomic_t m_FramesDecoded;
    SDL_atomic_t m_FramesTaken;
    SDL_atomic_t m_StopDecoder;
    SDL_atomic_t m_DecoderDone;
    SDL_atomic_t m_DecodeFailed;
    long m_ShownFrame;

    // Movie sound converted to the mixer's rate and channel count, 16 bit. Written by the decoder,
    // read by the audio thread: each side only moves its own counter
    bool m_bHasAudio;
    int m_SourceChannels;
    int m_SourceBits;
    double m_ResampleStep;
    double m_ResamplePos;
    int m_MixerRate;
    int m_MixerChannels;
    float m_Gain;
    std::vector<int16_t> m_AudioRing;
    SDL_atomic_t m_AudioWritten;
    SDL_atomic_t m_AudioRead;

    // Playback stats, logged by Stop. The decode times are only written by whoever decodes (Open,
    // then the decoder thread) and read after it has stopped
    int m_MovieIndex;
    uint32 m_OpenMs;
    uint32 m_StartTicks;
    int m_PrebufferedSoundMs;
    unsigned long m_FramesShown;
    unsigned long m_FramesSkipped;
    unsigned long m_LateUpdates;
    Uint64 m_DecodeTicks;
    Uint64 m_WorstDecodeTicks;
    unsigned long m_FramesTimed;
    SDL_atomic_t m_AudioUnderruns;
};

#endif
