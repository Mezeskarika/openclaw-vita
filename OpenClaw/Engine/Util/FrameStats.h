#ifndef __FRAMESTATS_H__
#define __FRAMESTATS_H__

#include <SDL2/SDL.h>

// Measures where the frame time goes and logs a summary every few seconds.
// Turned on with <LogFrameStats> in the GlobalOptions config section.
namespace FrameStats
{
    enum Section
    {
        Section_Update,     // Event queue + game logic, physics included
        Section_Physics,    // Box2D step only (part of Section_Update)
        Section_Render,     // Walking the scene and queueing draws with SDL
        Section_Present,    // SDL_RenderPresent: SDL executes the queued draws, then waits for the GPU / vsync
        Section_Count
    };

    void SetEnabled(bool enabled);
    bool IsEnabled();

    void AddTime(Section section, Uint64 startCounter);
    void AddDrawCall();
    void EndFrame();

    // Adds the time from its construction to its destruction to a section
    class ScopedTimer
    {
    public:
        explicit ScopedTimer(Section section)
            : m_Section(section), m_StartCounter(IsEnabled() ? SDL_GetPerformanceCounter() : 0) { }
        ~ScopedTimer() { if (m_StartCounter) { AddTime(m_Section, m_StartCounter); } }

    private:
        Section m_Section;
        Uint64 m_StartCounter;
    };
}

#endif
