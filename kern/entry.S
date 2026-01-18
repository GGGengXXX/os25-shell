#include <asm/asm.h>
#include <stackframe.h>

// 用户态发生 tlb_miss 异常时
// 跳转到 0x80000000
.section .text.tlb_miss_entry
tlb_miss_entry:
	j       exc_gen_entry


// CPU 发生异常时
// 跳转到 0x8000 0180
.section .text.exc_gen_entry
exc_gen_entry:
	// 将寄存器中的内容保存到 内核态的栈中
	SAVE_ALL
	/*
	* Note: When EXL is set or UM is unset, the processor is in kernel mode.
	* When EXL is set, the value of EPC is not updated when a new exception occurs.
	* To keep the processor in kernel mode and enable exception reentrancy,
	* we unset UM and EXL, and unset IE to globally disable interrupts.
	*/
	// 修改了 CP0_STATUS 
	// 清除 UM EXL IE 使得处理器处于内核态，关闭中断且允许嵌套异常
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
	mtc0    t0, CP0_STATUS
/* Exercise 3.9: Your code here. */
	mfc0    t0, CP0_CAUSE
	andi    t0, 0x7c   // 取到其中的 6 ~ 2 位
	// t0 得到异常原因的编号
	// 在 exception_handlers 中查找对应的异常处理程序地址
	lw      t0, exception_handlers(t0)
	jr      t0
