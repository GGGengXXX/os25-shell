#include <env.h>
#include <io.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>
#include <sched.h>
#include <syscall.h>

extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int c) {
	printcharc((char)c);
	return;
}

/* Overview:
 * 	This function is used to print a string of bytes on screen.
 *
 * Pre-Condition:
 * 	`s` is base address of the string, and `num` is length of the string.
 */
int sys_print_cons(const void *s, u_int num) {
	if (((u_int)s + num) > UTOP || ((u_int)s) >= UTOP || (s > s + num)) {
		return -E_INVAL;
	}
	u_int i;
	for (i = 0; i < num; i++) {
		printcharc(((char *)s)[i]);
	}
	return 0;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void) {
	return curenv->env_id;
}

/* Overview:
 *   Give up remaining CPU time slice for 'curenv'.
 *   放弃当前进程剩余的 CPU 时间片
 *
 * Post-Condition:
 *   Another env is scheduled.
 *
 * Hint:
 *   This function will never return.
 */
// void sys_yield(void);
void __attribute__((noreturn)) sys_yield(void) {
	// Hint: Just use 'schedule' with 'yield' set.
	/* Exercise 4.7: Your code here. */
	schedule(1);
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *  用来销毁当前的进程
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *  envid 必须是一个进程id
 * 要么是当前进程的子进程，要么是当前进程
 *
 * Post-Condition:
 *  Returns 0 on success.
 *  Returns the original error if underlying calls fail.
 */
int sys_env_destroy(u_int envid) {
	struct Env *e;
	try(envid2env(envid, &e, 1));

//	printk("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 *   Register the entry of user space TLB Mod handler of 'envid'.
 *
 * Post-Condition:
 *   The 'envid''s TLB Mod exception handler entry will be set to 'func'.
 *   Returns 0 on success.
 *   Returns the original error if underlying calls fail.
 */
/**
 * 注册 envid 用户空间的 TLB Mod 处理程序入口

 * envid 对应的进程 TLB Mod 异常处理程序入口将被设置为 func
*/
int sys_set_tlb_mod_entry(u_int envid, u_int func) {
	struct Env *env;

	/* Step 1: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.12: Your code here. (1/2) */
	try(envid2env(envid, &env, 1));
	/* Step 2: Set its 'env_user_tlb_mod_entry' to 'func'. */
	/* Exercise 4.12: Your code here. (2/2) */
	env->env_user_tlb_mod_entry = func;

	return 0;
}

/* Overview:
 *   Check 'va' is illegal or not, according to include/mmu.h
 */
/*
	虚拟地址介于 UTEMP 与 UTOP 之间
	[0x003f e000,  0x7f40 0000)
 */
static inline int is_illegal_va(u_long va) {
	return va < UTEMP || va >= UTOP;
}

static inline int is_illegal_va_range(u_long va, u_int len) {
	if (len == 0) {
		return 0;
	}
	return va + len < va || va < UTEMP || va + len > UTOP;
}

/* Overview:
 *   Allocate a physical page and map 'va' to it with 'perm' in the address space of 'envid'.
 *   If 'va' is already mapped, that original page is sliently unmapped.
 *   'envid2env' should be used with 'checkperm' set, like in most syscalls, to ensure the target is
 * either the caller or its child.
 * 申请一个物理页 根据envid找到对应env
 * 把env的pgdir下的va映射到该物理页 Pte entry权限位为 perm
 * va 如果已经有了映射，新映射会覆盖旧映射
 * 使用 checkperm 权限位检查  要操作的 env
 * 要么是当前进程， 要么是当前进程的子进程
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal (should be checked using 'is_illegal_va').
 *   Return the original error: underlying calls fail (you can use 'try' macro).
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_alloc', 'page_insert', 'try' (macro)
 */
int sys_mem_alloc(u_int envid, u_int va, u_int perm) {
	struct Env *env;
	struct Page *pp;
	/*
		is_illegal_va  va < UTEMP || va >= USTACKTOP
	 */
	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* Exercise 4.4: Your code here. (1/3) */
	/*
	检查 va 在 [UTEMP(0x003f e000) , UTOP/UENVS(0x7f400000) )
	 */
	if(is_illegal_va(va))return -E_INVAL;

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Hint: **Always** validate the permission in syscalls! */
	/* Exercise 4.4: Your code here. (2/3) */
	try(envid2env(envid,&env,1));
	/* Step 3: Allocate a physical page using 'page_alloc'. */
	/* Exercise 4.4: Your code here. (3/3) */
	try(page_alloc(&pp));
	/* Step 4: Map the allocated page at 'va' with permission 'perm' using 'page_insert'. */
	return page_insert(env->env_pgdir, env->env_asid, pp, va, perm);
}

/* Overview:
 *   Find the physical page mapped at 'srcva' in the address space of env 'srcid', and map 'dstid''s
 *   'dstva' to it with 'perm'.
 *
 *  找到虚拟地址 srcva 在进程srcid中 映射的物理页
 *  把 进程dstid的虚拟地址dstva 也映射到这个物理页中
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'srcid' or 'dstid'.
 *   Return -E_INVAL: 'srcva' or 'dstva' is illegal, or 'srcva' is unmapped in 'srcid'.
 *   Return the original error: underlying calls fail.
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_lookup', 'page_insert'
 */
int sys_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm) {
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *pp;

	/* Step 1: Check if 'srcva' and 'dstva' are legal user virtual addresses using
	 * 'is_illegal_va'. */
	/* Exercise 4.5: Your code here. (1/4) */
	if(is_illegal_va(srcva) || is_illegal_va(dstva))return -E_INVAL;
	/* Step 2: Convert the 'srcid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (2/4) */
	try(envid2env(srcid,&srcenv,1));
	/* Step 3: Convert the 'dstid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (3/4) */
	try(envid2env(dstid,&dstenv,1));
	/* Step 4: Find the physical page mapped at 'srcva' in the address space of 'srcid'. */
	/* Return -E_INVAL if 'srcva' is not mapped. */
	/* Exercise 4.5: Your code here. (4/4) */
	pp = page_lookup(srcenv->env_pgdir,srcva,NULL);
	if(pp == NULL)return -E_INVAL;
	/* Step 5: Map the physical page at 'dstva' in the address space of 'dstid'. */
	return page_insert(dstenv->env_pgdir, dstenv->env_asid, pp, dstva, perm);
}

/* Overview:
 *   Unmap the physical page mapped at 'va' in the address space of 'envid'.
 *   If no physical page is mapped there, this function silently succeeds.
 *
 *  取消envid 的进程地址空间中 
 *  虚拟地址 va 到物理页的映射
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal.
 *   Return the original error when underlying calls fail.
 */
int sys_mem_unmap(u_int envid, u_int va) {
	struct Env *e;

	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* Exercise 4.6: Your code here. (1/2) */
	if(is_illegal_va(va))
	{
		return -E_INVAL;
	}
	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.6: Your code here. (2/2) */
	try(envid2env(envid, &e, 1));
	/* Step 3: Unmap the physical page at 'va' in the address space of 'envid'. */
	page_remove(e->env_pgdir, e->env_asid, va);
	return 0;
}

/* Overview:
 *   Allocate a new env as a child of 'curenv'.
 *  分配一个 env 作为当前进程的子进程
 *
 * Post-Condition:
 *   Returns the child's envid on success, and
 *   - The new env's 'env_tf' is copied from the kernel stack, except for $v0 set to 0 to indicate
 *     the return value in child.
 *   - The new env's 'env_status' is set to 'ENV_NOT_RUNNABLE'.
 *   - The new env's 'env_pri' is copied from 'curenv'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   This syscall works as an essential step in user-space 'fork' and 'spawn'.
 */
int sys_exofork(void) {
	struct Env *e;

	/* Step 1: Allocate a new env using 'env_alloc'. */
	/* 申请一个新的进程控制块 */
	/* Exercise 4.9: Your code here. (1/4) */
	try(env_alloc(&e,curenv->env_id));
	/* Step 2: Copy the current Trapframe below 'KSTACKTOP' to the new env's 'env_tf'. */
	/* 将KSTACKTOP 下面的当前陷入帧复制到新进程的陷入帧中 */
	/* Exercise 4.9: Your code here. (2/4) */
	e->env_tf = *((struct Trapframe *)KSTACKTOP - 1);
	/* Step 3: Set the new env's 'env_tf.regs[2]' to 0 to indicate the return value in child. */
	/* 将新进程的 $v0 寄存器设置为 0 用以标识子进程 */
	/* Exercise 4.9: Your code here. (3/4) */
	e->env_tf.regs[2] = 0;
	/* Step 4: Set up the new env's 'env_status' and 'env_pri'.  */
	/* 设置新进程的进程状态和进程优先级 */
	/* Exercise 4.9: Your code here. (4/4) */
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri;

	e->env_cnt = 0;

	strcpy(e->r_path, curenv->r_path);

	for(int i = 0;i < curenv->env_cnt;i++)
	{
		if(curenv->env_vars[i].is_export)
		{
			e->env_vars[e->env_cnt++] = curenv->env_vars[i];
		}
	}


	return e->env_id;
}

/* Overview:
 *   Set 'envid''s 'env_status' to 'status' and update 'env_sched_list'.
 *
 *  设置env_status 并且更新 env_sched_list
 * Post-Condition:
 *   Returns 0 on success.
 *   Returns -E_INVAL if 'status' is neither 'ENV_RUNNABLE' nor 'ENV_NOT_RUNNABLE'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   The invariant that 'env_sched_list' contains and only contains all runnable envs should be
 *   maintained.
 */
int sys_set_env_status(u_int envid, u_int status) {
	struct Env *env;

	/* Step 1: Check if 'status' is valid. */
	/* Exercise 4.14: Your code here. (1/3) */
	if(status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
	{
		return -E_INVAL;
	}
	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.14: Your code here. (2/3) */
	try(envid2env(envid, &env,1));
	/* Step 3: Update 'env_sched_list' if the 'env_status' of 'env' is being changed. */
	/* Exercise 4.14: Your code here. (3/3) */

	if(env->env_status != ENV_NOT_RUNNABLE && status == ENV_NOT_RUNNABLE)
	{
		TAILQ_REMOVE(&env_sched_list,env,env_sched_link);
	}
	else if(env->env_status != ENV_RUNNABLE && status == ENV_RUNNABLE)
	{
		TAILQ_INSERT_TAIL(&env_sched_list, env, env_sched_link);
	}

	/* Step 4: Set the 'env_status' of 'env'. */
	env->env_status = status;

	/* Step 5: Use 'schedule' with 'yield' set if ths 'env' is 'curenv'. */
	if (env == curenv) {
		schedule(1);
	}
	return 0;
}

/* Overview:
 *  Set envid's trap frame to 'tf'.
 *
 * Post-Condition:
 *  The target env's context is set to 'tf'.
 *  Returns 0 on success (except when the 'envid' is the current env, so no value could be
 * returned).
 *  Returns -E_INVAL if the environment cannot be manipulated or 'tf' is invalid.
 *  Returns the original error if other underlying calls fail.
 */
int sys_set_trapframe(u_int envid, struct Trapframe *tf) {
	if (is_illegal_va_range((u_long)tf, sizeof *tf)) {
		return -E_INVAL;
	}
	struct Env *env;
	try(envid2env(envid, &env, 1));
	if (env == curenv) {
		*((struct Trapframe *)KSTACKTOP - 1) = *tf;
		// return `tf->regs[2]` instead of 0, because return value overrides regs[2] on
		// current trapframe.
		return tf->regs[2];
	} else {
		env->env_tf = *tf;
		return 0;
	}
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Post-Condition:
 * 	This function will halt the system.
 */
void sys_panic(char *msg) {
	panic("%s", TRUP(msg));
}

/* Overview:
 *   Wait for a message (a value, together with a page if 'dstva' is not 0) from other envs.
 *   'curenv' is blocked until a message is sent.
 *
 *  等待来自另一个进程的 message
 *  value (+ a page if dstva != 0)
 *  curenv 会被阻塞直到接收到 message
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_INVAL: 'dstva' is neither 0 nor a legal address.
 */
int sys_ipc_recv(u_int dstva) {
	/* Step 1: Check if 'dstva' is either zero or a legal address. */
	if (dstva != 0 && is_illegal_va(dstva)) {
		return -E_INVAL;
	}

	/* Step 2: Set 'curenv->env_ipc_recving' to 1. */
	/* 等待接收数据中 */
	/* Exercise 4.8: Your code here. (1/8) */
	curenv->env_ipc_recving = 1;
	/* Step 3: Set the value of 'curenv->env_ipc_dstva'. */
	/* 设置 接收到的页面要与自身的哪一个虚拟页面完成映射 */
	/* Exercise 4.8: Your code here. (2/8) */
	curenv->env_ipc_dstva = dstva;
	/* Step 4: Set the status of 'curenv' to 'ENV_NOT_RUNNABLE' and remove it from
	 * 'env_sched_list'. */
	/* 设为阻塞状态 */
	/* 把进程从调度序列中移除 */
	/* Exercise 4.8: Your code here. (3/8) */
	curenv->env_status = ENV_NOT_RUNNABLE;
	TAILQ_REMOVE(&env_sched_list, curenv, env_sched_link);
	/* Step 5: Give up the CPU and block until a message is received. */
	/* 放弃CPU并阻塞直到消息到达 */
	((struct Trapframe *)KSTACKTOP - 1)->regs[2] = 0;
	schedule(1);
}

/* Overview:
 *   Try to send a 'value' (together with a page if 'srcva' is not 0) to the target env 'envid'.
 *
 *  尝试发送一个 value 
 *  如果 srcva 非0 的话再发送一个 Page
 * Post-Condition:
 *   Return 0 on success, and the target env is updated as follows:
 *   - 'env_ipc_recving' is set to 0 to block future sends.
 *   - 'env_ipc_from' is set to the sender's envid.
 *   - 'env_ipc_value' is set to the 'value'.
 *   - 'env_status' is set to 'ENV_RUNNABLE' again to recover from 'ipc_recv'.
 *   env_ipc_recving = 0 -> 设置为不可接受状态 阻塞后续发送
 *   env_ipc_from -> 设置为 发送者的 envid
 *   env_ipc_value -> 设置为传入参数 value
 *   env_status = 重新设置为 ENV——RUNNABLE
 *   - if 'srcva' is not NULL, map 'env_ipc_dstva' to the same page mapped at 'srcva' in 'curenv'
 *     with 'perm'.
 *
 *   Return -E_IPC_NOT_RECV if the target has not been waiting for an IPC message with
 *   'sys_ipc_recv'.
 *   Return the original error when underlying calls fail.
 */
int sys_ipc_try_send(u_int envid, u_int value, u_int srcva, u_int perm) {
	struct Env *e;
	struct Page *p;

	/* Step 1: Check if 'srcva' is either zero or a legal address. */
	/* Exercise 4.8: Your code here. (4/8) */
	/* 检查 srcva 要么为 0  要么为 其他合法地址 */
	/* 如果 srcva 非0 且 不是合法地址 */
	if(srcva!=0 && is_illegal_va(srcva))
	{
		return -E_INVAL;
	}
	/* Step 2: Convert 'envid' to 'struct Env *e'. */
	/* 通过 envid 获得 Env *e  */
	/* This is the only syscall where the 'envid2env' should be used with 'checkperm' UNSET,
	   /* 这是 **唯一** 一次不设置 checkperm  */
	/* 的envid2env 的调用 */
	/* because the target env is not restricted to 'curenv''s children. */
	/* 因为接收者(目标进程) 并不一定是 */
	/* curenv的子进程 */
	/* Exercise 4.8: Your code here. (5/8) */
	try(envid2env(envid, &e, 0));
	/* Step 3: Check if the target is waiting for a message. */
	/* 检查target进程是否正在等待message */
	/* Exercise 4.8: Your code here. (6/8) */
	if(e->env_ipc_recving != 1)
	{
		return -E_IPC_NOT_RECV;
	}
	/* Step 4: Set the target's ipc fields. */
	/* update目标进程的ipc相关字段设置 */
	/* 值 value */
	e->env_ipc_value = value;
	/* 消息来源进程号 */
	e->env_ipc_from = curenv->env_id;
	/* 权限 */
	e->env_ipc_perm = PTE_V | perm;
	/* 取消接收状态 */
	e->env_ipc_recving = 0;

	/* Step 5: Set the target's status to 'ENV_RUNNABLE' again and insert it to the tail of
	 * 'env_sched_list'. */
	/* Exercise 4.8: Your code here. (7/8) */
	e->env_status = ENV_RUNNABLE;
	TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
	/* Step 6: If 'srcva' is not zero, map the page at 'srcva' in 'curenv' to 'e->env_ipc_dstva'
	 * in 'e'. */
	/* Return -E_INVAL if 'srcva' is not zero and not mapped in 'curenv'. */
	if (srcva != 0) {
		/* Exercise 4.8: Your code here. (8/8) */
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL)
		{
			return -E_INVAL;
		}
		try(page_insert(e->env_pgdir, e->env_asid, p, e->env_ipc_dstva, perm));
	}
	return 0;
}

// XXX: kernel does busy waiting here, blocking all envs
int sys_cgetc(void) {
	int ch;
	while ((ch = scancharc()) == 0) {
	}
	return ch;
}

/* Overview:
 *  This function is used to write data at 'va' with length 'len' to a device physical address
 *  'pa'. Remember to check the validity of 'va' and 'pa' (see Hint below);
 *
 *  这个函数是用来 将 va 处长度为 len 的数据写到设备物理地址 pa 的
 *  记得检查 va 和 pa 地址是否合法
 *  'va' is the starting address of source data, 'len' is the
 *  length of data (in bytes), 'pa' is the physical address of
 *  the device (maybe with a offset).
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *
 * Post-Condition:
 *  Data within [va, va+len) is copied to the physical address 'pa'.
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You may use the unmapped and uncached segment in kernel address space (KSEG1)
 *  to perform MMIO by assigning a corresponding-lengthed data to the address,
 *  or you can just simply use the io function defined in 'include/io.h',
 *  such as 'iowrite32', 'iowrite16' and 'iowrite8'.
 *
 *  All valid device and their physical address ranges:
 *	* ---------------------------------*
 *	|   device   | start addr | length |
 *	* -----------+------------+--------*
 *	|  console   | 0x180003f8 | 0x20   |
 *	|  IDE disk  | 0x180001f0 | 0x8    |
 *	* ---------------------------------*
 */

int valid_addr_space_num = 2;
unsigned int valid_addr_start[2] = { 0x180003f8, 0x180001f0 };
unsigned int valid_addr_end[2] = {0x180003f8 + 0x20, 0x180001f8};

static inline int is_illegal_dev_range(u_long pa, u_long len)
{
	/*
	字节（1 字节）访问：无对齐要求，地址可以是任意值。

	半字（2 字节）访问：地址必须是 2 的倍数，即 地址 % 2 == 0。

	字（4 字节）访问：地址必须是 4 的倍数，即 地址 % 4 == 0。
	 */
	if( (pa % 4 != 0 && len != 1 && len != 2) 
		|| (pa % 2 != 0 && len != 1))
	{
		return 1;
	}
	u_int start_addr = pa;
	u_int end_addr = pa + len;
	for(int i = 0;i < valid_addr_space_num ;i ++)
	{
		if(start_addr >= valid_addr_start[i] 
			&& end_addr <= valid_addr_end[i])
		{
			return 0;
		}
	}
	return 1;
}


int sys_write_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (1/2) */
	if(  is_illegal_va_range(va, len) || is_illegal_dev_range(pa, len)
					|| va % len != 0)
	{
		return -E_INVAL;
	}
	
	if( len == 1)
	{
		iowrite8(*(uint8_t *)va, pa);
	}
	else if(len == 2)
	{
		iowrite16(*(uint16_t *)va, pa);
	}
	else if(len == 4)
	{
		iowrite32(*(uint32_t *)va, pa);
	}
	else
	{
		return -E_INVAL;
	}
	return 0;
}

/* Overview:
 *  This function is used to read data from a device physical address.
 *  这个函数用来读 来自一个设备的物理地址的 数据
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *  len 必须等于 1 或者 2 或者 4
 *  否则返回 -E_INVAL
 * Post-Condition:
 *  Data at 'pa' is copied from device to [va, va+len).
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You can use function 'ioread32', 'ioread16' and 'ioread8' to read data from device.
 */
int sys_read_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (2/2) */
	if( is_illegal_va_range(va,len) || 
			is_illegal_dev_range(pa,len) 
			|| va % len != 0)
	{
		return -E_INVAL;
	}
	if(len == 4)
	{
		*(uint32_t *)va = ioread32(pa);
	}
	else if(len == 2)
	{
		*(uint16_t *)va = ioread16(pa);
	}
	else if(len == 1)
	{
		*(uint8_t *)va = ioread8(pa);
	}
	else 
	{
		return -E_INVAL;
	}
	return 0;
}

