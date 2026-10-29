#include "MoviePlayer.h"

#include <SDL2/SDL_mixer.h>
#include <libsmacker/smacker.h>

#include <string.h>

static void MoviePlayerMixAudio(void* pThis, Uint8* pStream, int len)
{
    static_cast<MoviePlayer*>(pThis)->MixAudio(pStream, len);
}

// Samples between the decoder and the mixer, about 3 seconds of mono sound at 22 kHz. A power of
// two, so the ring position stays right when the counters wrap around
static const size_t AUDIO_RING_SAMPLES = 1 << 16;

// How far the decoder may run ahead: about half a second at 15 fps (a quarter at 30). A slot holds a
// whole texture, 300 KB for a 320x240 movie with doubled lines
static const int FRAME_QUEUE_SIZE = 8;

// Frames decoded before the movie starts, so the picture and the sound start with a lead
static const int PREBUFFER_FRAMES = 4;

static uint32_t ReadU32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

MoviePlayer::MoviePlayer()
    :
    m_pSmk(NULL),
    m_pTexture(NULL),
    m_pDecoderThread(NULL),
    m_Width(0),
    m_Height(0),
    m_bHalfHeight(false),
    m_bInterlaced(false),
    m_bTextureLinesDoubled(false),
    m_TextureHeight(0),
    m_FrameCount(0),
    m_FrameDurationUs(66666.0),
    m_ElapsedUs(0.0),
    m_ShownFrame(-1),
    m_bHasAudio(false),
    m_SourceChannels(0),
    m_SourceBits(0),
    m_ResampleStep(1.0),
    m_ResamplePos(0.0),
    m_MixerRate(0),
    m_MixerChannels(1),
    m_Gain(1.0f),
    m_MovieIndex(-1),
    m_OpenMs(0),
    m_StartTicks(0),
    m_PrebufferedSoundMs(0),
    m_FramesShown(0),
    m_FramesSkipped(0),
    m_LateUpdates(0),
    m_DecodeTicks(0),
    m_WorstDecodeTicks(0),
    m_FramesTimed(0)
{
    SDL_AtomicSet(&m_FramesDecoded, 0);
    SDL_AtomicSet(&m_FramesTaken, 0);
    SDL_AtomicSet(&m_StopDecoder, 0);
    SDL_AtomicSet(&m_DecoderDone, 0);
    SDL_AtomicSet(&m_DecodeFailed, 0);
    SDL_AtomicSet(&m_AudioWritten, 0);
    SDL_AtomicSet(&m_AudioRead, 0);
    SDL_AtomicSet(&m_AudioUnderruns, 0);
}

MoviePlayer::~MoviePlayer()
{
    Close();
}

// CLAW.FEC's own directory is encrypted, but the movies are stored one after the other, each a
// complete Smacker file: find the first header, then skip from header to header by the movie's size
bool MoviePlayer::FindMovieOffset(FILE* pFile, int movieIndex, long& offset)
{
    const size_t windowSize = 4096;
    std::vector<unsigned char> window(windowSize);
    long searchFrom = 0;
    for (int index = 0; index <= movieIndex; index++)
    {
        if (fseek(pFile, searchFrom, SEEK_SET) != 0)
        {
            return false;
        }
        size_t got = fread(&window[0], 1, windowSize, pFile);
        long found = -1;
        for (size_t i = 0; i + 4 <= got; i++)
        {
            if (memcmp(&window[i], "SMK2", 4) == 0 || memcmp(&window[i], "SMK4", 4) == 0)
            {
                found = searchFrom + (long)i;
                break;
            }
        }
        if (found < 0)
        {
            return false;
        }
        if (index == movieIndex)
        {
            offset = found;
            return true;
        }

        // Header (104 bytes): frame count at 12, flags at 20 (bit 0: an extra ring frame), size of
        // the Huffman trees at 52. Then 4 bytes of size and 1 byte of type per frame, the trees and
        // the frames themselves (the low 2 bits of a frame size are flags)
        unsigned char header[104];
        if (fseek(pFile, found, SEEK_SET) != 0 || fread(header, 1, sizeof(header), pFile) != sizeof(header))
        {
            return false;
        }
        uint32_t frames = ReadU32(header + 12) + ((ReadU32(header + 20) & 1) ? 1 : 0);
        uint32_t treesSize = ReadU32(header + 52);
        long total = 104 + (long)frames * 5 + (long)treesSize;
        for (uint32_t frame = 0; frame < frames; frame++)
        {
            unsigned char size[4];
            if (fread(size, 1, 4, pFile) != 4)
            {
                return false;
            }
            total += (long)(ReadU32(size) & ~3u);
        }
        searchFrom = found + total;
    }
    return false;
}

