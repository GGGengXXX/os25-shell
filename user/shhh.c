#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`#"


#define TOKEN_EOF 0				// \0
#define TOKEN_STDIN_REDIRECT 1	// <
#define TOKEN_STDOUT_REDIRECT 2	// >
#define TOKEN_PIPE 3			// |
#define TOKEN_WORD 4			// word
#define TOKEN_BACKQUOTE 5		// `
#define TOKEN_COMMENT 6			// #
#define TOKEN_SIMICOLON 7		// ;
#define TOKEN_APPEND_REDIRECT 8	// >>
//#define TOKEN_QUOTATION 9		// "
#define TOKEN_BACKGOUND_EXC 10	// &
#define TOKEN_AND 11			// &&
#define TOKEN_OR 12				// ||
#define TOKEN_ERR -1			// error

const int HISTORY_SIZE = 4;
/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */


 static int curLine, hisCount, hisFull, hisContain;
 static int hisBuf[1024];
 static int interactive;

 // 字符串拼接的函数
void strcat(char *a, char * b)
{
	char *p = a;
	while(*p)p++;
	char *q = b;
	while(*q)(*p) = (*q),p++,q++;
	(*p) = '\0';
}



// return token type and set **begin and **end to the beginning and end of the token.
int _getNextToken(char **begin, char **end) {
	// char* 指向begin
	char *cmd = *begin;
	// null cmd
	// 如果指针指向空 返回 TOKEN_EOF
	if (cmd == 0) {
		return TOKEN_EOF;
	}
	// skip leading whitespace
	// 跳过空白字符
	while (strchr(WHITESPACE, *cmd)) {
		*cmd++ = 0;
	}
	if (*cmd == '\0') {
		*begin = *end = cmd;
		return TOKEN_EOF;
	}
	// check for special symbols
	// 检查特殊字符 "<|>&;()`#"
	// begin 指向符号 end指向符号的下一个字符
	// 把 begin 这里写一个 '\0'
	if (strchr(SYMBOLS, *cmd)) {
		switch (*cmd) {
		case '<':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_STDIN_REDIRECT;
		case '>':
			*begin = cmd;
			if (cmd[1] == '>') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_APPEND_REDIRECT;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_STDOUT_REDIRECT;
			}
		case '|':
			*begin = cmd;
			if (cmd[1] == '|') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_OR;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_PIPE;
			}
		case '`':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_BACKQUOTE;
		case '#':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_COMMENT;
		case ';':
			*begin = cmd;
			cmd[0] = 0;
			*end = cmd + 1;
			return TOKEN_SIMICOLON;
		// case '\"':
		// 	*begin = cmd;
		// 	cmd[0] = 0;
		// 	*end = cmd + 1;
		// 	return TOKEN_QUOTATION;
		case '&':
			*begin = cmd;
			if (cmd[1] == '&') {
				cmd[0] = cmd[1] = 0;
				*end = cmd + 2;
				return TOKEN_AND;
			} else {
				cmd[0] = 0;
				*end = cmd + 1;
				return TOKEN_BACKGOUND_EXC;
			}
		}
		return TOKEN_ERR;
	}
	if (*cmd == '\"') { // parse a quoted word
		*cmd = '\0';
		cmd++;
		*begin = cmd;
		while (*cmd && *cmd != '\"') {
			cmd++;
		}
		if (*cmd == '\"') {
			*cmd = '\0';
			cmd++;
		}
		*end = cmd;
		return TOKEN_WORD;
	} else { // parse a word
		*begin = cmd;
		while (*cmd && !strchr(WHITESPACE SYMBOLS, *cmd)) {
			cmd++;
		}
		*end = cmd;
		return TOKEN_WORD;
	}
}

/*
 * if cmd is null, return the next token in the current string.
 * if cmd is not null, parse the string and return the next token.
 */
// 如果cmd非零 返回结束 TOKEN_EOF
int getNextToken(char *cmd, char **buf) {

	static int type;
	static char *begin, *last_end;

	if (cmd != 0) { // set new cmd
		begin = cmd;
		last_end = cmd;
		return TOKEN_EOF;
	} else { // get next token
		begin = last_end;
		type = _getNextToken(&begin, &last_end);
		*buf = begin;
		return type;
	}
}


