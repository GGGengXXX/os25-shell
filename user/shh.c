// 不断读取用户的命令输入，根据命令创建对应的进程，并实现进程间的通信
#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"
#define UP 'A'
#define DOWN 'B'
#define OTHER -1

int flag_next_is_condition;


char outbuf[20000];

// history
const int HISTORY_SIZE = 20;
static int curLine, hisCount, hisContain;
static int hisBuf[1024];
static int interactive;
static int test = 0;


void wait_child(int pid)
{
	wait(pid);
}

int useCD(int argc, char* argv) {
	int r;
	char tmp[1024];
	char cur[1024];
	struct Stat st = {0};

	syscall_get_rpath(tmp);
	if(argc > 2)
	{
		printf("Too many args for cd command\n");
		return 1;
	}
	if(argc > 1)
	{
		cat2Path(tmp,argv,cur);
	}
	else
	{
		strcpy(cur,tmp);
	}
	// 跳回根目录
	// if (argc == 1) {
	// 	cur[0] = '/';
	// } else if (argv[0] != '/') { // 如果不是根目录
	// 	// 如果是 ./xxx 统一转为 xxx
	// 	char *p = argv;
	// 	if (argv[0] == '.') { p += 2; }

	// 	syscall_get_rpath(cur);
	// 	int len1 = strlen(cur);
	// 	int len2 = strlen(p);
	// 	if (len1 == 1) { // cur: '/'
	// 		strcpy(cur + 1, p);
	// 	} else {         // cur: '/a'
	// 		cur[len1] = '/';
	// 		strcpy(cur + len1 + 1, p);
	// 		cur[len1 + 1 + len2] = '\0';
	// 	}
		
	// } else {
	// 	strcpy(cur, argv);
	// }
	if ((r = stat(cur, &st)) < 0) { 
		printf("cd: The directory '%s' does not exist\n",argv);
		return 1; 
	}
	if (!st.st_isdir) {
		printf("%s is not a directory\n", cur);
		return 1;
		// printf("5");exit();
	}
	if ((r = chdir(cur)) < 0) { printf("6");exit(); }
	// syscall_set_rpath(cur);
	return 0;
}

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
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}
	// 跳过 s 中空白字符
	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (*s == '"') {
		*p1 = s; // 从双引号开始begin
		do {
			s++;
		} while(*s && *s != '"');
		*s = 0;// 得到 "xxxxx 没有下引号
		s++;
		*p2 = s;
		return 'w';
	}

	if (*s == '`') {
		*p1 = s; // 从反引号开始begin
		do {
			s++;
		} while(*s && *s != '`');
		*s = 0;// 得到 `xxx 没有match的反引号
		s++;
		*p2 = s;
		return 'w';
	}

	if (*s == '>' && *(s + 1) == '>') {// 重定向符号
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'a';
	}

	if (*s == '|' && *(s + 1) == '|') {// 或者
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'O';
	}

	if (*s == '&' && *(s + 1) == '&') {// 并且
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'A';
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
	// 如果 s 有值 执行_gettoken后返回
	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	// 这里np1会在gettoken中改变
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		flag_next_is_condition = 0;
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		int my_env_id;
		switch (c) {
		case 0:
		// 结束符号，返回参数个数
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			// 把参数的char * 装进 argv 中
			argv[argc++] = t;
			break;
		case '<':
			// 如果 < 后面为空 报错
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			if((fd = open(t, O_RDONLY)) < 0) {
				debugf("failed to open %s\n", t);
				exit();
			}
			if((r = dup(fd, 0)) < 0) {
				debugf("failed to duplicate file to <stdin>\n");
				exit();
			}
			close(fd);
			// user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			if((fd = open(t, O_WRONLY)) < 0) {
				fd = open(t, O_CREAT);
			    if (fd < 0) {
        			debugf("error in open %s\n", t);
					exit();
				}
			}
			close(fd);
			if ((fd = open(t, O_TRUNC)) < 0) {
        		debugf("error in open %s\n", t);
				exit();
			}
			close(fd);
			if ((fd = open(t, O_WRONLY)) < 0) {
        		debugf("error in open %s\n", t);
				exit();				
			}

			if((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit();
			}
			close(fd);

			break;
		case 'a':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}

			// 建文件
			if((fd = open(t, O_RDONLY)) < 0) {
				fd = open(t, O_CREAT);
			    if (fd < 0) {
        			debugf("error in open %s\n", t);
					exit();
				}
			}
			close(fd);
			// >>
			if ((fd = open(t, O_WRONLY | O_APPEND)) < 0) {
        		debugf("error in open %s\n", t);
				exit();				
			}

			if((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit();
			}
			close(fd);

			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			if((r = pipe(p)) < 0) {
				debugf("failed to create pipe\n");
				exit();
			}
			if((*rightpipe = fork()) == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			// user_panic("| not implemented");
			break;
		// ';'已经在SYMBOLS中
		case ';':;
			my_env_id = fork();
			if (my_env_id < 0) {
				debugf("failed to fork in sh.c\n");
				exit();
			} else if (my_env_id == 0) { // 子进程执行左边
				return argc; 
			} else { // 父进程执行右边
				wait_child(my_env_id);
				return parsecmd(argv, rightpipe); 
			}
			break;
		case 'O':;
			my_env_id = fork();
			if (my_env_id < 0) {
				debugf("failed to fork in sh.c\n");
				exit();
			} else if (my_env_id == 0) { // 子进程 先执行左边的东西
				flag_next_is_condition = 1;
				return argc;
			} else {
				u_int caller;
				int res = ipc_recv(&caller, 0, 0);
				if (res == 0) { // 不需要执行后面的了
					while(1) {
						int op = gettoken(0, &t);
						if (op == 0) {
							return 0;
						} else if (op == 'A') {//除非遇到了 && 否则全部跳过
							return parsecmd(argv, rightpipe);
						}
					}
				} else {
					return parsecmd(argv, rightpipe);// flag = 1 接着执行后面的内容
				}
				// return res == 0 ? 0 : parsecmd(argv, rightpipe);
			}
			// argv[argc++] = "||";
			break;
		case 'A':;
			my_env_id = fork();
			if (my_env_id < 0) {
				debugf("failed to fork in sh.c\n");
				exit();
			} else if (my_env_id == 0) {// 执行左边的内容
				flag_next_is_condition = 1;
				return argc;
			} else {
				u_int caller;
				int res = ipc_recv(&caller, 0, 0);
				// wait(my_env_id);
				if (res != 0) {
						int op = gettoken(0, &t);
						if (op == 0) {
							return 0;
						} else if (op == 'O') {
							return parsecmd(argv, rightpipe);
						}
				} else {
					return parsecmd(argv, rightpipe);
				}
				// return res == 0 ? parsecmd(argv, rightpipe) : 0;
			}
			// argv[argc++] = "&&";
			break;
		}
	}

	return argc;
}

