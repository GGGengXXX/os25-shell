#ifndef SYSCALL_H
#define SYSCALL_H

#ifndef __ASSEMBLER__
// 枚举类型
/*
	SYS_putchar = 0,
	SYS_print_cons = 1,
	SYS_getenvid = 2
    SYS_yield = 3,
    SYS_env_destroy = 4
    SYS_set_tlb_mod_entry = 5
	SYS_mem_alloc
    SYS_mem_map,
    SYS_mem_unmap,
    SYS_exofork,
    SYS_set_env_status = 10
    SYS_set_trapframe,
    SYS_panic,
    SYS_ipc_try_send,
    SYS_ipc_recv,
    SYS_cgetc = 15
    SYS_write_dev,
    SYS_read_dev,
    MAX_SYSNO = 18
 */
enum {
	SYS_putchar,
	SYS_print_cons,
	SYS_getenvid,
	SYS_yield,
	SYS_env_destroy,
	SYS_set_tlb_mod_entry,
	SYS_mem_alloc,
	SYS_mem_map,
	SYS_mem_unmap,
	SYS_exofork,
	SYS_set_env_status,
	SYS_set_trapframe,
	SYS_panic,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_cgetc,
	SYS_write_dev,
	SYS_read_dev,
	SYS_get_env_parent_id,
	SYS_get_rpath,
	SYS_set_rpath,
	SYS_declare_env_var,
	SYS_unset_env_var,
	SYS_list_env_var,
	SYS_find_env_var,
	SYS_sync_penv_var,
	SYS_list_env_var1,
	MAX_SYSNO,
};

#endif

#endif
