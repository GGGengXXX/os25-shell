#include <lib.h>
#include <history.h>

#define HISNAME "/.mos_history"
#define HIS_MAX 1024
#define HISTORY_SIZE 20
// 用于 history 输出
char outbuf[8192];


// 清空当前行
void clear_line(int *pi, int *plen)
{
	int i = *pi, len = *plen;
	*pi = *plen = 0;
	// 光标到行首
	if(i != 0)
	{
		printf("\033[%dD",i);
	}
	// 清空这一行
	for(int j = 0;j < len;j++)
	{
		printf(" ");
	}
	// 光标回到行首
	if(len != 0)
	{
		printf("\033[%dD",len);
	}
}

int prehisid(int id)
{
	return (id - 1 + HISTORY_SIZE) % HISTORY_SIZE;
}

int nexthisid(int id)
{
	return (id + 1) % HISTORY_SIZE;
}

int modid(int id)
{
	return (id + HISTORY_SIZE) % HISTORY_SIZE;
}

// 创建history文件 但是什么都不做
void init_hisNode(HistoryINFO *history)
{
	int fd, r;
	if ((fd = open(HISNAME, O_RDWR)) >= 0) {
        close(fd);
    }
	else
	{
		fd = open(HISNAME, O_CREAT);
	}
}

// 保存最新指令
// 如果与上一条相同，就不存储了
void store_cmd(HistoryINFO * history, char *cmd)
{
	// 助教说不用这个
	if(history->cursor_ptr != 0 && 
		strcmp(cmd, history->hisBuffer[prehisid(history->hisBuffer_ptr)]) == 0)
	{
		return;
	}
	// 写入缓冲区
	strcpy(history->hisBuffer[history->hisBuffer_ptr], cmd);
	// 移动指针
	history->hisBuffer_ptr = nexthisid(history->hisBuffer_ptr);
	history->now_ptr++, history->cursor_ptr = history->now_ptr;

	save2file(history);
	
}

// 打开 .mos_history
// 指令写入文件
void save2file(HistoryINFO *history)
{
	int fd;
	if((fd = open(HISNAME,O_WRONLY| O_TRUNC)) < 0)
	{
		printf("failed to open %s when store cmd\n",HISNAME);
		return;
	}
	for(int i = history->hisBuffer_ptr; 
		i < history->hisBuffer_ptr + HISTORY_SIZE;i++)
	{
		int ptr = modid(i);
		if(history->hisBuffer[ptr][0] == '\0')continue;
		fprintf(fd, "%s\n", history->hisBuffer[ptr]);
	}
	close(fd);
}


void call_history(HistoryINFO * history)
{
	long n;
	int f, r;
	if((f = open(HISNAME, O_RDONLY)) < 0)
	{
		printf("failed to open %s when call history\n",HISNAME);
		return;
	}

	while ((n = read(f, outbuf, (long)sizeof outbuf)) > 0) {
		if ((r = write(1, outbuf, n)) != n) {
			user_panic("write error copying %s: %d", HISNAME, r);
		}
	}
	if (n < 0) {
		user_panic("error reading %s: %d", HISNAME, n);
	}

}

// 判断能否按上键
int is_up_able(HistoryINFO * hisnode)
{
	// 如果位于最上面一条，或者和当前行的距离超过20，返回 0
	int fl = ((hisnode->cursor_ptr != 0 )
	&& (hisnode->cursor_ptr + HISTORY_SIZE != hisnode->now_ptr));
	return fl;
}

// 判断能否按下键
int is_down_able(HistoryINFO * hisnode)
{
	return !(hisnode->now_ptr == hisnode-> cursor_ptr);
}

// 判断按 UP 之前，是否应该将当前行放入缓存
// 即当前行就是最新行
int is_newline(HistoryINFO * hisnode)
{
	return (hisnode->now_ptr == hisnode->cursor_ptr);
}

// 判断 DOWN 之后， 是否到了最新行
int is_next_newline(HistoryINFO * hisnode)
{
	printf("cursor_ptr %d now_ptr %d\n",hisnode->cursor_ptr, hisnode->now_ptr);
	return (hisnode->cursor_ptr + 1 == hisnode->now_ptr);
}

void print_hisline(HistoryINFO* hisnode, int id,char *buf, int *pi,int *plen)
{
	clear_line(pi, plen);
	printf("%s",hisnode->hisBuffer[id]);
	strcpy(buf,hisnode->hisBuffer[id]);
	*pi = *plen = strlen(hisnode->hisBuffer[id]);
	// printf("buf[%s]\n",buf);
}

void print_curline(HistoryINFO*hisnode, char * buf,int *pi,int *plen)
{
	clear_line(pi, plen);
	printf("%s", hisnode->newBuffer);
	strcpy(buf, hisnode->newBuffer);
	*pi = *plen = strlen(hisnode->newBuffer);
	// printf("buf[%s]\n",buf);
}

// 按下了上键
void pressUP(HistoryINFO * hisnode, char *buf, int *pi, int *plen)
{
	if(!is_up_able(hisnode))return;
	// 保存当前行
	if(is_newline(hisnode))
	{
		strcpy(hisnode->newBuffer, buf);
	}
	else
	{
		strcpy(hisnode->hisBuffer[modid(hisnode->cursor_ptr)], buf);
	}

	hisnode->cursor_ptr -= 1;
	print_hisline(hisnode, modid(hisnode->cursor_ptr),buf,pi,plen);
}

// 按下了下键
void pressDOWN(HistoryINFO * hisnode, char *buf, int *pi,int *plen)
{
	if(!is_down_able(hisnode))return;

	strcpy(hisnode->hisBuffer[modid(hisnode->cursor_ptr)], buf);
	hisnode->cursor_ptr += 1;
	if(is_newline(hisnode))
	{
		print_curline(hisnode, buf, pi, plen);
	}
	else
	{
		print_hisline(hisnode,modid(hisnode->cursor_ptr),buf,pi,plen);
	}
}