/*
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}*/

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	/* 保存命令行中单词（即参数）的个数。比如命令行是 cat file.txt，那么 argc 最后是 2。 */
	int argc = 0;
	while (1) {
		char *buf;
		int fd, r;
		/*调用 gettoken() 从命令行中读下一个 token

返回值 c 是 token 类型：'w'（单词/参数），或符号如 '<'、'>'、'|'；

t 是指向这个 token 字符串的指针（例如 cat、file.txt 等）；*/
		int type = getNextToken(0, &buf);
		int p[2];
		char backquote_buf[1024] = {0};
//		int c = gettoken(0, &t);
		switch (type) {
		case TOKEN_EOF:
			return argc;
		case TOKEN_BACKGOUND_EXC:
			return argc;
		case TOKEN_WORD:
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = buf;
			break;
		case TOKEN_STDIN_REDIRECT:
			if (getNextToken(0, &buf) != TOKEN_WORD) {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			fd = open(buf, O_RDONLY);
			if (fd < 0) {
				debugf("open %s failed!\n", buf);
				exit();
			}
			dup(fd, 0);
			close(fd);
			break;
		// 输出重定向
		case TOKEN_STDOUT_REDIRECT:
		// 后面应该有一个 word
			if (getNextToken(0, &buf) != TOKEN_WORD) {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// 打开这个文件
			fd = open(buf, O_WRONLY);
			if (fd < 0) {
				debugf("open %s failed!\n", buf);
				exit();
			}
			close(fd);
			remove(buf);                    // 删除该文件（清空）
			create(buf, FTYPE_REG);        // 重新创建一个空的常规文件
			fd = open(buf, O_WRONLY);      // 再次打开新文件
			dup(fd, 1);                    // 将该 fd 复制到标准输出（文件描述符 1）
			close(fd);                     // 关闭原来的 fd
			break;
		// 追加重定向
		case TOKEN_APPEND_REDIRECT:
			if (getNextToken(0, &buf) != TOKEN_WORD) {
				debugf("syntax error: >> not followed by word\n");
				exit();
			}
			fd = open(buf, O_WRONLY);
			if (fd < 0) {
				create(buf, FTYPE_REG);
				fd = open(buf, O_WRONLY);
			}
			struct Fd *fd_struct = (struct Fd*) num2fd(fd);
			struct Filefd *ffd = (struct Filefd*) fd_struct;
			fd_struct->fd_offset = ffd->f_file.f_size;
			dup(fd, 1);
			close(fd);
			break;
		case TOKEN_PIPE:
			pipe(p);
			*rightpipe = fork();
			if (*rightpipe == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else if (*rightpipe > 0) {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		case TOKEN_COMMENT:
			return argc;
		}
	}
	return argc;
}

// 这个函数 executeCommandAndCaptureOutput 的作用是 在子进程中运行命令 cmd，并把它的标准输出捕获到父进程给定的缓冲区 output 中。
int executeCommandAndCaptureOutput(char *cmd, char *output, int maxLen) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return -1;
    }

    int pid = fork();
    if (pid == -1) {
        return -1;
    }

    if (pid == 0) { // Child process
        close(pipefd[0]);
        dup(pipefd[1], 1);
        close(pipefd[1]);
		debugf("`child` running command %s\n", cmd);
		runcmd_conditional(cmd);
		debugf("`child` finished running command %s\n", cmd);
    } else { // Parent process
		// dup(pipefd[0], 0);
        close(pipefd[1]);

		int r;
		for (int i = 0; i < maxLen; i++) {
			if ((r = read(pipefd[0], output + i, 1)) != 1) {
				if (r < 0) {
					debugf("read error: %d\n", r);
				}
				break;
			}
		}
		// debugf("`parent` read output: <%s>\n", output);

        close(pipefd[0]);
        wait(pid);
    }

    return 0;
}

int replaceBackquoteCommands(char *cmd) {
    char *begin, *end;
    char output[1024];

	// debugf("replaceBackquoteCommands: <%s>\n", cmd);

	while (1) {
		char *t = cmd;
		int in_quotes = 0;

		begin = 0;
		while(*t != '\0') {
			if (*t == '\"') {
				in_quotes = !in_quotes;
			}
			if (*t == '`' && !in_quotes) {
				begin = t;
				t++;
				break;
			}
			t++;
		}
		if (begin == NULL) {
			return 0; // No backquote found
		}
		end = 0;
		while(*t != '\0') {
			if (*t == '\"') {
				in_quotes = !in_quotes;
			}
			if (*t == '`' && !in_quotes) {
				end = t;
				t++;
				break;
			}
			t++;
		}
        if (end == NULL) {
            return -1; // Syntax error: unmatched backquote
        }

        *begin = '\0'; // Terminate the string at the start of the backquote command
        *end = '\0'; // Terminate the backquote command

		char temp_cmd[1024];
		strcpy(temp_cmd, end + 1);

        // Execute the command and capture its output
		printf("sub command %s\n", begin+1);
        if (executeCommandAndCaptureOutput(begin + 1, output, sizeof(output)) == -1) {
            return -1;
        }

		// debugf("backquote output: <%s>\n", output);

		// debugf("original cmd: <%s>\n", cmd);

        // Concatenate the parts
        strcat(cmd, output);
        strcat(cmd, temp_cmd);

		// debugf("new cmd: <%s>\n", cmd);
    }
    return 0;
}


int runcmd(char *s, int background_exc) {
	char ori_cmd[1024];
	strcpy(ori_cmd, s);

	replaceBackquoteCommands(s);

//	gettoken(s, 0);
	getNextToken(s,0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return 0;
	}
	argv[argc] = 0;


	// modify for challenge
	// history
	if (strcmp(argv[0], "history") == 0) {
		int history_fd = -1;
		// 打开 .mosh_history 文件
		if ((history_fd = open("/.mosh_history", O_RDONLY)) < 0) {
			debugf("can not open /.mosh_history: %d\n", history_fd);
			return 1;
		}
		char history_buf[1024];
		int i = 0;
		int r;
		// 把文件内容读到history_buf中
		for (int i = 0; i < 1024; i++) {
			if ((r = read(history_fd, history_buf + i, 1)) != 1) {
				if (r < 0) {
					debugf("read error: %d\n", r);
				}
				break;
			}
			if(*(history_buf + i) == -1)
			{
				*(history_buf+i) = '\0';
				break;
			}
		}
		// 把文件内容print出来
		printf("%s",history_buf);
		close(history_fd);
		close_all();
		if (rightpipe) {
			wait(rightpipe);
		}
		return 0;
	}

	if(strcmp(argv[0],"cd") == 0)
	{
		debugf("functioning cd");
		return 0;
	}

	int child = spawn(argv[0], argv);
// modify for challenge
	if(child < 0)
	{
		char cmd_b[1024];
		int cmd_len = strlen(argv[0]);
		strcpy(cmd_b,argv[0]);
		if (cmd_b[cmd_len-1] == 'b' && cmd_b[cmd_len-2]=='.')
		{
			cmd_b[cmd_len-2] = '\0';
		}
		else 
		{
			cmd_b[cmd_len] = '.';
			cmd_b[cmd_len+1] = 'b';
			cmd_b[cmd_len+2] = '\0';
		}
		child = spawn(cmd_b,argv);
	}
	close_all();

	/*
	if (child >= 0) {
		wait(child);
	} else {
		
		debugf("spawn %s: %d\n", argv[0], child);
	}*/
	

	int exit_status = -1;
	if(child >= 0)
	{
		if(background_exc)
		{
			syscall_ipc_try_send(env->env_parent_id, child, 0, 0);
			syscall_ipc_recv(0);
			wait(child);
			exit_status = env->env_ipc_value;
		}
		else
		{
			syscall_ipc_recv(0);
			wait(child);
			exit_status = env->env_ipc_value;
		}
	}
	else
	{
		debugf("spawn %s: %d\n", argv[0], child);
	}


	if (rightpipe) {
		wait(rightpipe);
	}
	return exit_status;
	// exit();
}


int readPast(int target, char *code) {
	// history
	int r, fd, spot = 0;
	char buff[10240];
	if ((fd = open("/.mosh_history", O_RDWR)) < 0 && (fd != 0)) {
		printf("%d", fd);
		return fd;
	}
	// for(int i=  0;i < target;i ++)
	// {
	// 	spot += (hisBuf[i] + 1);		
	// }
	for(int i = (hisCount - hisContain + HISTORY_SIZE) % HISTORY_SIZE; i != target; i = (i + 1) % HISTORY_SIZE)
	{
		spot += (hisBuf[i] + 1);
	}
	if ((r = readn(fd, buff, spot)) != spot) { printf("G2");return r; }
	if ((r = readn(fd, code, hisBuf[target])) != hisBuf[target]) { printf("G3");return r; }
	if ((r = close(fd)) < 0) { printf("G4");return r; }

	code[hisBuf[target]] = '\0';
	return 0;
}

void readline(char *buf, u_int n)
{
	char curIn[1024];
	int r, len = 0;
	char temp = 0;
	// i 是光标位置 len 是字符串长度
	for (int i = 0; len < n;)
	{
		if ((r = read(0, &temp, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}
		switch(temp) {
			case '~':
				if (i == len) {break;}

				for(int j = i;j <= len - 1;j++) {
					buf[j] = buf[j+1];
				}
				buf[--len] = 0;
				if (i != 0) {
					printf("\033[%dD%s \033[%dD", i, buf, (len - i + 1));
				} else {
					printf("%s \033[%dD", buf, (len - i + 1));
				}
				break;
			case 0x7f:
				if (i <= 0) { break; } // cursor at left bottom, ignore backspace
				for (int j = (--i); j <= len - 1; j++) {
					buf[j] = buf[j + 1];
				}
				buf[--len] = 0;
				printf("\033[%dD%s \033[%dD", (i + 1), buf, (len - i + 1));
				break;
			case '\033':
				read(0, &temp, 1);
				if (temp == '[') {
					read(0, &temp, 1);
					if (temp == 'D') {
						if (i > 0) {
							i -= 1;
						} else {
							printf("\033[C");
						}
					} else if(temp == 'C') {
						if (i < len) {
							i += 1;
						} else {
							printf("\033[D");
						}
					} else if (temp == 'A') {
						printf("\033[B");
						if(curLine != 1) {
							// if (/*curLine != 0*/ (curLine - 1 + HISTORY_SIZE) % HISTORY_SIZE != hisCount) {
								buf[len] = '\0';
								// 位于 最后一行 还没存入history文件
								// 先保存到curIn 中
								if (curLine == hisContain + 1) {
									strcpy(curIn, buf);
								}
								if (i != 0) { printf("\033[%dD", i); }
								for (int j = 0; j < len; j++) { printf(" "); }
								if (len != 0) { printf("\033[%dD", len); }
								curLine --;
								if ((r = readPast((hisCount - (hisContain-curLine)-1 + HISTORY_SIZE) % HISTORY_SIZE, buf)) < 0) { printf("G");exit(); }
								printf("%s", buf);
								i = strlen(buf);
								len = i;
								// redirect cursor
						}
					} else if (temp == 'B') {
						buf[len] = '\0';
						if (i != 0) { printf("\033[%dD", i); }
						for (int j = 0; j < len; j++) { printf(" "); }
						if (len != 0) { printf("\033[%dD", len); }
						if (curLine +1 <= hisContain) {
							curLine++;
							if ((r = readPast((hisCount - (hisContain-curLine)-1 + HISTORY_SIZE) % HISTORY_SIZE, buf)) < 0) { printf("G");exit(); }
						} else {
							strcpy(buf, curIn);
							curLine = hisContain + 1;
						}
						printf("%s", buf);
						i = strlen(buf);
						len = i;
						// redirect cursor
						// debugf("curline = %d\n",curLine);
					}
				}
				break;
			case '\r':
			case '\n':
				buf[len] = '\0';
				// if (hisCount == 0) {
				// 	if ((r = touch("/.history")) != 0) { printf("err1\n"); exit(); } 
				// }
				// int hisFd;
				// if ((hisFd = open("/.mosh_history", O_RDWR)) < 0) { printf("err2\n"); exit(); }
				// if ((r = write(hisFd, buf, len)) != len) { printf("err3\n"); exit(); }
				// if ((r = write(hisFd, "\n", 1)) != 1) { printf("err4\n"); exit(); }
				// if ((r = close(hisFd)) < 0) { printf("err5\n"); exit(); }
				// hisBuf[(hisCount + 1) % HISTORY_SIZE] = len;
				hisBuf[hisCount] = len;
				// curLine = hisCount; // cannot 'curLine++', otherwise usable instrctions will be [0, curLine + 1]
				memset(curIn, '\0', sizeof(curIn));
				return;
			default:
				buf[len + 1] = 0;
				for (int j = len;j >= i + 1;j--)
				{
					buf[j] = buf[j-1];
				}
				buf[i++] = temp;
				if (interactive != 0) {
					printf("\033[%dD%s",i,buf);
					if((r = len++ + 1 - i) != 0) {
						printf("\033[%dD", r);
					}
				}else {
					len++;
				}
			break;
		}
		// if (buf[i] == '\b' || buf[i] == 0x7f) {
		// 	if (i > 0) {
		// 		i -= 2;
		// 	} else {
		// 		i = -1;
		// 	}
		// 	if (buf[i] != '\b') {
		// 		printf("\b");
		// 	}
		// }
		// if (buf[i] == '\r' || buf[i] == '\n') {
		// 	buf[i] = 0;
		// 	return;
		// }
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

// void readline(char *buf, u_int n) {
// 	int r;
// 	for (int i = 0; i < n; i++) {
// 		if ((r = read(0, buf + i, 1)) != 1) {
// 			if (r < 0) {
// 				debugf("read error: %d\n", r);
// 			}
// 			exit();
// 		}
// 		if (buf[i] == '\b' || buf[i] == 0x7f) {
// 			if (i > 0) {
// 				i -= 2;
// 			} else {
// 				i = -1;
// 			}
// 			if (buf[i] != '\b') {
// 				printf("\b");
// 			}
// 		}
// 		if (buf[i] == '\r' || buf[i] == '\n') {
// 			buf[i] = 0;
// 			return;
// 		}
// 	}
// 	debugf("line too long\n");
// 	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
// 		;
// 	}
// 	buf[0] = 0;
// }

char buf[1024];


void runcmd_conditional(char *s)
{
	char cmd_buf[1024];
	int cmd_buf_len = 0;
	char op;
	char last_op = 0;
	int r;
	int exit_status = 0;
	while(1)
	{
		op = '\0';
		int in_quotes = 0, in_backquotes = 0;
		while(*s)
		{
			if((*s) == '\"')
			{
				in_quotes = !in_quotes;
			}
			if((*s) == '`' &&  !in_quotes)
			{
				in_backquotes = !in_backquotes;
			}
			if((*s) == '|' && *(s+1) == '|' && !in_quotes && !in_backquotes)
			{
				cmd_buf[cmd_buf_len] = '\0';
				op = '|';
				s += 2;
				cmd_buf_len = 0;
				break;
			}
			else if(*s == '&' && *(s+1)=='&' && !in_quotes && !in_backquotes)
			{
				cmd_buf[cmd_buf_len] = '\0';
				op = '&';
				s += 2;
				cmd_buf_len = 0;
				break;
			}
			else if(*s == ';' && !in_quotes && !in_backquotes)
			{
				cmd_buf[cmd_buf_len] = '\0';
				op = ';';
				s+=1;
				cmd_buf_len = 0;
				break;
			}
			else
			{
				cmd_buf[cmd_buf_len++] = *s;
				cmd_buf[cmd_buf_len] = '\0';
				s++;
			}

		}

		if(last_op == 0 ||
				(last_op == '&' && exit_status == 0)||
				(last_op == '|' && exit_status != 0) ||
				(last_op == ';'))
		{
			int len = strlen(cmd_buf);
			int i;
			int background_exc = 0;
			for(i = len-1;i >= 1;i--){
				if(cmd_buf[i] == '&' && cmd_buf[i-1] != '&')
				{
					background_exc = 1;
					break;
				}
				if(strchr(WHITESPACE, cmd_buf[i]) != 0)
				{
					break;
				}
			}
			if((r = fork()) < 0) {
				user_panic("fork:  %d", r);
			}
			if( r == 0 )
			{
				exit_status = runcmd(cmd_buf,background_exc);
				if(!background_exc) {
					syscall_ipc_try_send(env->env_parent_id, exit_status, 0, 0);
					exit();
				}
			}
			else
			{
				if(!background_exc)	
				{
					syscall_ipc_recv(0);
					wait(r);
					exit_status = env->env_ipc_value;
				}
				else
				{
					syscall_ipc_recv(0);
					int child_pid = env->env_ipc_value;
					exit_status = 0;
				}
			}
		}
		last_op = op;

		if(op == '\0')
		{
			break;
		}
	}
}

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}

int main(int argc, char **argv) {
	int r;
	interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}


	int history_fd = -1;
	if ((history_fd = open("/.mosh_history", O_RDWR)) < 0) {
		if ((r = create("/.mosh_history", FTYPE_REG)) != 0) {
			debugf("canbit create /.mosh_history: %d\n", r);
        }
	}
	char history_buf[20][1024];
	// int hisCount = 0;
	hisContain = 0;
	curLine = hisContain + 1;

	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		
		// history
		strcpy(history_buf[hisCount], buf);
		if(hisCount + 1 == HISTORY_SIZE)hisFull = 1;
		hisCount = (hisCount + 1) % HISTORY_SIZE;

		hisContain = hisContain + 1;
		if(hisContain > HISTORY_SIZE)hisContain = HISTORY_SIZE;
		curLine = hisContain + 1;
		if ((history_fd = open("/.mosh_history", O_RDWR)) >= 0) {
			int i;
			// 每次从 hisCount + i 开始写入 .mosh_history 文件中
			for (i = 0; i < HISTORY_SIZE; i++) {
				char *history_cmd = history_buf[(hisCount + i) % HISTORY_SIZE];
				if (history_cmd[0] == '\0') {
					continue;
				}
				fprintf(history_fd, "%s\n", history_cmd);
			}
			fprintf(history_fd, "%c", -1);
			close(history_fd);
		}


		runcmd_conditional(buf);
		/*
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit();
		} else {
			wait(r);
		}
		*/
	}
	return 0;
}