int sys_get_env_parent_id(u_int envid) {
	int r;
	struct Env* env;
	if ((r = envid2env(envid, &env, 0)) < 0) {
		printk("error in sys_get_parent_id: %d\n", r);
		return r;
	}
	return env->env_parent_id;
}


// kern/syscall_all.c

int sys_set_rpath(char *newPath) {
    if (strlen(newPath) > 1024) { return -1; }
    strcpy(curenv->r_path, newPath);
    return 0;
}

int sys_get_rpath(char *dst) {
    if (dst == 0) { return -1; }
    strcpy(dst, curenv->r_path);
    return 0;
}

/*
	@name 必须有值
	@val 必须有值
	@is_export 如果为 -1 可以不设置值
	@is_readonly 如果为 -1 可以不设置值

	实现变量定义或者变量更新
	定义 必须指定 is_export is_readonly
	更新 可以不指定 is_export is_readonly

*/
int sys_declare_env_var(char *name, char *val, int is_export, int is_readonly)
{
	struct Env_var *select_var;
	int select = -1;
	for (int i = 0;i < curenv->env_cnt;i++)
	{
		if(strcmp(name, curenv->env_vars[i].name) == 0)
		{
			select = i;
			select_var = &curenv->env_vars[i];
			break;
		}
	}
	
	if (select != -1)
	{
		if (select_var->is_readonly)
		{
			return -E_VAR_RDONLY;
		}
		if (is_export != -1) select_var->is_export = is_export;
		if (is_readonly != -1) select_var->is_readonly = is_readonly;

	} else 
	{
		select_var = &curenv->env_vars[curenv->env_cnt++];
		strcpy(select_var->name, name);
		select_var->is_readonly = is_readonly;
		select_var->is_export = is_export;
	}

	
	if (val)
	{
		strcpy(select_var->val, val);
	}
	else
	{
		select_var->val[0] = '\0';
	}
	return 0;
}

