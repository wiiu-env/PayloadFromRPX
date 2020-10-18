#include <stdio.h>
#include <string.h>
#include <coreinit/time.h>

#include <coreinit/foreground.h>

#include <proc_ui/procui.h>
#include <coreinit/thread.h>
#include <coreinit/screen.h>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>
#include <coreinit/cache.h>
#include <vpad/input.h>
#include <string>
#include "utils/logger.h"
#include "ElfUtils.h"
#include "ios_exploit.h"

#include "gx2sploit.h"

void SplashScreen(int32_t durationInMs);

bool CheckRunning() {

    switch (ProcUIProcessMessages(true)) {
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

int main(int argc, char **argv) {
    WHBLogUdpInit();

    DEBUG_FUNCTION_LINE("Hello!");

    VPADReadError err;
    VPADStatus vpad_data;
    VPADRead(VPAD_CHAN_0, &vpad_data, 1, &err);

    uint32_t btn = vpad_data.hold | vpad_data.trigger;
    bool loadWithoutHacks = false;
    bool kernelDone = false;
    bool skipKernel = false;

    if ((btn & VPAD_BUTTON_R) == VPAD_BUTTON_R) {
        skipKernel = true;
        loadWithoutHacks = true;
    }
    if ((btn & VPAD_BUTTON_ZR) == VPAD_BUTTON_ZR) {
        loadWithoutHacks = true;
    }
    if ((btn & VPAD_BUTTON_ZL) == VPAD_BUTTON_ZL) {
        // In case that fopen check is not working...
        DEBUG_FUNCTION_LINE("Force kernel exploit");
        kernelDone = true;
        DoKernelExploit();
    }

    if (!kernelDone && !skipKernel) {
        if (fopen("fs:/vol/external01/wiiu/payload.elf", "r") != NULL) {
            DEBUG_FUNCTION_LINE("We need the kernel exploit to load the payload");
            DoKernelExploit();
        }
    }

    if (!loadWithoutHacks) {
        uint32_t entryPoint = load_loader_elf_from_sd(0, "wiiu/payload.elf");
        if (entryPoint != 0) {
            DEBUG_FUNCTION_LINE("New entrypoint at %08X", entryPoint);
            int res = ((int (*)(int, char **)) entryPoint)(argc, argv);
            if (res > 0) {
                DEBUG_FUNCTION_LINE("Returning result of payload");
                WHBLogUdpDeinit();
                return 0;
            } else {
                loadWithoutHacks = true;
            }
        } else {
            SplashScreen(1000);
            loadWithoutHacks = true;
        }
    }
    ProcUIInit(OSSavesDone_ReadyToRelease);
    DEBUG_FUNCTION_LINE("ProcUIInit done");

    if (loadWithoutHacks) {
        DEBUG_FUNCTION_LINE("Load Wii U Menu");
        // Restore the default title id to the normal Wii U Menu.
        unsigned long long sysmenuIdUll = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_HOME_MENU);
        memcpy((void *) 0xF417FFF0, &sysmenuIdUll, 8);
        DCStoreRange((void *) 0xF417FFF0, 0x8);

        DEBUG_FUNCTION_LINE("Forcing start of title: %016llX", sysmenuIdUll);

        ExecuteIOSExploit();
        SYSLaunchMenu();
    }

    while (CheckRunning()) {
        // wait.
        OSSleepTicks(OSMillisecondsToTicks(100));
    }
    ProcUIShutdown();

    DEBUG_FUNCTION_LINE("Exiting.");
    WHBLogUdpDeinit();

    return 0;
}


void SplashScreen(int32_t durationInMs) {
    int32_t screen_buf0_size = 0;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(SCREEN_TV);
    OSScreenSetBufferEx(SCREEN_TV, (void *) 0xF4000000);
    OSScreenSetBufferEx(SCREEN_DRC, (void *) (0xF4000000 + screen_buf0_size));

    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);

    // Clear screens
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    std::string message1 = "Failed to load sd:/wiiu/payload.elf";
    std::string message2 = "Starting the console without any modifcations.";

    OSScreenPutFontEx(SCREEN_TV, 0, 0, message1.c_str());
    OSScreenPutFontEx(SCREEN_DRC, 0, 0, message1.c_str());

    OSScreenPutFontEx(SCREEN_TV, 0, 1, message2.c_str());
    OSScreenPutFontEx(SCREEN_DRC, 0, 1, message2.c_str());

    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    OSSleepTicks(OSMillisecondsToTicks(durationInMs));
}