bool MoviePlayer::Open(const std::string& fecPath, int movieIndex, SDL_Renderer* pRenderer, bool isInterlaced,
                       float volume)
{
    Close();
    const uint32 openStart = SDL_GetTicks();

    FILE* pFile = fopen(fecPath.c_str(), "rb");
    if (!pFile)
    {
        LOG_WARNING("Movies not available: " + fecPath);
        return false;
    }

    long offset = 0;
    if (!FindMovieOffset(pFile, movieIndex, offset) || fseek(pFile, offset, SEEK_SET) != 0)
    {
        LOG_ERROR("Movie " + ToStr(movieIndex) + " not found in " + fecPath);
        fclose(pFile);
        return false;
    }

    // From here libsmacker owns the file (it closes it) and reads each frame from the card when needed
    m_pSmk = smk_open_filepointer(pFile, SMK_MODE_DISK);
    if (!m_pSmk)
    {
        LOG_ERROR("Could not open movie " + ToStr(movieIndex) + " at offset " + ToStr((int)offset));
        return false;
    }

    unsigned char yScaleMode = SMK_FLAG_Y_NONE;
    smk_info_all(m_pSmk, NULL, &m_FrameCount, &m_FrameDurationUs);
    smk_info_video(m_pSmk, &m_Width, &m_Height, &yScaleMode);
    m_bHalfHeight = (yScaleMode != SMK_FLAG_Y_NONE) || (m_Width >= m_Height * 2);
    m_bInterlaced = isInterlaced;
    m_bTextureLinesDoubled = m_bHalfHeight || m_bInterlaced;
    m_TextureHeight = m_bTextureLinesDoubled ? m_Height * 2 : m_Height;

    unsigned char trackMask = 0;
    unsigned char channels[7], bitDepth[7];
    unsigned long audioRate[7];
    smk_info_audio(m_pSmk, &trackMask, channels, bitDepth, audioRate);

    int mixerRate = 0, mixerChannels = 0;
    Uint16 mixerFormat = 0;
    Mix_QuerySpec(&mixerRate, &mixerFormat, &mixerChannels);

    std::string soundInfo = "no sound track";
    if (trackMask & SMK_AUDIO_TRACK_0)
    {
        soundInfo = ToStr((int)audioRate[0]) + " Hz, " + ToStr((int)bitDepth[0]) + " bit, " +
            ToStr((int)channels[0]) + " channel(s)";
    }
    m_bHasAudio = (trackMask & SMK_AUDIO_TRACK_0) && (bitDepth[0] == 8 || bitDepth[0] == 16) &&
        (channels[0] == 1 || channels[0] == 2) && audioRate[0] > 0 &&
        mixerRate > 0 && mixerFormat == AUDIO_S16SYS && (mixerChannels == 1 || mixerChannels == 2);
    if ((trackMask & SMK_AUDIO_TRACK_0) && !m_bHasAudio)
    {
        LOG_WARNING("Movie sound not playable (" + soundInfo + ", mixer " + ToStr(mixerRate) + " Hz, " +
            ToStr(mixerChannels) + " channel(s))");
    }

    smk_enable_video(m_pSmk, 1);
    smk_enable_audio(m_pSmk, 0, m_bHasAudio ? 1 : 0);

    // Stretched movies are scaled by an uneven factor, which looks best filtered. The hint is read
    // when the texture is created
    const char* pScaleQuality = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
    std::string previousScaleQuality = pScaleQuality ? pScaleQuality : "nearest";
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, m_bInterlaced ? "nearest" : "linear");
    m_pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        (int)m_Width, (int)m_TextureHeight);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, previousScaleQuality.c_str());
    if (!m_pTexture)
    {
        LOG_ERROR("Could not create movie texture: " + std::string(SDL_GetError()));
        Close();
        return false;
    }
    m_FrameSlots.assign(FRAME_QUEUE_SIZE, std::vector<uint16_t>(m_Width * m_TextureHeight, 0));

    if (m_bHasAudio)
    {
        m_SourceChannels = channels[0];
        m_SourceBits = bitDepth[0];
        m_MixerRate = mixerRate;
        m_ResampleStep = (double)audioRate[0] / mixerRate;
        m_ResamplePos = 0.0;
        m_MixerChannels = mixerChannels;
        m_Gain = volume;
        m_AudioRing.assign(AUDIO_RING_SAMPLES, 0);
    }
    SDL_AtomicSet(&m_AudioWritten, 0);
    SDL_AtomicSet(&m_AudioRead, 0);
    SDL_AtomicSet(&m_AudioUnderruns, 0);

    m_MovieIndex = movieIndex;
    m_ElapsedUs = 0.0;
    m_ShownFrame = -1;
    m_FramesShown = 0;
    m_FramesSkipped = 0;
    m_LateUpdates = 0;
    m_DecodeTicks = 0;
    m_WorstDecodeTicks = 0;
    m_FramesTimed = 0;
    SDL_AtomicSet(&m_FramesDecoded, 0);
    SDL_AtomicSet(&m_FramesTaken, 0);
    SDL_AtomicSet(&m_StopDecoder, 0);
    SDL_AtomicSet(&m_DecoderDone, 0);
    SDL_AtomicSet(&m_DecodeFailed, 0);

    // The first frames here, before the decoder thread owns libsmacker
    smk_first(m_pSmk);
    ConvertFrame(&m_FrameSlots[0][0]);
    QueueFrameAudio();
    SDL_AtomicSet(&m_FramesDecoded, 1);
    while (SDL_AtomicGet(&m_FramesDecoded) < PREBUFFER_FRAMES && (unsigned long)SDL_AtomicGet(&m_FramesDecoded) < m_FrameCount)
    {
        if (!DecodeNextFrame())
        {
            break;
        }
    }
    m_PrebufferedSoundMs = m_bHasAudio ?
        (int)((SDL_AtomicGet(&m_AudioWritten) / m_MixerChannels) * 1000LL / m_MixerRate) : 0;