int sys_unset_env_var(char *name)
{
	for(int i = 0;i < curenv->env_cnt;i++)
	{
		if(!strcmp(name, curenv->env_vars[i].name) && !curenv->env_vars[i].is_readonly)
		{
			for(int j = i;j < curenv->env_cnt-1;j++)
			{
				curenv->env_vars[j] = curenv->env_vars[j+1];
			}
			curenv->env_cnt-=1;
			return 0;
		}
	}
	return 1;
}

int sys_list_env_var(char *output_buf)
{
	char *p = output_buf;
	char *q;
	// printk("var num:%d\n",curenv->env_cnt);
	for(int i = 0;i < curenv->env_cnt; i++)
	{
		struct Env_var *var = &curenv->env_vars[i];
		strcat(output_buf,var->name);
		strcat(output_buf,"=");
		strcat(output_buf,var->val);
		strcat(output_buf,"\n");
		// q = var->name;
		// while(*q)
		// {
		// 	(*p) = (*q);
		// 	p++,q++;
		// } 
		// *p = '=';
		// p++;
		// q = var->val;
		// while(*q)
		// {
		// 	(*p) = (*q);
		// 	p++,q++;
		// }
		// (*p) = '\n',p++;
	}
	return 0;
}

int sys_find_env_var(char *name, char *res_val)
{
	for(int i = 0;i < curenv->env_cnt;i++)
	{
		struct Env_var *var = &curenv->env_vars[i];
		if(!strcmp(name, var->name))
		{
			if(var->val)strcpy(res_val, var->val);
			else res_val[0] = '\0';
			return 0;
		}
	}
	return 1;
}