void strcat(char *a, char * b)
{
	char *p1 = a, *p2 = b;
	while((*p1))p1++;
	while((*p2))
	{
		*(p1++) = *(p2++);
	}
	*p1 = '\0';
}

void runcmd(char *s) {
	int exit_status = 0;
	char *s_begin = s;
	char s_copy[1025];
	strcpy(s_copy, s);

	// replaceBackquoteCommands(s);
	// 首先调用一次 gettoken，这将把 s 设定为要解析的字符串
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	// 调用 parsecmd 将完整的字符串解析。解析的参数返回到 argv，参数的数量返回为 argc
	int argc = parsecmd(argv, &rightpipe);
	// 无参数 无返回值
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;

	if (strcmp(argv[0],"ggengx") == 0)
	{
		test = 10086;
		ipc_send(syscall_get_env_parent_id(0),0,0,0);
		exit();
	}

	// modify for challenge
	// history
	if (strcmp(argv[0], "history") == 0) {
		int history_fd = -1;
		// 打开 .mosh_history 文件
		if ((history_fd = open("/.mosh_history", O_RDONLY)) < 0) {
			debugf("can not open /.mosh_history: %d\n", history_fd);
			exit();
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
		ipc_send(syscall_get_env_parent_id(0), 0, 0, 0);
		exit();
	}

	if (strcmp("cd", argv[0]) == 0) {
		int res = useCD(argc, argv[1]);
		if (rightpipe) 
		{
			wait(rightpipe);
		}
		ipc_send(syscall_get_env_parent_id(0), res, 0, 0);
		exit();
	}

	// if (strcmp(argv[0], "echo") == 0) {
		for (int i = 1; i < argc; ++i) {
			// 处理双引号
			if (argv[i][0] == '"') {
				for (int j = 0; argv[i][j] != '\0'; j++) {
					argv[i][j] = argv[i][j + 1];
				}
			}
			// 处理反引号
			else if (argv[i][0] == '`') {
				argv[i][0] = ' ';
				int p[2];
				if(pipe(p) < 0) {
					debugf("failed to create pipe\n");
					exit();
				}
				int my_env_id = fork();
				if (my_env_id < 0) {
					debugf("failed to fork in sh.c\n");
					exit();
				} else if (my_env_id == 0) { // 子进程执行反引号部分
					close(p[0]);
					dup(p[1], 1);
					close(p[1]);

					char temp_cmd[1024] = {0};	
					strcpy(temp_cmd, argv[i]);
					runcmd(temp_cmd);
					
				} else { // 父进程处理argv
					close(p[1]);
					memset(outbuf, 0, sizeof(outbuf));
					int offset = 0;
					int read_num = 0;
					while((read_num = read(p[0], outbuf + offset, sizeof(outbuf) - offset - 5)) > 0) {
						offset += read_num;
					}
					if (read_num < 0) {
						debugf("error in `\n");
						exit();
					}
					close(p[0]);
					u_int caller;
				int res = ipc_recv(&caller, 0, 0);
					wait_child(my_env_id);
					argv[i] = outbuf;
				}
			}
			// int tmp_i = 0;
			// while(strchr(WHITESPACE,argv[i][tmp_i++]));
			// tmp_i--;
			// int j;
			// for(j = 0;argv[i][tmp_i]!='\0';j++,tmp_i++)
			// {
			// 	argv[i][j] = argv[i][tmp_i];
			// }
			// argv[i][j] = argv[i][tmp_i];
			// tmp_i--;
			// if(strchr(WHITESPACE,argv[i][tmp_i]))
			// {
			// 	while(strchr(WHITESPACE,argv[i][tmp_i]))
			// 	{
			// 		tmp_i--;
			// 	}
			// 	argv[i][tmp_i+1]='\0';
			// }
		}
	// }


	int child = spawn(argv[0], argv);
	
	// 关闭当前进程的所有文件
	if( child < 0 )
	{
		char cmd_b[1024];
		int cmd_len = strlen(argv[0]);
		strcpy(cmd_b, argv[0]);
		if(cmd_b[cmd_len - 1] == 'b' && cmd_b[cmd_len - 2] == '.')
		{
			cmd_b[cmd_len - 2] = '\0';
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
	if (child >= 0) {
		u_int caller;
		int res = ipc_recv(&caller, 0, 0);
		if (flag_next_is_condition) {
			ipc_send(syscall_get_env_parent_id(0), res, 0, 0);
		}
		// wait(child);
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit();
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

int parseCD(char *buf) {
	char *p = buf;
	if (strlen(buf) < 2) { return 0; }
	if (*p == 'c' && *(p + 1) == 'd') {
		return 1;
	} else {
		for (int i = 0; i < strlen(buf) - 2; i++) {
			if (*(p + i) == ';' || *(p + i) == '&') {
				i++;
				while (*(p + i) == ' ') {
					i++;
				}
				if (i <= strlen(buf) - 2 && *(p + i) == 'c' && *(p + i + 1) == 'd') {
					return 1;
				}
			}
		}
	}
	return 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}

void print_hello_message();
void save_line(char *line);

int main(int argc, char **argv) {
	int r;
	interactive = iscons(0);
	int echocmds = 0;
	print_hello_message();
	// 解析命令中的选项
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
	
	syscall_set_rpath("/");

	int history_fd = -1;
	if ((history_fd = open("/.mosh_history", O_RDWR)) < 0) {
		if ((r = create("/.mosh_history", FTYPE_REG)) != 0) {
			debugf("canbit create /.mosh_history: %d\n", r);
        }
	}

	char history_buf[20][1024];
	hisContain = 0;
	curLine = hisContain + 1;
	
	for (;;) {
		// 交互式
		if (interactive) {
			char path[128] = {0};
			getcwd(path);
			// printf("\n%s$ ",path);
			printf("\n$ ");
		}

		// 读取一行
		readline(buf, sizeof buf);

		// 忽略空指令
		if (buf[0] == '\0') {
			continue;
		}


		// 忽略以 # 开头的注释
		if (buf[0] == '#') {
			continue;
		}
		for (int i = 0; i < strlen(buf); ++i) {
			if (buf[i] == '#') {
				buf[i] = '\0';
			}
		}

		// history
		strcpy(history_buf[hisCount], buf);
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
		
		// 在 echocmds 模式下输出读入的命令
		if (echocmds) {
			printf("# %s\n", buf);
		}
		// 调用 fork 复制了一个 Shell 进程
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}

		// if (parseCD(buf) == 0) {
		// 	if ((r = fork()) < 0) {
		// 		user_panic("fork: %d", r);
		// 	}
		// } else {
		// 	runcmd(buf);
		// 	continue;
		// }

		// if ((r = fork()) < 0) {
		// 		user_panic("fork: %d", r);
		// 	}

		if (r == 0) {
			// 子进程执行 runcmd 函数来运行命令，然后死亡
			printf("a\n");
			runcmd(buf);
			printf("b\n");
			exit();
		} else {
			// 原本的 Shell 进程则等待该新复制的进程结束。
			u_int caller;
			int res = ipc_recv(&caller, 0, 0);
			// printf("receive res %d\n",res);
			wait_child(r);
			printf("test = %d\n",test);
		}
	}
	return 0;
}
/*-------------------------------------------------------------------------------------*/

void print_hello_message(void) {
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
}
