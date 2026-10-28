#include "BaseGameApp.h"
#include "MainLoop.h"
#include <fstream>

#ifndef __vita__
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef __vita__
#include <cstdio>
#include <sys/stat.h>

// app0: (the installed VPK) is read-only, so the writable files are seeded from it on first start
static void CopyFileIfMissing(const std::string& srcPath, const std::string& dstPath)
{
    if (TryToFindFile(dstPath.c_str()))
    {
        return;
    }

    std::ifstream src(srcPath, std::ios::binary);
    if (!src.is_open())
    {
        LOG_ERROR("Failed to open " + srcPath);
        return;
    }

    bool copied = false;
    {
        std::ofstream dst(dstPath, std::ios::binary);
        copied = dst.is_open() && (dst << src.rdbuf()) && dst.flush();
    }
    if (!copied)
    {
        // Don't leave a partial file behind, it would stop the next start from retrying
        remove(dstPath.c_str());
        LOG_ERROR("Failed to copy " + srcPath + " to " + dstPath);
        return;
    }

    LOG("Copied " + srcPath + " to " + dstPath);
}
#endif

int RunGameEngine(int argc, char** argv)
{
    if (SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH) != 0)
    {
        LOG_WARNING("Failed to set high priority class to this process");
    }

    std::string userDirectory = "";

#ifdef __vita__
    userDirectory = "ux0:/data/openclaw/";

    mkdir(userDirectory.c_str(), 0777);
    CopyFileIfMissing("app0:/config.xml", userDirectory + "config.xml");
    CopyFileIfMissing("app0:/SAVES.XML", userDirectory + "SAVES.XML");
#else
    const char* homedir;

    if ((homedir = getenv("HOME")) == NULL) 
    {
        homedir = getpwuid(getuid())->pw_dir;
    }
    assert(homedir != NULL);

    userDirectory = std::string(homedir) + "/.config/openclaw/";

#endif

    LOG("Looking for: " + userDirectory + "config.xml");

#ifndef __vita__
    // Temporary hack - always prefer config in the same folder as binary to default config
    // (not on Vita, where the working directory is the read-only app0:)
    if (TryToFindFile("config.xml"))
    {
        // File was found in a working directory
        userDirectory = "";
    }
#endif

    LOG("Expecting config.xml in path: " + userDirectory + "config.xml");

    // Load options
    if (!g_pApp->LoadGameOptions(std::string(userDirectory + "config.xml").c_str()))
    {
        LOG_ERROR("Could not load game options. Exiting.");
        return -1;
    }

    g_pApp->GetGameConfig()->userDirectory = userDirectory;

    std::string savesFilePath = g_pApp->GetGameConfig()->userDirectory + g_pApp->GetGameConfig()->savesFile;
    LOG("Loaded with:\n\tConfig File: " + userDirectory + "config.xml" + "\n\tSaves File: " + savesFilePath);

    // Initialize game instance
    if (!g_pApp->Initialize(argc, argv))
    {
        LOG_ERROR("Failed to initialize. Exiting.");
        return -1;
    }

    // Run the game
    return g_pApp->Run();
}

bool TryToFindFile(const char *path) {
    {
        std::ifstream f(path);
        if (f.good()) {
            return true;
        }
    }
    return false;
}
