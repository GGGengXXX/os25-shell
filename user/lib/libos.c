#include <env.h>
#include <lib.h>
#include <mmu.h>

// modify for challenge
int exit_status = -1;
void exit(int exit_status) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif
	syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
	syscall_env_destroy(0);
	user_panic("unreachable code");
}

const volatile struct Env *env;
extern int main(int, char **);

/*
#define envs ((const volatile struct Env*)UENVS)
 */

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	// syscall_getenvid() 获取当前进程的 envid
	// 通过 ENVX 由 envid 获取到对应的进程控制块的索引编号
	/*
		注意 env 和 envs 与lab3中的不一样
		这里是定义在用户虚拟空间中 供用户态访问的 user/include/lib.h
	 */
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	// modify for challenge
	exit_status = main(argc, argv);

	// exit gracefully
	// 优雅退出
	exit(exit_status);
}
