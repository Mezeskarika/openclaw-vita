#include "Logger.h"

#include <stdio.h>
#include <string.h>

namespace
{
    FILE* g_pLogFile = NULL;
    SDL_mutex* g_pLogMutex = NULL;

    const char* PriorityName(SDL_LogPriority priority)
    {
        switch (priority)
        {
            case SDL_LOG_PRIORITY_VERBOSE: return "VERBOSE";
            case SDL_LOG_PRIORITY_DEBUG: return "DEBUG";
            case SDL_LOG_PRIORITY_WARN: return "WARN";
            case SDL_LOG_PRIORITY_ERROR: return "ERROR";
            case SDL_LOG_PRIORITY_CRITICAL: return "CRITICAL";
            default: return "INFO";
        }
    }

    void LogOutput(void* userdata, int category, SDL_LogPriority priority, const char* message)
    {
        // The messages carry their own line break
        size_t length = strlen(message);
        while (length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r'))
        {
            length--;
        }

        Uint32 ms = SDL_GetTicks();
        if (g_pLogMutex)
        {
            SDL_LockMutex(g_pLogMutex);
        }
        FILE* pOut = g_pLogFile ? g_pLogFile : stderr;
        fprintf(pOut, "%5u.%03u %s: %.*s\n", ms / 1000, ms % 1000, PriorityName(priority), (int)length, message);
        fflush(pOut);
        if (g_pLogMutex)
        {
            SDL_UnlockMutex(g_pLogMutex);
        }
    }
}

namespace Logger
{
    void InstallLogSink(const char* filePath)
    {
        g_pLogMutex = SDL_CreateMutex();
        if (filePath)
        {
            g_pLogFile = fopen(filePath, "a");
        }
        SDL_LogSetOutputFunction(&LogOutput, NULL);
    }

    void GetOutputString(std::string& outOutputBuffer, const std::string& tag, const std::string& message, const char* funcName, const char* sourceFile, unsigned int lineNum)
    {
        if (funcName != NULL && sourceFile != NULL)
        {
            outOutputBuffer = "[" + std::string(sourceFile) + ", " + funcName + "] ";
        }
        else if (funcName != NULL)
        {
            outOutputBuffer = "[" + std::string(funcName) + "] ";
        }
        else if (sourceFile != NULL)
        {
            outOutputBuffer = "[" + std::string(sourceFile) + "] ";
        }

        outOutputBuffer += message;

        /*if (lineNum != 0)
        {
            outOutputBuffer += "\nLine: ";
            char lineNumBuffer[11];
            memset(lineNumBuffer, 0, sizeof(char));
            outOutputBuffer += _itoa_s(lineNum, lineNumBuffer, 10);
        }*/

        outOutputBuffer += "\n";
    }
}