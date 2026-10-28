//#define SDL_MAIN_HANDLED
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "Engine/GameApp/MainLoop.h"
#include "ClawGameApp.h"

#ifdef ANDROID
#include <jni.h>
#endif

ClawGameApp testApp;

#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>

// Read by the VitaSDK runtime at startup. Sized explicitly rather than relying on the SDK defaults;
// the resource cache alone may take ResourceCacheSize MB. Tune both together.
extern "C"
{
    int _newlib_heap_size_user = 128 * 1024 * 1024;
    unsigned int sceUserMainThreadStackSize = 1 * 1024 * 1024;
}
#endif

int main(int argc, char* argv[]);

#ifdef ANDROID
extern "C" void SDL_Android_Init(JNIEnv* env, jclass cls);

// Start up the SDL app
extern "C" void Java_org_libsdl_app_SDLActivity_nativeInit( \
    JNIEnv* env, jclass cls, jobject obj) {
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env, cls);

    SDL_SetMainReady();

    /* Run the application code! */
    int status;
    char *argv[2];
    argv[0] = strdup("SDL_app");
    argv[1] = NULL;
    status = main(1, argv);

/* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
//exit(status);
}
#endif //ANDROID

int main(int argc, char* argv[])
{
#ifdef _CRTDBG_MAP_ALLOC
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
//    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
//    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#ifdef __vita__
    // Apps start at 333 MHz; run the CPU at its full 444 MHz and raise the GPU clocks to match.
    // Real hardware only - Vita3K ignores these.
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // End the process explicitly instead of running global destructors, which would tear
    // down objects that other threads may still use. The system reclaims everything.
    sceKernelExitProcess(RunGameEngine(argc, argv));
    return 0;
#else
    return RunGameEngine(argc, argv);
#endif
}
