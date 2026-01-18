#include <fs.h>
#include <lib.h>

#define debug 0

static int file_close(struct Fd *fd);
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset);
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset);
static int file_stat(struct Fd *fd, struct Stat *stat);

// Dot represents choosing the member within the struct declaration
// to initialize, with no need to consider the order of members.

// 文件系统设备
struct Dev devfile = {
    .dev_id = 'f',
    .dev_name = "file",
    .dev_read = file_read,
    .dev_write = file_write,
    .dev_close = file_close,
    .dev_stat = file_stat,
};

int open_from_root(const char *path, int mode)
{
	int r;
	char ppath[1024] = {0};
	// Step 1: Alloc a new 'Fd' using 'fd_alloc' in fd.c.
	// Hint: return the error code if failed.
	struct Fd *fd;


	/* Exercise 5.9: Your code here. (1/5) */
	if ((r = fd_alloc(&fd)) != 0) {
		return r;
	}
	
	// Step 2: Prepare the 'fd' using 'fsipc_open' in fsipc.c.
	/* Exercise 5.9: Your code here. (2/5) */
	try(fsipc_open(path, mode, fd));

	// Step 3: Set 'va' to the address of the page where the 'fd''s data is cached, using
	// 'fd2data'. Set 'size' and 'fileid' correctly with the value in 'fd' as a 'Filefd'.
	// 虽然我们获得了服务进程共享给用户进程的文件描述符
	// 可文件的内容还没有被一同共享过来
	// 还需要使用 fsipc_map 进行映射
	char *va;
	struct Filefd *ffd;
	u_int size, fileid;
	/* Exercise 5.9: Your code here. (3/5) */
	// 通过 fd2data 获取文件内容应该映射到的地址
	// 该函数为不同的文件描述符提供不同的地址用于映射
	va = fd2data(fd);
	ffd = (struct Filefd *)fd;
	size = ffd->f_file.f_size;
	fileid = ffd->f_fileid;
	// Step 4: Map the file content using 'fsipc_map'.
	for (int i = 0; i < size; i += PTMAP) {
		/* Exercise 5.9: Your code here. (4/5) */
		try(fsipc_map(fileid, i, va+i));
	}

	// Step 5: Return the number of file descriptor using 'fd2num'.
	/* Exercise 5.9: Your code here. (5/5) */
	return fd2num(fd);
}

// Overview:
//  Open a file (or directory).
//
// Returns:
//  the file descriptor on success,
//  the underlying error on failure.
// 打开文件或者目录
// 成功 ： 返回文件描述符
// 失败 ： 返回错误码
int open(const char *path, int mode) 
{
	int r;
	char ppath[1024] = {0};
	// Step 1: Alloc a new 'Fd' using 'fd_alloc' in fd.c.
	// Hint: return the error code if failed.
	struct Fd *fd;


	/* Exercise 5.9: Your code here. (1/5) */
	if ((r = fd_alloc(&fd)) != 0) {
		return r;
	}
	// Step 2: Prepare the 'fd' using 'fsipc_open' in fsipc.c.
	/* Exercise 5.9: Your code here. (2/5) */
	// try(fsipc_open(path, mode, fd));
	syscall_get_rpath(ppath);
	char respath[256];
	cat2Path(ppath,path,respath);
	// if (path[0] != '/') {
	// 	if (path[0] == '.' && 
	// 		path[1] == '/') { path += 2; }
			
	// 	syscall_get_rpath(ppath);
	// 	int len1 = strlen(ppath);
	// 	int len2 = strlen(path);
	// 	if (len1 == 1) { // ppath: '/'
	// 		strcpy(ppath + 1, path);
	// 	} else {         // ppath: '/a'
	// 		ppath[len1] = '/';
	// 		strcpy(ppath + len1 + 1, path);
	// 		ppath[len1 + 1 + len2] = '\0';
	// 	}
		
	// } else {
	// 	strcpy(ppath, path);
	// }


	// Step 2: Prepare the 'fd' using 'fsipc_open' in fsipc.c.
	/* Exercise 5.9: Your code here. (2/5) */

	if ((mode & O_CREAT) == 0) {
		if ((r = fsipc_open(respath, mode, fd)) != 0) { 
			return r; 
		}
	} else { // mkdir
		mode &= ~O_CREAT;
		if ((r = fsipc_open(respath, mode, fd)) != 0) {
			return fsipc_create(respath, mode, fd); // mode = f_type
		} else {
			return 1; // already exist.
		}
	}

	// Step 3: Set 'va' to the address of the page where the 'fd''s data is cached, using
	// 'fd2data'. Set 'size' and 'fileid' correctly with the value in 'fd' as a 'Filefd'.
	// 虽然我们获得了服务进程共享给用户进程的文件描述符
	// 可文件的内容还没有被一同共享过来
	// 还需要使用 fsipc_map 进行映射
	char *va;
	struct Filefd *ffd;
	u_int size, fileid;
	/* Exercise 5.9: Your code here. (3/5) */
	// 通过 fd2data 获取文件内容应该映射到的地址
	// 该函数为不同的文件描述符提供不同的地址用于映射
	va = fd2data(fd);
	ffd = (struct Filefd *)fd;
	size = ffd->f_file.f_size;
	fileid = ffd->f_fileid;
	// Step 4: Map the file content using 'fsipc_map'.
	for (int i = 0; i < size; i += PTMAP) {
		/* Exercise 5.9: Your code here. (4/5) */
		// try(fsipc_map(fileid, i, va+i));
		if(r = fsipc_map(fileid, i, va + i) < 0)
		{
			return r;
		}
	}

	// Step 5: Return the number of file descriptor using 'fd2num'.
	/* Exercise 5.9: Your code here. (5/5) */
	return fd2num(fd);
}