#if SDL_VERSION_ATLEAST(2, 0, 9)
    m_pDecoderThread = SDL_CreateThreadWithStackSize(&MoviePlayer::DecoderThread, "MovieDecoder", 256 * 1024, this);
#else
    m_pDecoderThread = SDL_CreateThread(&MoviePlayer::DecoderThread, "MovieDecoder", this);
#endif
    if (!m_pDecoderThread)
    {
        LOG_ERROR("Could not start the movie decoder: " + std::string(SDL_GetError()));
        Close();
        return false;
    }

    // Hooked once sound is queued, so the mixer doesn't start on an empty buffer
    if (m_bHasAudio)
    {
        Mix_HookMusic(&MoviePlayerMixAudio, this);
    }

    m_StartTicks = SDL_GetTicks();
    m_OpenMs = m_StartTicks - openStart;
    LOG("Playing movie " + ToStr(movieIndex) + ": " + ToStr((int)m_Width) + "x" + ToStr((int)m_Height) +
        (m_bHalfHeight ? " (half height)" : "") + ", " + ToStr((int)m_FrameCount) + " frames at " +
        ToStr((int)(1000000.0 / m_FrameDurationUs)) + " fps, " + soundInfo + ", opened in " + ToStr((int)m_OpenMs) +
        " ms, " + ToStr(m_PrebufferedSoundMs) + " ms of sound queued");
    return true;
}

void MoviePlayer::Close()
{
    if (m_pDecoderThread)
    {
        SDL_AtomicSet(&m_StopDecoder, 1);
        SDL_WaitThread(m_pDecoderThread, NULL);
        m_pDecoderThread = NULL;
    }
    if (m_bHasAudio)
    {
        // Takes the mixer's lock, so the audio thread is out of MixAudio afterwards
        Mix_HookMusic(NULL, NULL);
        m_bHasAudio = false;
    }
    if (m_pSmk)
    {
        smk_close(m_pSmk);
        m_pSmk = NULL;
    }
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = NULL;
    }
    m_FrameSlots.clear();
    m_AudioRing.clear();
}

void MoviePlayer::Stop(const std::string& reason)
{
    if (!m_pSmk)
    {
        return;
    }

    // The decode times are the decoder thread's; read them once it has stopped
    if (m_pDecoderThread)
    {
        SDL_AtomicSet(&m_StopDecoder, 1);
        SDL_WaitThread(m_pDecoderThread, NULL);
        m_pDecoderThread = NULL;
    }

    // Wall-clock time against the movie's own clock shows slow motion; skipped frames and late updates
    // show a decoder that fell behind; underruns show sound that ran dry
    const double msPerTick = 1000.0 / (double)SDL_GetPerformanceFrequency();
    char stats[320];
    snprintf(stats, sizeof(stats), "at frame %ld of %lu after %.1f s (movie clock %.1f s): %lu frames shown, "
        "%lu skipped, %lu updates waiting for the decoder, decode %.2f ms per frame (worst %.1f), %d sound underruns",
        m_ShownFrame + 1, m_FrameCount, (SDL_GetTicks() - m_StartTicks) / 1000.0, m_ElapsedUs / 1000000.0,
        m_FramesShown, m_FramesSkipped, m_LateUpdates,
        m_FramesTimed > 0 ? m_DecodeTicks * msPerTick / m_FramesTimed : 0.0, m_WorstDecodeTicks * msPerTick,
        SDL_AtomicGet(&m_AudioUnderruns));
    LOG("Movie " + ToStr(m_MovieIndex) + " " + reason + " " + stats);
    Close();
}

