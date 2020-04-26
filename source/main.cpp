#include <stdio.h>
#include <string.h>
#include <vector>

#include <coreinit/ios.h>
#include <coreinit/time.h>

#include <coreinit/systeminfo.h>
#include <coreinit/foreground.h>

#include <nsysnet/socket.h>

#include <proc_ui/procui.h>
#include <coreinit/thread.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <sysapp/launch.h>
#include <coreinit/exit.h>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <vpad/input.h>
#include "utils/logger.h"
#include "utils/utils.h"
#include "ElfUtils.h"
#include "ios_exploit.h"

#include "gx2sploit.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

bool CheckRunning() {

    switch(ProcUIProcessMessages(true)) {
    case PROCUI_STATUS_EXITING: {
        return false;
    }
    case PROCUI_STATUS_RELEASE_FOREGROUND: {
        ProcUIDrawDoneRelease();
        break;
    }
    case PROCUI_STATUS_IN_FOREGROUND: {
        break;
    }
    case PROCUI_STATUS_IN_BACKGROUND:
    default:
        break;
    }
    return true;
}

extern "C" uint64_t _SYSGetSystemApplicationTitleId(int);

int main(int argc, char **argv) {
    WHBLogUdpInit();

    WHBLogPrintf("Hello!");

    VPADReadError err;
    VPADStatus vpad_data;
    VPADRead(VPAD_CHAN_0, &vpad_data, 1, &err);

    uint32_t btn = vpad_data.hold | vpad_data.trigger;
    bool loadWithoutHacks = false;
    bool kernelDone = false;
    if((btn & VPAD_BUTTON_ZR) == VPAD_BUTTON_ZR) {
        loadWithoutHacks = true;
    }
    if((btn & VPAD_BUTTON_ZL) == VPAD_BUTTON_ZL) {
        // In case that fopen check is not working...
        WHBLogPrintf("Force kernel exploit");
        kernelDone = true;
        DoKernelExploit();
    }

    if(!kernelDone) {
        if(fopen("fs:/vol/external01/wiiu/payload.elf", "r") != NULL) {
            WHBLogPrintf("We need the kernel exploit to load the payload");
            DoKernelExploit();
        }
    }

    if(!loadWithoutHacks) {
        uint32_t entryPoint = load_loader_elf_from_sd(0, "wiiu/payload.elf");
        if(entryPoint != 0) {
            WHBLogPrintf("New entrypoint: %08X", entryPoint);
            int res = ((int (*)(int, char **))entryPoint)(argc, argv);
            if(res > 0) {
                WHBLogPrintf("Returning...");
                WHBLogUdpDeinit();
                return 0;
            }
        } else {
            loadWithoutHacks = true;
        }
    }
    ProcUIInit(OSSavesDone_ReadyToRelease);
    DEBUG_FUNCTION_LINE("ProcUIInit done");

    if(loadWithoutHacks) {
        DEBUG_FUNCTION_LINE("Load system menu");
        // Restore the default title id to the normal wii u menu.
        unsigned long long sysmenuIdUll = _SYSGetSystemApplicationTitleId(0);
        memcpy((void*)0xF417FFF0, &sysmenuIdUll, 8);
        DCStoreRange((void*)0xF417FFF0,0x8);

        DEBUG_FUNCTION_LINE("THIS IS A TEST %016llX\n",sysmenuIdUll);

        ExecuteIOSExploit();
        SYSLaunchMenu();
    }

    while(CheckRunning()) {
        // wait.
        OSSleepTicks(OSMillisecondsToTicks(100));
    }
    ProcUIShutdown();

    DEBUG_FUNCTION_LINE("Bye!");
    WHBLogUdpDeinit();

    return 0;
}