// Overview:
//  Close a file descriptor
int file_close(struct Fd *fd) {
	int r;
	struct Filefd *ffd;
	void *va;
	u_int size, fileid;
	u_int i;

	ffd = (struct Filefd *)fd;
	fileid = ffd->f_fileid;
	size = ffd->f_file.f_size;

	// Set the start address storing the file's content.
	va = fd2data(fd);

	// Tell the file server the dirty page.
	for (i = 0; i < size; i += PTMAP) {
		if ((r = fsipc_dirty(fileid, i)) < 0) {
			debugf("cannot mark pages as dirty\n");
			return r;
		}
	}

	// Request the file server to close the file with fsipc.
	if ((r = fsipc_close(fileid)) < 0) {
		debugf("cannot close the file\n");
		return r;
	}

	// Unmap the content of file, release memory.
	if (size == 0) {
		return 0;
	}
	for (i = 0; i < size; i += PTMAP) {
		if ((r = syscall_mem_unmap(0, (void *)(va + i))) < 0) {
			debugf("cannont unmap the file\n");
			return r;
		}
	}
	return 0;
}

// Overview:
//  Read 'n' bytes from 'fd' at the current seek position into 'buf'. Since files
//  are memory-mapped, this amounts to a memcpy() surrounded by a little red
//  tape to handle the file size and seek pointer.
static int file_read(struct Fd *fd, void *buf, u_int n, u_int offset) {
	u_int size;
	struct Filefd *f;
	f = (struct Filefd *)fd;

	// Avoid reading past the end of file.
	size = f->f_file.f_size;

	if (offset > size) {
		return 0;
	}

	if (offset + n > size) {
		n = size - offset;
	}

	memcpy(buf, (char *)fd2data(fd) + offset, n);
	return n;
}