int MoviePlayer::DecoderThread(void* pThis)
{
    static_cast<MoviePlayer*>(pThis)->DecodeLoop();
    return 0;
}

void MoviePlayer::DecodeLoop()
{
    while (!SDL_AtomicGet(&m_StopDecoder))
    {
        const int decoded = SDL_AtomicGet(&m_FramesDecoded);
        if ((unsigned long)decoded >= m_FrameCount)
        {
            break;
        }
        if (decoded - SDL_AtomicGet(&m_FramesTaken) >= FRAME_QUEUE_SIZE)
        {
            // Far enough ahead; the main thread takes a frame every 33 or 67 ms
            SDL_Delay(4);
            continue;
        }
        if (!DecodeNextFrame())
        {
            SDL_AtomicSet(&m_DecodeFailed, 1);
            break;
        }
    }
    SDL_AtomicSet(&m_DecoderDone, 1);
}

bool MoviePlayer::DecodeNextFrame()
{
    const int frame = SDL_AtomicGet(&m_FramesDecoded);
    const Uint64 start = SDL_GetPerformanceCounter();
    if (smk_next(m_pSmk) == SMK_ERROR)
    {
        LOG_ERROR("Movie frame " + ToStr(frame) + " could not be decoded");
        return false;
    }
    ConvertFrame(&m_FrameSlots[frame % FRAME_QUEUE_SIZE][0]);
    QueueFrameAudio();

    const Uint64 ticks = SDL_GetPerformanceCounter() - start;
    m_DecodeTicks += ticks;
    m_WorstDecodeTicks = ticks > m_WorstDecodeTicks ? ticks : m_WorstDecodeTicks;
    m_FramesTimed++;

    // Publishes the slot to the main thread (SDL's atomics are full barriers)
    SDL_AtomicSet(&m_FramesDecoded, frame + 1);
    return true;
}

void MoviePlayer::Update(uint32 msDiff)
{
    if (!m_pSmk)
    {
        return;
    }

    // After a long hitch (e.g. the system menu) carry on from where it was instead of racing to catch up
    m_ElapsedUs += msDiff < 500 ? msDiff * 1000.0 : m_FrameDurationUs;

    const long dueFrame = (long)(m_ElapsedUs / m_FrameDurationUs);
    const long decoded = SDL_AtomicGet(&m_FramesDecoded);
    const bool decoderDone = SDL_AtomicGet(&m_DecoderDone) != 0;

    if (dueFrame > m_ShownFrame)
    {
        // The newest frame that is due and decoded; frames in between are dropped
        const long frame = dueFrame < decoded ? dueFrame : decoded - 1;
        if (frame > m_ShownFrame)
        {
            SDL_UpdateTexture(m_pTexture, NULL, &m_FrameSlots[frame % FRAME_QUEUE_SIZE][0], (int)m_Width * 2);
            m_FramesSkipped += frame - m_ShownFrame - 1;
            m_FramesShown++;
            m_ShownFrame = frame;
            SDL_AtomicSet(&m_FramesTaken, (int)frame + 1);
        }
        else if (!decoderDone)
        {
            m_LateUpdates++;
        }
    }

    if (decoderDone && m_ShownFrame + 1 >= decoded && dueFrame >= decoded)
    {
        Stop(SDL_AtomicGet(&m_DecodeFailed) ? "stopped" : "finished");
    }
}