int sys_sync_penv_var()
{
	printk("panic:sync is called\n");
	struct Env *env;
	try(envid2env(curenv->env_parent_id, &env ,0));
	struct Env_var export_var[20];
	struct Env_var local_var[20];
	int eid = 0, lid = 0;
	for(int i = 0;i < env->env_cnt;i++)
	{
		struct Env_var now = env->env_vars[i];
		if(!now.is_export)
		{
			local_var[lid++] = now;
		}
	}
	for(int i = 0; i< curenv->env_cnt; i++)
	{
		struct Env_var now = curenv->env_vars[i];
		if(now.is_export)
		{
			export_var[eid++] = now;
		}
	}
	env->env_cnt = 0;
	for(int i = 0;i < eid;i++)
	{
		env->env_vars[env->env_cnt++] = export_var[i];
	}
	for(int i = 0;i < lid;i++)
	{
		env->env_vars[env->env_cnt++] = local_var[i];
	}
	return 0;
}

int sys_list_env_var1(char **name, char **res, int *cnt)
{
	*cnt = curenv->env_cnt;
	for(int i = 0; i < (*cnt);i++)
	{
		strcpy(*(name+i), curenv->env_vars[i].name);
		if(curenv->env_vars[i].val[0] != '\0')strcpy(*(res+i), curenv->env_vars[i].val);
	}
}