// Overview:
//  Find the virtual address of the page that maps the file block
//  starting at 'offset'.
int read_map(int fdnum, u_int offset, void **blk) {
	int r;
	void *va;
	struct Fd *fd;

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	va = fd2data(fd) + offset;

	if (offset >= MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if (!(vpd[PDX(va)] & PTE_V) || !(vpt[VPN(va)] & PTE_V)) {
		return -E_NO_DISK;
	}

	*blk = (void *)va;
	return 0;
}

// Overview:
//  Write 'n' bytes from 'buf' to 'fd' at the current seek position.
static int file_write(struct Fd *fd, const void *buf, u_int n, u_int offset) {
	int r;
	u_int tot;
	struct Filefd *f;

	f = (struct Filefd *)fd;

	// Don't write more than the maximum file size.
	tot = offset + n;

	if (tot > MAXFILESIZE) {
		return -E_NO_DISK;
	}
	// Increase the file's size if necessary
	if (tot > f->f_file.f_size) {
		if ((r = ftruncate(fd2num(fd), tot)) < 0) {
			return r;
		}
	}

	// Write the data
	memcpy((char *)fd2data(fd) + offset, buf, n);
	return n;
}

static int file_stat(struct Fd *fd, struct Stat *st) {
	struct Filefd *f;

	f = (struct Filefd *)fd;

	strcpy(st->st_name, f->f_file.f_name);
	st->st_size = f->f_file.f_size;
	st->st_isdir = f->f_file.f_type == FTYPE_DIR;
	return 0;
}

// Overview:
//  Truncate or extend an open file to 'size' bytes
int ftruncate(int fdnum, u_int size) {
	int i, r;
	struct Fd *fd;
	struct Filefd *f;
	u_int oldsize, fileid;

	if (size > MAXFILESIZE) {
		return -E_NO_DISK;
	}

	if ((r = fd_lookup(fdnum, &fd)) < 0) {
		return r;
	}

	if (fd->fd_dev_id != devfile.dev_id) {
		return -E_INVAL;
	}

	f = (struct Filefd *)fd;
	fileid = f->f_fileid;
	oldsize = f->f_file.f_size;
	f->f_file.f_size = size;

	if ((r = fsipc_set_size(fileid, size)) < 0) {
		return r;
	}

	void *va = fd2data(fd);

	// Map any new pages needed if extending the file
	for (i = ROUND(oldsize, PTMAP); i < ROUND(size, PTMAP); i += PTMAP) {
		if ((r = fsipc_map(fileid, i, va + i)) < 0) {
			int _r = fsipc_set_size(fileid, oldsize);
			if (_r < 0) {
				return _r;
			}
			return r;
		}
	}

	// Unmap pages if truncating the file
	for (i = ROUND(size, PTMAP); i < ROUND(oldsize, PTMAP); i += PTMAP) {
		if ((r = syscall_mem_unmap(0, (void *)(va + i))) < 0) {
			user_panic("ftruncate: syscall_mem_unmap %08x: %d\n", va + i, r);
		}
	}

	return 0;
}

// Overview:
//  Delete a file or directory.
int remove(const char *path) {
	// Call fsipc_remove.
	char a[1024],b[1024];
	syscall_get_rpath(a);
	cat2Path(a,path,b);
	/* Exercise 5.13: Your code here. */
	// return fsipc_remove(path);
	return fsipc_remove(b);
}

// Overview:
//  Synchronize disk with buffer cache
int sync(void) {
	return fsipc_sync();
}


int create(const char* path, int type) {
	char a[1024],b[1024];
	syscall_get_rpath(a);
	cat2Path(a,path,b);
	// printf("create [%s]\n",path);
	// return fsipc_create(path, type);
	return fsipc_create(b,type);
}


// user/lib/file.c

int chdir(char *newPath) {
    return syscall_set_rpath(newPath);
}

int getcwd(char *path) {
    return syscall_get_rpath(path);
}

/*
连接两个路径
默认path1 必须以 / 结尾
path2 可以为 . 此时返回 path1
可以为 `/` 开头 此时返回 path2

*/
void cat2Path(char *path1, char *path2, char *res) {
    char buf1[128], buf2[128];
    char *segs[128];
    int top = 0;

    // 1. 处理 path1
    strcpy(buf1, path1);
    buf1[128-1] = '\0';
    // 去掉尾部多余 '/'
    int len1 = strlen(buf1);
    while (len1 > 1 && buf1[len1-1] == '/') {
        buf1[--len1] = '\0';
    }
    // 手动切分 buf1
    char *p = buf1;
    while (*p) {
        // 跳过所有 '/'
        while (*p == '/') p++;
        if (!*p) break;
        // 记录段开始
        char *start = p;
        // 找到下一个 '/' 或 末尾
        while (*p && *p != '/') p++;
        // 断开段
        if (*p) {
            *p = '\0';
            p++;
        }
        // 将段入栈
        segs[top++] = start;
    }

    // 2. 如果 path2 是绝对路径，重置 stack
    if (path2[0] == '/') {
        top = 0;
    }

    // 3. 处理 path2
    strcpy(buf2, path2);
    buf2[128-1] = '\0';
    p = buf2;
    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;
        char *start = p;
        while (*p && *p != '/') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
        // 判断 "."、".."、普通段
        if (strcmp(start, ".") == 0) {
            // do nothing
        } else if (strcmp(start, "..") == 0) {
            if (top > 0) top--;
        } else {
            segs[top++] = start;
        }
    }

    // 4. 生成结果到 res
    char *r = res;
    if (top == 0) {
        // 根目录
        *r++ = '/';
        *r = '\0';
    } else {
        for (int i = 0; i < top; i++) {
            *r++ = '/';
            // 复制段内容
            char *s = segs[i];
            while (*s) {
                *r++ = *s++;
            }
        }
        *r = '\0';
    }
}