void MoviePlayer::ConvertFrame(uint16_t* pDst)
{
    const unsigned char* pPalette = smk_get_palette(m_pSmk);
    const unsigned char* pFrame = smk_get_video(m_pSmk);
    if (!pPalette || !pFrame)
    {
        return;
    }

    uint16_t palette565[256];
    for (int i = 0; i < 256; i++)
    {
        const unsigned char* c = pPalette + i * 3;
        palette565[i] = (uint16_t)(((c[0] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[2] >> 3));
    }

    // Two texture lines per movie line: the second repeats the first when stretched, and stays
    // black when interlaced (it is never written)
    for (unsigned long y = 0; y < m_Height; y++)
    {
        const unsigned char* pSrc = pFrame + y * m_Width;
        uint16_t* pLine = pDst + (m_bTextureLinesDoubled ? y * 2 : y) * m_Width;
        for (unsigned long x = 0; x < m_Width; x++)
        {
            pLine[x] = palette565[pSrc[x]];
        }
        if (m_bTextureLinesDoubled && !m_bInterlaced)
        {
            memcpy(pLine + m_Width, pLine, m_Width * sizeof(uint16_t));
        }
    }
}

void MoviePlayer::QueueSample(int left, int right)
{
    int written = SDL_AtomicGet(&m_AudioWritten);
    int queued = written - SDL_AtomicGet(&m_AudioRead);
    if (queued + m_MixerChannels > (int)m_AudioRing.size())
    {
        // The mixer stopped taking sound (e.g. paused); drop it rather than block
        return;
    }

    const size_t mask = m_AudioRing.size() - 1;
    if (m_MixerChannels == 1)
    {
        m_AudioRing[(unsigned)written & mask] = (int16_t)(((left + right) / 2) * m_Gain);
    }
    else
    {
        m_AudioRing[(unsigned)written & mask] = (int16_t)(left * m_Gain);
        m_AudioRing[(unsigned)(written + 1) & mask] = (int16_t)(right * m_Gain);
    }
    SDL_AtomicSet(&m_AudioWritten, written + m_MixerChannels);
}

void MoviePlayer::QueueFrameAudio()
{
    if (!m_bHasAudio)
    {
        return;
    }
    const unsigned char* pData = smk_get_audio(m_pSmk, 0);
    unsigned long bytes = smk_get_audio_size(m_pSmk, 0);
    if (!pData || bytes == 0)
    {
        return;
    }

    const unsigned long frames = bytes / ((m_SourceBits / 8) * m_SourceChannels);
    // Resampled to the mixer's rate by taking the nearest source sample: enough for 1997 movie sound
    for (; m_ResamplePos < frames; m_ResamplePos += m_ResampleStep)
    {
        unsigned long index = (unsigned long)m_ResamplePos * m_SourceChannels;
        int left, right;
        if (m_SourceBits == 16)
        {
            const int16_t* pSamples = (const int16_t*)pData;
            left = pSamples[index];
            right = m_SourceChannels == 2 ? pSamples[index + 1] : left;
        }
        else
        {
            left = (pData[index] - 128) << 8;
            right = m_SourceChannels == 2 ? (pData[index + 1] - 128) << 8 : left;
        }
        QueueSample(left, right);
    }
    m_ResamplePos -= frames;
}

// Audio thread, while the movie's hook is installed
void MoviePlayer::MixAudio(Uint8* pStream, int len)
{
    int16_t* pOut = (int16_t*)pStream;
    const int samples = len / 2;
    const int read = SDL_AtomicGet(&m_AudioRead);
    int available = SDL_AtomicGet(&m_AudioWritten) - read;
    int count = available < samples ? available : samples;
    // Running dry before the decoder is done means the sound stuttered
    if (count < samples && !SDL_AtomicGet(&m_DecoderDone))
    {
        SDL_AtomicIncRef(&m_AudioUnderruns);
    }

    const size_t mask = m_AudioRing.size() - 1;
    for (int i = 0; i < count; i++)
    {
        pOut[i] = m_AudioRing[(unsigned)(read + i) & mask];
    }
    // The rest of the buffer is the mixer's silence
    SDL_AtomicSet(&m_AudioRead, read + count);
}

void MoviePlayer::Render(SDL_Renderer* pRenderer, int windowWidth, int windowHeight)
{
    if (!m_pTexture || m_Width == 0 || m_Height == 0 || m_ShownFrame < 0)
    {
        return;
    }

    // Interlaced: shown at the size the original showed it (320x240 movies doubled to 640x480), at
    // 1:1 so the lines stay even. Stretched, or if that doesn't fit: as large as fits, aspect kept
    const int movieHeight = (int)m_TextureHeight;
    const int movieWidth = (int)((m_bInterlaced && !m_bHalfHeight) ? m_Width * 2 : m_Width);

    float scale = 1.0f;
    if (!(m_bInterlaced && movieWidth <= windowWidth && movieHeight <= windowHeight))
    {
        float scaleX = (float)windowWidth / movieWidth;
        float scaleY = (float)windowHeight / movieHeight;
        scale = scaleX < scaleY ? scaleX : scaleY;
    }

    SDL_Rect dst;
    dst.w = (int)(movieWidth * scale);
    dst.h = (int)(movieHeight * scale);
    dst.x = (windowWidth - dst.w) / 2;
    dst.y = (windowHeight - dst.h) / 2;
    SDL_RenderCopy(pRenderer, m_pTexture, NULL, &dst);
}