void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
	[SYS_get_env_parent_id] = sys_get_env_parent_id,
	[SYS_get_rpath] = sys_get_rpath,
    [SYS_set_rpath] = sys_set_rpath,
	[SYS_declare_env_var] = sys_declare_env_var,
	[SYS_unset_env_var] = sys_unset_env_var,
	[SYS_list_env_var] = sys_list_env_var,
	[SYS_find_env_var] = sys_find_env_var,
	[SYS_sync_penv_var] = sys_sync_penv_var,
	[SYS_list_env_var1] = sys_list_env_var1,
};

/* Overview:
 *   Call the function in 'syscall_table' indexed at 'sysno' with arguments from user context and
 * stack.
 *
 * Hint:
 *   Use sysno from $a0 to dispatch the syscall.
 *   The possible arguments are stored at $a1, $a2, $a3, [$sp + 16 bytes], [$sp + 20 bytes] in
 *   order.
 *   Number of arguments cannot exceed 5.
 */
void do_syscall(struct Trapframe *tf) {
	int (*func)(u_int, u_int, u_int, u_int, u_int);
	/*
		取出 Trapframe 中 a0 的值
		即 调用 `msyscall` 的第一个参数
		总共 18 个系统调用 include/syscall.h 中定义了 enum 类型
		标号 0 ~ 17 
		MAX_SYSNO = 18
	 */
	int sysno = tf->regs[4];
	if (sysno < 0 || sysno >= MAX_SYSNO) {
		// 为 Trapframe 中的 v0 赋值
		tf->regs[2] = -E_NO_SYS;
		return;
	}
	/*
		epc 寄存器存储了发生异常的指令地址
		完成异常处理 调用 ret_from_exception 返回时，也是回到 epc 指向的指令再次执行
		这里将 epc 寄存器的地址 +4
		意味着从异常返回后不重新执行 syscall 指令，而是其下一条指令
	 */
	/* Step 1: Add the EPC in 'tf' by a word (size of an instruction). */
	/* Exercise 4.2: Your code here. (1/4) */
	tf -> cp0_epc += 4;
	/* Step 2: Use 'sysno' to get 'func' from 'syscall_table'. */
	/* Exercise 4.2: Your code here. (2/4) */
	/*
		得到对应的系统调用函数的地址
	 */
	func = syscall_table[sysno];
	/* Step 3: First 3 args are stored in $a1, $a2, $a3. */
	u_int arg1 = tf->regs[5];
	u_int arg2 = tf->regs[6];
	u_int arg3 = tf->regs[7];

	/* Step 4: Last 2 args are stored in stack at [$sp + 16 bytes], [$sp + 20 bytes]. */
	u_int arg4, arg5;
	/* Exercise 4.2: Your code here. (3/4) */
	arg4 = *((u_int *)(tf->regs[29] + 16));
	arg5 = *((u_int *)(tf->regs[29] + 20));
	// 取得 arg1 - arg5 最后根据 sysno 取得的系统调用函数，调用该函数
	// 将其 返回值存储在 v0 寄存器中
	/* Step 5: Invoke 'func' with retrieved arguments and store its return value to $v0 in 'tf'.
	 */
	/* Exercise 4.2: Your code here. (4/4) */
	tf->regs[2] = func(arg1,arg2,arg3,arg4,arg5);
}
