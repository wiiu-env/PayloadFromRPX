/***************************************************************************
 * Copyright (C) 2016
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include "kernel_patches.h"
#include "elf_patcher.h"
#include "types.h"
#include "utils.h"

extern void __KERNEL_CODE_START(void);

extern void __KERNEL_CODE_END(void);

void kernel_launch_ios(u32 launch_address, u32 L, u32 C, u32 H) {
    void (*kernel_launch_bootrom)(u32 launch_address, u32 L, u32 C, u32 H) = (void *) 0x0812A050;

    if (*(u32 *) (launch_address - 0x300 + 0x1AC) == 0x00DFD000) {
        int level                     = disable_interrupts();
        unsigned int control_register = disable_mmu();

        u32 ios_elf_start = launch_address + 0x804 - 0x300;

        //! try to keep the order of virt. addresses to reduce the memmove amount
        kernel_run_patches(ios_elf_start);

        restore_mmu(control_register);
        enable_interrupts(level);
    }

    kernel_launch_bootrom(launch_address, L, C, H);
}

void kernel_run_patches(u32 ios_elf_start) {
    section_write(ios_elf_start, (u32) __KERNEL_CODE_START, __KERNEL_CODE_START, __KERNEL_CODE_END - __KERNEL_CODE_START);
    section_write_word(ios_elf_start, 0x0812A120, ARM_BL(0x0812A120, kernel_launch_ios));

    // update check
    section_write_word(ios_elf_start, 0xe22830e0, 0x00000000);
    section_write_word(ios_elf_start, 0xe22b2a78, 0x00000000);
    section_write_word(ios_elf_start, 0xe204fb68, 0xe3a00000);
}
