#include "FrameStats.h"

#include <cstdio>

namespace FrameStats
{
    namespace
    {
        const double REPORT_INTERVAL_SECONDS = 2.0;
        const char* SECTION_NAMES[Section_Count] = { "update", "physics", "render", "present" };

        bool g_Enabled = false;

        Uint64 g_IntervalStart = 0;
        Uint64 g_FrameStart = 0;
        Uint64 g_WorstFrame = 0;
        Uint64 g_SectionTotals[Section_Count] = {};
        Uint32 g_FrameCount = 0;
        Uint32 g_DrawCalls = 0;

        double ToMs(Uint64 counterTicks)
        {
            return counterTicks * 1000.0 / SDL_GetPerformanceFrequency();
        }

        void Reset(Uint64 now)
        {
            g_IntervalStart = now;
            g_WorstFrame = 0;
            g_FrameCount = 0;
            g_DrawCalls = 0;
            for (Uint64& total : g_SectionTotals)
            {
                total = 0;
            }
        }

        void Report(Uint64 now)
        {
            double frames = g_FrameCount;
            double elapsedMs = ToMs(now - g_IntervalStart);

            char line[256];
            int length = snprintf(line, sizeof(line), "[FrameStats] %.1f fps | frame %.1f ms (worst %.1f)",
                frames * 1000.0 / elapsedMs, elapsedMs / frames, ToMs(g_WorstFrame));
            for (int i = 0; i < Section_Count && length < (int)sizeof(line); i++)
            {
                length += snprintf(line + length, sizeof(line) - length, " | %s %.1f",
                    SECTION_NAMES[i], ToMs(g_SectionTotals[i]) / frames);
            }
            if (length < (int)sizeof(line))
            {
                snprintf(line + length, sizeof(line) - length, " | draws %.0f", g_DrawCalls / frames);
            }

            SDL_Log("%s", line);
        }
    }

    void SetEnabled(bool enabled)
    {
        g_Enabled = enabled;
        g_FrameStart = 0;
        Reset(SDL_GetPerformanceCounter());
    }

    bool IsEnabled()
    {
        return g_Enabled;
    }

    void AddTime(Section section, Uint64 startCounter)
    {
        g_SectionTotals[section] += SDL_GetPerformanceCounter() - startCounter;
    }

    void AddDrawCall()
    {
        g_DrawCalls++;
    }

    void EndFrame()
    {
        if (!g_Enabled)
        {
            return;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        if (g_FrameStart != 0)
        {
            Uint64 frameTime = now - g_FrameStart;
            if (frameTime > g_WorstFrame)
            {
                g_WorstFrame = frameTime;
            }
            g_FrameCount++;
        }
        else
        {
            // First frame: start the interval here so start-up time isn't counted
            Reset(now);
        }
        g_FrameStart = now;

        if (g_FrameCount > 0 && ToMs(now - g_IntervalStart) >= REPORT_INTERVAL_SECONDS * 1000.0)
        {
            Report(now);
            Reset(now);
        }
    }
}
