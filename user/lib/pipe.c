#include <env.h>
#include <lib.h>
#include <mmu.h>
#define debug 0

static int pipe_close(struct Fd *);
static int pipe_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int pipe_stat(struct Fd *, struct Stat *);
static int pipe_write(struct Fd *fd, const void *buf, u_int n, u_int offset);

struct Dev devpipe = {
    .dev_id = 'p',
    .dev_name = "pipe",
    .dev_read = pipe_read,
    .dev_write = pipe_write,
    .dev_close = pipe_close,
    .dev_stat = pipe_stat,
};

#define PIPE_SIZE 32 // small to provoke races

struct Pipe {
	/* 下一个将要从管道读的数据的位置 */
	u_int p_rpos;		 // read position
	/* 下一个将要向管道写的数据的位置 */
	u_int p_wpos;		 // write position
	/* 环形缓冲区 */
	u_char p_buf[PIPE_SIZE]; // data buffer
};

/* Overview:
 *   Create a pipe.
 *
 * Post-Condition:
 *   Return 0 and set 'pfd[0]' to the read end and 'pfd[1]' to the
 *   write end of the pipe on success.
 *   Return an corresponding error code on error.
 */
/*
 将 pfd[0] 设置为管道的 读端口
 将 pfd[1] 设置为管道的 写端口
 */
/*
* 首先，创建并分配两个文件描述符 fd0、 fd1，并为这两个文件描述符自身分配相应的空
间。
• 然后给 fd0 对应的数据区域分配一页空间，并将 fd1 对应的数据区域映射到相同的物理
页，这一页的内容为 pipe 结构体。
• 最后，将作为读端的 fd0 的权限设置为只读，作为写端的 fd1 的权限设置为只写，并通
过函数的传入参数将这两个文件描述符的编号返回。
值得注意的是，目前我们一共分配了三个页面，权限位均需要包含 PTE_LIBRARY，这是因
为这些页面的数据对于父子进程是共享的，修改页面内容时不触发写时复制，这样才能顺利的在
父子进程之间传递信息。
 */
int pipe(int pfd[2]) {
	int r;
	void *va;
	struct Fd *fd0, *fd1;

	/* Step 1: Allocate the file descriptors. */
	// param: envid ,va ,perm
	// 在 fd0 处申请一个物理页
	if ((r = fd_alloc(&fd0)) < 0 || (r = syscall_mem_alloc(0, fd0, PTE_D | PTE_LIBRARY)) < 0) {
		goto err;
	}
	// 在 fd1 处申请一个物理页
	if ((r = fd_alloc(&fd1)) < 0 || (r = syscall_mem_alloc(0, fd1, PTE_D | PTE_LIBRARY)) < 0) {
		goto err1;
}

	/* Step 2: Allocate and map the page for the 'Pipe' structure. */
	// 得到 fd0 对应的编号 然后 FILEBASE(0x60000000) + (i) * PDMAP;
	// 根据 fd0 确定另一个 虚拟地址va
	// 作为 pipe 为 va申请一个物理页
	va = fd2data(fd0);
	if ((r = syscall_mem_alloc(0, (void *)va, PTE_D | PTE_LIBRARY)) < 0) {
		goto err2;
	}
	// 根据 fd1 也可以确定一个 虚拟地址va
	// 使得这两个虚拟地址映射到同一个物理页
	if ((r = syscall_mem_map(0, (void *)va, 0, (void *)fd2data(fd1), PTE_D | PTE_LIBRARY)) <
	    0) {
		goto err3;
	}
	// 共计 3 个物理页
	// pipe的物理页有两个映射


	/* Step 3: Set up 'Fd' structures. */
	fd0->fd_dev_id = devpipe.dev_id;
	fd0->fd_omode = O_RDONLY;

	fd1->fd_dev_id = devpipe.dev_id;
	fd1->fd_omode = O_WRONLY;

	// debugf("[%08x] pipecreate \n", env->env_id, vpt[VPN(va)]);

	/* Step 4: Save the result. */
	pfd[0] = fd2num(fd0);
	pfd[1] = fd2num(fd1);
	return 0;

err3:
	syscall_mem_unmap(0, (void *)va);
err2:
	syscall_mem_unmap(0, fd1);
err1:
	syscall_mem_unmap(0, fd0);
err:
	return r;
}

/* Overview:
 *   Check if the pipe is closed.
 *
 * Post-Condition:
 *   Return 1 if pipe is closed.
 *   Return 0 if pipe isn't closed.
 *
 * Hint:
 *   Use 'pageref' to get the reference count for
 *   the physical page mapped by the virtual page.
 */
static int _pipe_is_closed(struct Fd *fd, struct Pipe *p) {
	// The 'pageref(p)' is the total number of readers and writers.
	// pageref(p) 是readers 和 writers的总数
	// The 'pageref(fd)' is the number of envs with 'fd' open
	// (readers if fd is a reader, writers if fd is a writer).
	//
	// Check if the pipe is closed using 'pageref(fd)' and 'pageref(p)'.
	// If they're the same, the pipe is closed.
	// Otherwise, the pipe isn't closed.

	int fd_ref, pipe_ref, runs;
	// Use 'pageref' to get the reference counts for 'fd' and 'p', then
	// save them to 'fd_ref' and 'pipe_ref'.
	// Keep retrying until 'env->env_runs' is unchanged before and after
	// reading the reference counts.
	/* Exercise 6.1: Your code here. (1/3) */
	/*
	 * 同一次调度中 runs 保持不变
	 * 保障 fd_ref 和 pipe_ref 是在同一次调度中得到的
	 */
	do{
		runs = env->env_runs;
		fd_ref = pageref(fd);
		pipe_ref = pageref(p);
	}while(runs != env->env_runs);
	return fd_ref == pipe_ref;
}

