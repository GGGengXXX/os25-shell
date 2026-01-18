#ifndef _ENV_H_
#define _ENV_H_

#include <mmu.h>
#include <queue.h>
#include <trap.h>
#include <types.h>

#define LOG2NENV 10
#define NENV (1 << LOG2NENV)
#define ENVX(envid) ((envid) & (NENV - 1))

// All possible values of 'env_status' in 'struct Env'.
#define ENV_FREE 0
#define ENV_RUNNABLE 1
#define ENV_NOT_RUNNABLE 2

struct Env_var{
	char name[20];
	char val[20];
	int is_export;
	int is_readonly;
};

// Control block of an environment (process).
struct Env {
	// 用于保存上下文信息的结构体
	struct Trapframe env_tf;	 // saved context (registers) before switching
	LIST_ENTRY(Env) env_link;	 // intrusive entry in 'env_free_list'
	u_int env_id;			 // unique environment identifier
	u_int env_asid;			 // ASID of this env
	u_int env_parent_id;		 // env_id of this env's parent
	// 有三种可能的取值 ENV_FREE ENV_RUNNABLE ENV_NOT_RUNNABLE
	u_int env_status;		 // status of this env
	// 该进程页目录的内核虚拟地址
	Pde *env_pgdir;			 // page directory
	// 该字段用来构造 调度序列
	TAILQ_ENTRY(Env) env_sched_link; // intrusive entry in 'env_sched_list'
	// 保存 进程的优先级
	u_int env_pri;			 // schedule priority

	// Lab 4 IPC
	// 进程传递的具体数值
	u_int env_ipc_value;   // the value sent to us
	// 发送方的进程id
	u_int env_ipc_from;    // envid of the sender
	// 1： 等待接收数据中
	// 0： 不可接收数据
	u_int env_ipc_recving; // whether this env is blocked receiving
	// 接收到的页面要与自身的哪一个虚拟页面完成映射
	u_int env_ipc_dstva;   // va at which the received page should be mapped
	// 传送的页面的权限位设置
	u_int env_ipc_perm;    // perm in which the received page should be mapped

	// Lab 4 fault handling
	u_int env_user_tlb_mod_entry; // userspace TLB Mod handler

	// Lab 6 scheduler counts
	u_int env_runs; // number of times we've been env_run'ed
	char r_path[256];
	struct Env_var env_vars[20];
	int env_cnt;
};

LIST_HEAD(Env_list, Env);
/*
	_TAILQ_HEAD(Env_sched_list,struct Env)
	struct Env_sched_list{
		struct Env * tqh_first;
		struct Env ** tqh_last;
	}
 */
TAILQ_HEAD(Env_sched_list, Env);
extern struct Env *curenv;		     // the current env
extern struct Env_sched_list env_sched_list; // runnable env list

void env_init(void);
int env_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
struct Env *env_create(const void *binary, size_t size, int priority);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e) __attribute__((noreturn));

void env_check(void);
void envid2env_check(void);

#define ENV_CREATE_PRIORITY(x, y)                                                                  \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, y);                       \
	})

#define ENV_CREATE(x)                                                                              \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, 1);                       \
	})

#endif // !_ENV_H_
