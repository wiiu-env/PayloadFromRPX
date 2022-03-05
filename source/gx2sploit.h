#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define KERN_SYSCALL_TBL_1 0xFFE84C70 // unknown
#define KERN_SYSCALL_TBL_2 0xFFE85070 // works with games
#define KERN_SYSCALL_TBL_3 0xFFE85470 // works with loader
#define KERN_SYSCALL_TBL_4 0xFFEAAA60 // works with home menu
#define KERN_SYSCALL_TBL_5 0xFFEAAE60 // works with browser (previously KERN_SYSCALL_TBL)

int DoKernelExploit(void);

void KernelWrite(uint32_t addr, const void *data, uint32_t length);
void kern_write(const void *addr, uint32_t value);

extern int32_t Register(char *driver_name, uint32_t name_length, void *buf1, void *buf2);
extern void CopyToSaveArea(char *driver_name, uint32_t name_length, void *buffer, uint32_t length);
extern void set_semaphore_phys(uint32_t set_semaphore, uint32_t kpaddr, uint32_t gx2data_addr);

#ifdef __cplusplus
}
#endif
