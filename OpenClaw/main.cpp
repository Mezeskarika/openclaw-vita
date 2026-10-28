#include "Engine/GameApp/MainLoop.h"
#include "ClawGameApp.h"

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

int main(int argc, char* argv[])
{
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
