#include <asm/asm.h>
#include <mmu.h>

.text
EXPORT(_start)
// 暴露入口点(entry point) 使其对全局可见
.set at
.set reorder
/* Lab 1 Key Code "enter-kernel" */
	/* clear .bss segment */
	la      v0, bss_start
	la      v1, bss_end
clear_bss_loop:
	beq     v0, v1, clear_bss_done
	sb      zero, 0(v0)
	addiu   v0, v0, 1
	j       clear_bss_loop
/* End of Key Code "enter-kernel" */

clear_bss_done:
	/* disable interrupts */
	mtc0    zero, CP0_STATUS
	// mtc0 Move to Coprocessor 0
	// CP0_STATUS 是 CP0 中的 Status 寄存器，用于控制CPU状态

	/* hint: you can refer to the memory layout in include/mmu.h */
	/* set up the kernel stack */
	/* Exercise 1.3: Your code here. (1/2) */
	li sp,  KSTACKTOP
	/* jump to mips_init */
	/* Exercise 1.3: Your code here. (2/2) */
	j mips_init
