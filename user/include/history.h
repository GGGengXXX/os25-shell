#ifndef _HISTORY_H_
#define _HISTORY_H_

typedef struct
{
	int now_ptr;
	int cursor_ptr;
	char hisBuffer[20][1024];
	int hisBuffer_ptr;
	char newBuffer[1024];
} HistoryINFO;

// 创建history文件 但是什么都不做
void init_hisNode(HistoryINFO *history);

// 保存最新指令
void store_cmd(HistoryINFO * history, char *cmd);

// 调用 history 
void call_history(HistoryINFO * history);

// 响应上键
void pressUP(HistoryINFO * hisnode, char *buf, int *pi, int *plen);

#endif