/* Overview:
 *   Read at most 'n' bytes from the pipe referred by 'fd' into 'vbuf'.
 *
 * Post-Condition:
 *   Return the number of bytes read from the pipe.
 *   The return value must be greater than 0, unless the pipe is closed and nothing
 *   has been written since the last read.
 *
 * Hint:
 *   Use 'fd2data' to get the 'Pipe' referred by 'fd'.
 *   Use '_pipe_is_closed' to check if the pipe is closed.
 *   The parameter 'offset' isn't used here.
 */
static int pipe_read(struct Fd *fd, void *vbuf, u_int n, u_int offset) {
	int i;
	struct Pipe *p;
	char *rbuf;

	// Use 'fd2data' to get the 'Pipe' referred by 'fd'.
	// Write a loop that transfers one byte in each iteration.
	// Check if the pipe is closed by '_pipe_is_closed'.
	// When the pipe buffer is empty:
	//  - If at least 1 byte is read, or the pipe is closed, just return the number
	//    of bytes read so far.
	//  - Otherwise, keep yielding until the buffer isn't empty or the pipe is closed.
	/* Exercise 6.1: Your code here. (2/3) */
	p = (struct Pipe *)fd2data(fd);
	rbuf = (char *)vbuf;
	for(i = 0;i < n;i++)
	{
		while(p->p_rpos == p->p_wpos)
		{
			if(_pipe_is_closed(fd,p) || i > 0)
			{
				return i;
			}
			syscall_yield();
		}
		rbuf[i] = p->p_buf[p->p_rpos % PIPE_SIZE];
		p->p_rpos++;
	}
	return n;
	user_panic("pipe_read not implemented");
}

/* Overview:
 *   Write 'n' bytes from 'vbuf' to the pipe referred by 'fd'.
 *
 * Post-Condition:
 *   Return the number of bytes written into the pipe.
 *
 * Hint:
 *   Use 'fd2data' to get the 'Pipe' referred by 'fd'.
 *   Use '_pipe_is_closed' to judge if the pipe is closed.
 *   The parameter 'offset' isn't used here.
 */
static int pipe_write(struct Fd *fd, const void *vbuf, u_int n, u_int offset) {
	int i;
	struct Pipe *p;
	char *wbuf;

	// Use 'fd2data' to get the 'Pipe' referred by 'fd'.
	// Write a loop that transfers one byte in each iteration.
	// If the bytes of the pipe used equals to 'PIPE_SIZE', the pipe is regarded as full.
	// Check if the pipe is closed by '_pipe_is_closed'.
	// When the pipe buffer is full:
	//  - If the pipe is closed, just return the number of bytes written so far.
	//  - If the pipe isn't closed, keep yielding until the buffer isn't full or the
	//    pipe is closed.
	/* Exercise 6.1: Your code here. (3/3) */
	p = (struct Pipe *)fd2data(fd);
	wbuf = (char *)vbuf;
	for(i = 0;i < n;i++)
	{
		while(p->p_wpos -  p->p_rpos == PIPE_SIZE){
			if(_pipe_is_closed(fd,p)){
				return i;
			}
			syscall_yield();
		}
		p->p_buf[p->p_wpos % PIPE_SIZE] = wbuf[i];
		p->p_wpos++;
	}
	// user_panic("pipe_write not implemented");

	return n;
}

/* Overview:
 *   Check if the pipe referred by 'fdnum' is closed.
 *
 * Post-Condition:
 *   Return 1 if the pipe is closed.
 *   Return 0 if the pipe isn't closed.
 *   Return -E_INVAL if 'fdnum' is invalid or unmapped.
 *
 * Hint:
 *   Use '_pipe_is_closed'.
 */
int pipe_is_closed(int fdnum) {
	struct Fd *fd;
	struct Pipe *p;
	int r;

	// Step 1: Get the 'fd' referred by 'fdnum'.
	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}
	// Step 2: Get the 'Pipe' referred by 'fd'.
	p = (struct Pipe *)fd2data(fd);
	// Step 3: Use '_pipe_is_closed' to judge if the pipe is closed.
	return _pipe_is_closed(fd, p);
}

/* Overview:
 *   Close the pipe referred by 'fd'.
 *
 * Post-Condition:
 *   Return 0 on success.
 *
 * Hint:
 *   Use 'syscall_mem_unmap' to unmap the pages.
 */
static int pipe_close(struct Fd *fd) {
	// Unmap 'fd' and the referred Pipe.
	syscall_mem_unmap(0, fd);
	syscall_mem_unmap(0, (void *)fd2data(fd));
//	syscall_mem_unmap(0, fd);
	return 0;
}

static int pipe_stat(struct Fd *fd, struct Stat *stat) {
	return 0;
}
