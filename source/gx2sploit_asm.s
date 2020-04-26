    .globl set_semaphore_phys
set_semaphore_phys:
    mflr 0
    stwu 1, -0x20(1)
    stw 31, 0x1C(1)
    stw 30, 0x18(1)
    stw 0, 0x24(1)

    mtctr 3
    mr 3, 4
    mr 30, 5
    li 31, 1
    bctr

    .globl Register
Register:
    li 0,0x3200
    sc
    blr

    .globl CopyToSaveArea
CopyToSaveArea:
    li 0,0x4800
    sc
    blr

    .globl SC_0x36_SETBATS
SC_0x36_SETBATS:
    li 0,0x3600
    sc
    blr

    .globl SC_0x09_SETIBAT0
SC_0x09_SETIBAT0:
    li 0, 0x0900
    sc
    blr

    .global SCKernelCopyData
SCKernelCopyData:
	// Disable data address translation
	mfmsr %r6
	li %r7, 0x10
	andc %r6, %r6, %r7
	mtmsr %r6

	// Copy data
	addi %r3, %r3, -1
	addi %r4, %r4, -1
	mtctr %r5
SCKernelCopyData_loop:
	lbzu %r5, 1(%r4)
	stbu %r5, 1(%r3)
	bdnz SCKernelCopyData_loop

	// Enable data address translation
	ori %r6, %r6, 0x10
	mtmsr %r6
blr

.global SC_KernelCopyData
SC_KernelCopyData:
	li %r0, 0x2500
	sc
blr
