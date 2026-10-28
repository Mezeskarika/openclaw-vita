#include "Profilers.h"
#include "../SharedDefines.h"

// For memory profiler

#include <string>
#include <stdint.h>
#include <iostream>
#include <SDL2/SDL.h>

CPU_PROFILER::CPU_PROFILER(std::string tag)
{
    m_Tag = tag;

    m_StartingTime = SDL_GetPerformanceCounter();
}

CPU_PROFILER::~CPU_PROFILER()
{
    uint64_t now = SDL_GetPerformanceCounter();

    if (!m_Tag.empty())
    {
        std::string s("[" + m_Tag + "]: Elapsed microseconds: " + ToStr((unsigned long)(((now - m_StartingTime) * 1000000) / SDL_GetPerformanceFrequency())));
        std::cout << s << std::endl;
    }
    else
    {
        std::string s("Elapsed microseconds: " + ToStr((unsigned long)(((now - m_StartingTime) * 1000000) / SDL_GetPerformanceFrequency())));
        std::cout << s << std::endl;
    }
}


MEMORY_PROFILER::MEMORY_PROFILER(std::string tag)
{
    m_Tag = tag;
    LOG_ERROR("Memory profiler not supported on this platform !");
}

MEMORY_PROFILER::~MEMORY_PROFILER()
{
}