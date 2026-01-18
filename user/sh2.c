#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()`"
#define true 1
#define false 0
#define N 1024
#define MAX_NODE_NUM 128
#define FILENAME_LEN 128
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define stderr 2

#define INDENT_STEP 2


// history
const int HISTORY_SIZE = 20;
static int curLine, hisCount, hisContain;
static int hisBuf[1024];
char history_buf[1024][1024];

// common func
int isalpha(char c){return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');}
int isdigit(char c){return c >= '0' && c <= '9';}
int isalnum(char c){return isalpha(c) || isdigit(c);}

int origin_envid = -1;

char sys_vars_namebuf[20][30];
char sys_vars_valbuf[20][30];

int _cd(int, char **);
int _exit(int, char **);
int _parent_envid(int, char **);
int _envid(int ,char **);
int _declare(int argc, char **argv);
int _unset(int argc, char **argv);
int _queryvar(int argc, char **argv);
int _list_var(int argc, char **argv);
int _history(int argc, char **argv);

struct BuiltinCmd {
    const char *name;
    int (*func)(int, char **);
};

static struct BuiltinCmd builtin_cmds[] = {
    {"cd", _cd},
    {"envid", _envid},
    {"parent_env", _parent_envid},
    {"exit", _exit},
    {"declare", _declare},
    {"unset",_unset},
    {"queryvar", _queryvar},
    {"listvar",_list_var},
    {"history",_history},
    // {"pwd", _pwd},
    // {"history", _history},
    // {"declare", _declare},
    // {"unset", _unset},
    // {"exit", _exit},
    {0, 0}  // Sentinel
};

// ======================
// 词法分析器 (Lexer)
// ======================

typedef enum {
    TOKEN_END,      // 输入结束
    TOKEN_WORD,     // 普通单词
    TOKEN_PIPE,     // |
    TOKEN_AND,      // &&
    TOKEN_OR,       // ||
    TOKEN_SEMI,     // ;
    TOKEN_LESS,     // <
    TOKEN_GREAT,    // >
    TOKEN_DGREAT,   // >>
    TOKEN_BACKTICK  // `
} TokenType;


void recognize_tokentype(TokenType type, char *call_name);

typedef struct {
    TokenType type;
    char value[1000];
} Token;
// 可被分配的字符缓冲区
char alloc_string[10][1000];
char alloc_string_id;
char *alloc_string_func();

// 遇到反引号
int in_backtick = 0;
// 反引号占位符号
char backtick_takeplace[1000][2];
int backtick_takcplace_alloc_id;

char *alloc_backtick_takeplace();


int token_count = 0;
int current_token = 0;
Token tokens[100];

// 分词函数
void tokenize(char *input);

static int interactive;
// 输入缓冲
char buf[1024];

// 起始语句
void print_mos_start();
// 终端缓冲读入
void readline(char *buf, u_int n);

// 放入token
// 编号  type  字符串地址  截断位置
void pushToken(int idx, TokenType type, char * value, int len);

// sh 参数错误
void sh_usage(void);

// 获取当前 token
Token* peek_token();

// 消耗当前 token
Token* next_token();

Token* malloc_token();

// 检查 token 类型是否匹配
int match_token(TokenType type);

// ======================
// 抽象语法树 (AST)
// ======================

typedef enum {
    NODE_COMMAND,   // 简单命令
    NODE_PIPE,      // 管道命令
    NODE_AND,       // &&
    NODE_OR,        // ||
    NODE_SEQ,       // ;
    NODE_REDIR_IN,  // <
    NODE_REDIR_OUT, // >
    NODE_REDIR_APP, // >>
    NODE_BACKTICK   // 反引号
} NodeType;



typedef struct BackTickNode{
    struct ASTNode *src;
    char val[N];
    struct BackTickNode *lnk;
}BackTickNode;

typedef struct ASTNode {
    NodeType type;
    char *argv[MAX_NODE_NUM];
    int argc;
    char *filename;
    struct ASTNode *left;
    struct ASTNode *right;
    BackTickNode *backtick_command_lst;
} ASTNode;

// BackTickNode
BackTickNode backnode_arr[N];
int backnode_alloc_id;

// ASTNode
ASTNode astnode_arr[N];
int ast_alloc_id;

// 创建一个BackNode
BackTickNode *create_backTickNode();
// 创建一个ASTNode
ASTNode *create_node(NodeType type);

// 解析命令序列
ASTNode *parse_command();

// 解析条件执行
ASTNode *parse_and_or();

// 解析简单命令
ASTNode *parse_simple_command();

// 输出语法树
void print_ast_debug(ASTNode *node, int indent);



// ===========
// 初始化
// ===========

void init_shell();

// 执行单条指令之前
void init_for_cmd();

// ======================
// 执行器 (Executor)
// ======================

// 递归执行 AST
int execute_ast(ASTNode *node);
// 执行重定向
int execute_redirection(ASTNode *node);
// 执行反引号命令
char *execute_backtick(ASTNode *node);
// 执行管道命令
int execute_pipeline(ASTNode *node);
// 执行简单命令
int execute_command(ASTNode *node);
// 替换反引号中的内容
void resolve_backticks(ASTNode *ast);

// ======================
// 环境变量
// ======================
// 在sh.c开头定义环境变量结构
#define MAX_VAR_LEN 16
#define MAX_VARS 50

typedef struct {
    char name[MAX_VAR_LEN + 1];
    char value[MAX_VAR_LEN + 1];
    int is_export;      // 是否为环境变量
    int is_readonly;    // 是否为只读变量
} ShellVar;

// 扩展Shell状态结构
typedef struct {
    // ... 原有成员 ...
    ShellVar vars[MAX_VARS];
    int var_count;
}ShellState;

ShellState shell_state;

// 添加环境变量
int add_variable(const char *name, const char *value, int is_export, int is_readonly);

// 变量名验证
int is_valid_var_name(const char *name);

// token中的变量名替换
void replace_token_var();

// 将var 保存到 sys_var_buf 返回个数
int save_var2buf();

void init_history();

void save_curline_history(char *buf);

int readPast(int target, char *code);

int is_blank(char *str);

int main(int argc, char ** argv)
{
    int r;
    interactive = iscons(0);
    int echocmds = 0;

    init_shell();
    print_mos_start();

    // 解析命令中的选项
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		sh_usage();
	}
	ARGEND


    // 调用sh 参数处理
    if (argc > 1) {
		sh_usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}

    

    // shell 主循环
    while (true)
    {
        // 交互式
		if (interactive) {
			printf("\n$ ");
		}
        readline(buf, sizeof buf);
        if(is_blank(buf))continue;
        save_curline_history(buf);
        // printf("curline %d hiscount %d hisContain %d\n",curLine,hisCount,hisContain);
        init_for_cmd();
        tokenize(buf);
        replace_token_var();
        // for(int i = 0;i < token_count;i++)
        // {
        //     printf("[%s]\n",tokens[i].value);
        // }
        // token 指针置 0
        current_token = 0;
        ASTNode *ast = parse_command();
        
        
        if(ast == NULL)continue;
        resolve_backticks(ast);
        int status = execute_ast(ast);
    }
}

void print_mos_start()
{
    printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2025                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
}

void readline(char *buf, u_int n)
{
    char curIn[1024];
    int r, len = 0;
    // 用户每次输入的字符
    char temp = 0;

    // i 是光标位置，len是指令长度，n是指令限制的长度
    for (int i = 0; len < n;)
    {
        // 读到temp 如果出错返回
        if ((r = read(0, &temp, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
            // 这里是父进程，出错直接 g
			exit(1);
		}

        // 处理得到的 temp
        switch (temp) {
            // 如果是 backspace 键的话
            case 0x7f:
            case '\b':
                // 光标最左，没什么好做的了
                if (i <= 0)
                {
                    break;
                }
                // 把光标右侧的东西全部左移
                for (int j = (--i); j <= len-1; j++) {
                    buf[j] = buf[j + 1];
                }
                buf[--len] = 0;
                printf("\033[%dD%s \033[%dD", (i + 1), buf, (len - i + 1));
                break;
            case '\x01':  /* Ctrl‑A：跳至行首 */
                while (i > 0) { printf("\033[D"); i--; }
                break;
            case '\x05':  /* Ctrl‑E：跳至行尾 */
                while (i < len) { printf("\033[C"); i++; }
                break;
            case '\x0b':  /* Ctrl‑K：删除光标至行尾 */
            // case '\x02':
                buf[i] = '\0';
                if (interactive) {
                    printf("\033[%dD%s",i,buf);
                    for(int k = 0;k < len - i;k++)printf(" ");
                    printf("\033[%dD",len-i);
                    // printf("\033[K");
                }
                len = i;
                break;
            case '\x15':  /* Ctrl‑U：删除行首至光标 */
            // case '\x02':
                if (interactive) {
                    printf("\033[%dD", i);  // 光标回到行首

                    // 重绘 buf+i 开始的内容（即右边内容），并清尾巴
                    printf("%s", buf + i);

                    // 清除尾部旧字符残留
                    int n = i;
                    for (int k = 0; k < n; k++) putchar(' ');
                    printf("\033[%dD", len);  // 回到行首
                }

                // 删除左侧内容
                memmove(buf, buf + i, i);
                len -= i;
                buf[len] = '\0';
                i = 0;
                break;
            case '\x17':  /* Ctrl‑W：删除前一个 word */
            // case '\x02':
            {
                int j = i;
                while (j > 0 && buf[j - 1] == ' ') j--;        // 跳过空格
                while (j > 0 && buf[j - 1] != ' ') j--;        // 跳过前一个单词

                if (interactive) {
                    int old_len = len;

                    printf("\033[%dD", i - j);                 // 移动光标到删除起点
                    memmove(buf + j, buf + i, len - i);        // 向左覆盖
                    buf[old_len - (i - j)] = '\0';             // 临时终止字符串
                    printf("%s", buf + j);                     // 重绘后面内容

                    // 多出的部分手动覆盖为空格
                    int clear_len = old_len - (j + (len - i)); // = i - j
                    for (int k = 0; k < clear_len; ++k) putchar(' ');

                    printf("\033[%dD", old_len - j);           // 光标回到 j
                } else {
                    memmove(buf + j, buf + i, len - i);
                }

                len -= (i - j);
                i = j;
                buf[len] = '\0';
                break;
            }
            case '\033':
                read(0, &temp, 1);
                if (temp == '[') {
                    read(0, &temp, 1);
                    // 向左
                    if (temp == 'D') {
                        if (i > 0) {
                            i-=1;
                        } else {
                            printf("\033[C");
                        }
                    }
                    // 向右
                    else if(temp == 'C') {
                        if (i < len) {
                            i += 1;
                        } else {
                            printf("\033[D");
                        }
                    }
                    // 向上 
                    else if (temp == 'A')
                    {
                        // 先 按回来
                        printf("\033[B");
                        if( curLine != hisContain + 1)
                        {
                            save_line_history(buf,(hisCount - (hisContain - curLine) - 1 + HISTORY_SIZE) % HISTORY_SIZE);
                        }
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
                            if ((r = readPast((hisCount - (hisContain-curLine)-1 + HISTORY_SIZE) % HISTORY_SIZE, buf)) < 0) { printf("G");exit(1); }
                            printf("%s", buf);
                            i = strlen(buf);
                            len = i;
                            // redirect cursor
						}
                    }
                    // 向下
                    else if(temp == 'B') 
                    {
                        if( curLine != hisContain + 1)
                        {
                            save_line_history(buf,(hisCount - (hisContain - curLine) - 1 + HISTORY_SIZE) % HISTORY_SIZE);
                        }
                        if(curLine != hisContain + 1)
                        {
                            buf[len] = '\0';
                            if (i != 0) { printf("\033[%dD", i); }
                            for (int j = 0; j < len; j++) { printf(" "); }
                            if (len != 0) { printf("\033[%dD", len); }
                            if (curLine +1 <= hisContain) {
                                curLine++;
                                if ((r = readPast((hisCount - (hisContain-curLine)-1 + HISTORY_SIZE) % HISTORY_SIZE, buf)) < 0) { printf("G");exit(1); }
                            } else {
                                strcpy(buf, curIn);
                                curLine = hisContain + 1;
                            }
                            printf("%s", buf);
                            i = strlen(buf);
                            len = i;
                        }
                    }
                }
                break;
            case '\r':
            case '\n':
                buf[len] = '\0';
                if(!is_blank(buf))hisBuf[hisCount] = len;
                // hisBuf[hisContain] = len;
                memset(curIn, '\0', sizeof(curIn));
                return;
            default:
                buf[len + 1] = '\0';
                // 在 i 处插入一个新的字符
                for(int j = len; j >= i+1;j--)
                {
                    buf[j] = buf[j - 1];
                }
                buf[i++] = temp;//插入 i向后移动
                if (interactive != 0) {
                    printf("\033[%dD%s",i,buf);
                    if((r = len++ + 1 - i) != 0) {
                        printf("\033[%dD",r);
                    }
                } else {
                    len++;
                }
            break;
        }
    }
    debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

void pushToken(int idx, TokenType type, char * value, int len)
{
    tokens[idx].type = type;
    if(value)
    {
        strcpy(tokens[idx].value,value);
        tokens[idx].value[len] = '\0';
    }
    else
    {
        tokens[idx].value[0] = '\0';
    }
}

void tokenize(char *input)
{
    char *start = input;
    char *current = input;
    int in_word = 0;

    token_count = 0;

    while (*current) {
        if(*current == '#')
        {
            *current = '\0';
            break;
        }
        if (strchr(WHITESPACE, *current)) {
            if (in_word) {
                token_count++;
                in_word = 0;
            }
        } 
        else if(strchr(SYMBOLS, *current))
        {
            if (in_word) token_count++;
            token_count++;
            in_word = 0;
        } 
        else {
            in_word = 1;
        }
        current++;
    }
    if (in_word) token_count++;

    current = input;
    int token_index = 0;
    in_word = 0;
    start = input;
    while( *current )
    {
        if (strchr(WHITESPACE, *current)) {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current-start);
                in_word = 0;
            }
        } 
        else if (*current == '|' && *(current+1) == '|') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_OR, current, 2);
            current++; // 跳过第二个字符
        } 
        else if (*current == '|') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current-start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_PIPE, current, 1);
        } 
        else if (*current == '&' && *(current+1) == '&') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_AND, current, 2);
            current++; // 跳过第二个字符
        } 
        else if (*current == ';') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_SEMI, current, 1);
        } else if (*current == '<') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_LESS, current, 1);
        } else if (*current == '>' && *(current+1) == '>') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_DGREAT, current, 2);
            current++; // 跳过第二个字符
        } else if (*current == '>') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_GREAT, current, 1);
        } else if (*current == '`') {
            if (in_word) {
                pushToken(token_index++, TOKEN_WORD, start, current - start);
                in_word = 0;
            }
            pushToken(token_index++, TOKEN_BACKTICK, current, 1);
        } else {
            if (!in_word) {
                start = current;
                in_word = 1;
            }
        }
        current++;
    }
    if (in_word) {
        pushToken(token_index++, TOKEN_WORD, start, current - start);
        in_word = 0;
    }
    // || && 会合并两个token
    if(token_index < token_count)
    {
        pushToken(token_index, TOKEN_END, NULL, 0);
        token_count = token_index + 1;
    }
}


void sh_usage(void)
{
	printf("usage: sh [-ix] [script-file]\n");
	exit(1);
}

Token *peek_token()
{
    if (current_token < token_count)
    {
        return &tokens[current_token]; 
    }
    pushToken(token_count,TOKEN_END,NULL, 0);
    return &tokens[token_count];
}

Token *next_token()
{
    if (current_token < token_count)
    {
        return &tokens[current_token++]; 
    }
    pushToken(token_count,TOKEN_END,NULL, 0);
    return &tokens[token_count];
}

// 检查 token 类型是否匹配
int match_token(TokenType type) {
    if (current_token < token_count && tokens[current_token].type == type) {
        current_token++;
        return 1;
    }
    return 0;
}

ASTNode *create_node(NodeType type)
{
    ASTNode *node = &astnode_arr[ast_alloc_id++];
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    return node;
}

void init_for_cmd()
{
    // token
    token_count = 0;
    current_token = 0;
    // Backnode
    backnode_alloc_id = 0;
    // Ast
    ast_alloc_id = 0;
    // backtick
    backtick_takcplace_alloc_id = 0;
    // string alloc
    alloc_string_id = 0;

}

ASTNode *create_command_node(char **argv, int argc) {
    ASTNode *node = create_node(NODE_COMMAND);
    for(int i = 0;i < argc;i++,argv++)
    {
        node->argv[i] = *argv;
    }
    node->argc = argc;
    return node;
}


ASTNode *parse_simple_command()
{
    char *argv[MAX_NODE_NUM];
    int argc = 0;
    BackTickNode *btk_lnk_list = NULL;
    while (true) 
    {
        // '\0' | && || ;
        Token *token = peek_token();
        if (token->type == TOKEN_END || 
            token->type == TOKEN_PIPE || 
            token->type == TOKEN_AND || 
            token->type == TOKEN_OR || 
            token->type == TOKEN_SEMI) {
            break;
        }
        if (token->type == TOKEN_LESS ||
        token->type == TOKEN_GREAT ||
        token->type == TOKEN_DGREAT)
        {
            next_token();

            Token * file_token = next_token();
            if(file_token->type != TOKEN_WORD)
            {
                fprintf(stderr, "Syntax error: expected filename after redirection\n");
                return NULL;
            }

            ASTNode *redir_node = create_node(
                token->type == TOKEN_LESS ? NODE_REDIR_IN :
                token->type == TOKEN_GREAT ? NODE_REDIR_OUT : NODE_REDIR_APP
            );
            redir_node->filename = file_token->value;
            // 将之前的命令挂到 当前节点的左边
            // 然后返回当前节点
            if(argc > 0)
            {
                ASTNode *cmd_node = create_command_node(argv,argc);
                cmd_node->backtick_command_lst = btk_lnk_list;
                redir_node->left = cmd_node;
                return redir_node;
            }
            else
            {
                fprintf(stderr, "Syntax error: redirection without command\n");
                return NULL;
            }
        }
        else if(token->type == TOKEN_BACKTICK)
        {
            if(in_backtick)
            {
                if(argc == 0)
                {
                    return NULL;
                }
                return create_command_node(argv,argc);
            }
            in_backtick = 1;
            next_token();
            ASTNode *backtick_node = parse_command();

            if(!backtick_node)
            {
                fprintf(stderr, "Syntax error: invalid command in backtick\n");
                return NULL;
            }

            BackTickNode* btnode = create_backTickNode();
            btnode->src = backtick_node;
            BackTickNode *cur = btk_lnk_list, *pre = btk_lnk_list;
            // btnode->lnk = btk_lnk_list;
            if(cur == NULL)
            {
                btk_lnk_list = btnode;
            }
            else
            {
                while(cur)
                {
                    pre = cur;
                    cur = cur->lnk;
                }
                pre->lnk = btnode;
            }

            // btk_lnk_list = btnode;
            // argv[argc++] = alloc_backtick_takeplace();
            if(!match_token(TOKEN_BACKTICK))
            {
                fprintf(stderr, "Syntax error: expected closing backtick\n");
                return NULL;
            }
            else in_backtick = 0;
            strcpy(token->value,"`");
            argv[argc++] = token->value;
        }
        else
        {
            // 普通单词
            next_token(); // 消耗 token
            argv[argc++] = token->value;
        }
    }

    if (argc == 0)
    {
        return NULL;
    }
    ASTNode* res = create_command_node(argv, argc);
    res->backtick_command_lst = btk_lnk_list;
    return res;
}


// 解析管道命令
ASTNode *parse_pipeline() {
    ASTNode *left = parse_simple_command();
    if (!left) return NULL;

    // 如果没有 | 直接就 返回 left
    // 有管道 也有可能有多个管道
    // 取到管道 + 右命令
    // 挂载左右节点，并将管道作为新的left 继续循环
    while (match_token(TOKEN_PIPE)) {
        ASTNode *right = parse_simple_command();
        // 有管道 但是管道右边为空
        if (!right) {
            fprintf(stderr, "Syntax error: expected command after pipe\n");
            return left;
        }

        ASTNode *pipe_node = create_node(NODE_PIPE);
        pipe_node->left = left;
        pipe_node->right = right;
        left = pipe_node;
    }

    return left;
}

ASTNode *parse_and_or()
{
    ASTNode *left = parse_pipeline();
    if (!left) return NULL;

    // 如果没有 || 或者&& 则直接返回
    // 如果有的话，挂载左右节点，当前节点成为新的左节点

    while(1)
    {
        if (match_token(TOKEN_AND))
        {
            ASTNode *right = parse_pipeline();
            if (!right) 
            {
                fprintf(stderr, "Syntax error: expected command after &&\n");
                return left;
            }

            ASTNode *and_node = create_node(NODE_AND);
            and_node->left = left;
            and_node->right = right;
            left = and_node;
        }
        else if(match_token(TOKEN_OR))
        {
            ASTNode *right = parse_pipeline();
            if (!right)
            {
                fprintf(stderr, "Syntax error: expected command after ||\n");
                return left;
            }

            ASTNode *or_node = create_node(NODE_OR);
            or_node->left = left;
            or_node->right = right;
            left = or_node;
        }
        else
        {
            break;
        }
    }
    return left;
}

ASTNode * parse_command()
{
    ASTNode *left = parse_and_or();
    if(!left) return NULL;

    while (match_token(TOKEN_SEMI))
    {
        ASTNode *right = parse_and_or();
        if (!right)
        {
            return left;
        }

        ASTNode *seq_node = create_node(NODE_SEQ);
        seq_node->left = left;
        seq_node->right = right;
        left = seq_node;
    }

    return left;
}

void recognize_tokentype(TokenType type, char *call_name) {
    switch (type) {
        case TOKEN_END:
            strcpy(call_name, "TOKEN_END");
            break;
        case TOKEN_WORD:
            strcpy(call_name, "TOKEN_WORD");
            break;
        case TOKEN_PIPE:
            strcpy(call_name, "TOKEN_PIPE");
            break;
        case TOKEN_AND:
            strcpy(call_name, "TOKEN_AND");
            break;
        case TOKEN_OR:
            strcpy(call_name, "TOKEN_OR");
            break;
        case TOKEN_SEMI:
            strcpy(call_name, "TOKEN_SEMI");
            break;
        case TOKEN_LESS:
            strcpy(call_name, "TOKEN_LESS");
            break;
        case TOKEN_GREAT:
            strcpy(call_name, "TOKEN_GREAT");
            break;
        case TOKEN_DGREAT:
            strcpy(call_name, "TOKEN_DGREAT");
            break;
        case TOKEN_BACKTICK:
            strcpy(call_name, "TOKEN_BACKTICK");
            break;
        default:
            strcpy(call_name, "UNKNOWN_TOKEN");
            break;
    }
}


void recognize_nodetype(NodeType type, char *call_name) {
    switch (type) {
        case NODE_COMMAND:
            strcpy(call_name, "NODE_COMMAND");
            break;
        case NODE_PIPE:
            strcpy(call_name, "NODE_PIPE");
            break;
        case NODE_AND:
            strcpy(call_name, "NODE_AND");
            break;
        case NODE_OR:
            strcpy(call_name, "NODE_OR");
            break;
        case NODE_SEQ:
            strcpy(call_name, "NODE_SEQ");
            break;
        case NODE_REDIR_IN:
            strcpy(call_name, "NODE_REDIR_IN");
            break;
        case NODE_REDIR_OUT:
            strcpy(call_name, "NODE_REDIR_OUT");
            break;
        case NODE_REDIR_APP:
            strcpy(call_name, "NODE_REDIR_APP");
            break;
        case NODE_BACKTICK:
            strcpy(call_name, "NODE_BACKTICK");
            break;
        default:
            strcpy(call_name, "UNKNOWN_NODE");
            break;
    }
}


// 递归打印语法树
void print_ast_debug(ASTNode *node, int indent) {
    if (!node) return;

    // 输出缩进
    for (int i = 0; i < indent; i++) {
        putchar(' ');
    }

    // 获取类型名
    char type_name[32];
    recognize_nodetype(node->type, type_name);
    printf("Node Type: %s", type_name);

    // 如果是命令，输出命令参数
    if (node->type == NODE_COMMAND) {
        printf(", argc = %d, argv = [", node->argc);
        for (int i = 0; i < node->argc; i++) {
            printf("\"%s\"", node->argv[i]);
            if (i != node->argc - 1) printf(", ");
        }
        printf("]");
        BackTickNode *start = node->backtick_command_lst;
        while(start)
        {
            debugf("\n--backtick son--\n");
            print_ast_debug(start->src,indent + 2);
            start = start->lnk;
        }
    }

    // 如果是重定向节点，输出文件名
    if (node->type == NODE_REDIR_IN || node->type == NODE_REDIR_OUT || node->type == NODE_REDIR_APP) {
        printf(", filename = \"%s\"", node->filename);
    }

    putchar('\n');

    // 递归子节点
    print_ast_debug(node->left, indent + INDENT_STEP);
    print_ast_debug(node->right, indent + INDENT_STEP);
}

BackTickNode *create_backTickNode()
{
    memset(&backnode_arr[backnode_alloc_id], 0, sizeof(BackTickNode));
    return &backnode_arr[backnode_alloc_id++];
}

int execute_ast(ASTNode *node)
{
    if(!node) return 0;

    switch (node->type)
    {
        case NODE_COMMAND:
            return execute_command(node);
        // |
        case NODE_PIPE:
            return execute_pipeline(node);
        // &&
        case NODE_AND:
        {
            int left_status = execute_ast(node->left);
            if (left_status == 0)
            {
                return execute_ast(node->right);
            }
            return left_status;   
        }
        // ||
        case NODE_OR: 
        {
            int left_status = execute_ast(node->left);
            if (left_status != 0)
            {
                return execute_ast(node->right);
            }
            return left_status;
        }

        case NODE_SEQ:
        {
            execute_ast(node->left);
            execute_ast(node->right);
        }

        case NODE_REDIR_IN:
        case NODE_REDIR_OUT:
        case NODE_REDIR_APP:
        {
            return execute_redirection(node);
        }
        case NODE_BACKTICK:
        {
            debugf("backtick node should be resolved during parsing\n");
            fprintf(stderr, "Error: backtick node should be resolved during parsing\n");
            return 1;
        }
        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            return 1;
    }

}

// 执行重定向
int execute_redirection(ASTNode *node)
{
    int pid = fork();
    int r, fd;
    if(pid == 0)
    {
        if(node->type == NODE_REDIR_IN)
        {
            if((fd = open(node->filename, O_RDONLY)) < 0) {
				debugf("failed to open %s\n", node->filename);
				exit(1);
			}
            if((r = dup(fd, 0)) < 0) {
				debugf("failed to duplicate file to <stdin>\n");
				exit(1);
			}
        }
        else if(node->type == NODE_REDIR_OUT)
        {
			if((fd = open(node->filename, O_WRONLY)) < 0) {
				fd = open(node->filename, O_CREAT);
			    if (fd < 0) {
        			debugf("error in open %s\n", node->filename);
					exit(1);
				}
			}
			close(fd);
			if ((fd = open(node->filename, O_TRUNC)) < 0) {
        		debugf("error in open %s\n", node->filename);
				exit(1);
			}
			close(fd);
			if ((fd = open(node->filename, O_WRONLY)) < 0) {
        		debugf("error in open %s\n", node->filename);
				exit(1);				
			}

			if((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit(1);
			}
        }
        else if(node->type == NODE_REDIR_APP)
        {
            // 建文件
			if((fd = open(node->filename, O_RDONLY)) < 0) {
				fd = open(node->filename, O_CREAT);
			    if (fd < 0) {
        			debugf("error in open %s\n", node->filename);
					exit(1);
				}
			}
			close(fd);
			// >>
			if ((fd = open(node->filename, O_WRONLY | O_APPEND)) < 0) {
        		debugf("error in open %s\n", node->filename);
				exit(1);				
			}

			if((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit(1);
			}
        }
        close(fd);
        // print_ast_debug(node->left, 0);
        r = execute_ast(node->left);
        exit(r);
    }
    else if(pid > 0)
    {
        u_int caller;
        r =  ipc_recv(&caller, 0, 0);
        return r;
    }
}

// 执行反引号命令
char *execute_backtick(ASTNode *node)
{
    return tokens[0].value;
}
// 执行管道命令
int execute_pipeline(ASTNode *node)
{
    int p[2];

    int r = pipe(p);

    if(r < 0)
    {
        debugf("failed to create pipe\n");
        exit(1);
    }

    
    int left_exit,right_exit;
    int left_pid = fork();
    if(left_pid == 0)
    {
        dup(p[1], 1);
        close(p[1]);
        close(p[0]);
        left_exit = execute_ast(node->left);
        exit(left_exit);
    }

    int right_pid = fork();
    if(right_pid == 0)
    {
        dup(p[0],0);
        close(p[0]);
        close(p[1]);

        right_exit = execute_ast(node->right);
        exit(right_exit);
    }
    close(p[1]);
    close(p[0]);
    wait(left_pid);
    wait(right_pid);
    return (left_exit | right_exit);
}
// 执行简单命令
int execute_command(ASTNode *node)
{
    int r,son_r;
    for (int i = 0; builtin_cmds[i].name; i++) {
        if (node->type == NODE_COMMAND && strcmp(node->argv[0], builtin_cmds[i].name) == 0) {
            r = builtin_cmds[i].func(node->argc, node->argv);
            return r;
        }
    }
    
    int pid = fork();
    if (pid == 0)
    {
        int child = spawn(node->argv[0], node->argv);

        if(child < 0)
        {
            char cmd_b[1024];
            int cmd_len = strlen(node->argv[0]);
            strcpy(cmd_b, node->argv[0]);
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
            child = spawn(cmd_b,node->argv);
        }
        close_all();
        if(child >= 0)
        {
            u_int caller;
            son_r = ipc_recv(&caller, 0, 0);
        }
        else 
        {
            debugf("spawn %s: %d\n", node->argv[0], child);
        }
        exit(son_r);
    }
    else if(pid > 0)
    {
        u_int caller;
        r = ipc_recv(&caller, 0, 0);
        wait(pid);
    }
    else
    {
        fprintf(stderr,"fork failed\n");
        return 1;
    }
    return r;
}

int _cd(int argc, char **argv)
{
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
		cat2Path(tmp,argv[1],cur);
	}
	else
	{
        cur[0] = '/',cur[1] = '\0';
	}
    if ((r = stat(cur, &st)) < 0) { 
		printf("cd: The directory '%s' does not exist\n",argv[1]);
		return 1; 
	}
	if (!st.st_isdir) {
		printf("%s is not a directory\n", cur);
		return 1;
	}
    if ((r = chdir(cur)) < 0) { printf("set rpath error\n");exit(1); }
    return 0;
}

void init_shell()
{
    // history
    init_history();
    // 设置工作目录为根目录
    char tmp[30];
    syscall_get_rpath(tmp);
    if(tmp[0] == '\0')
    {
        tmp[0] = '/',tmp[1] = '\0';
        chdir(tmp);
    }
    
    for(int i = 0;i < 100;i ++)
    {
        backtick_takeplace[i][0] = '`';
        backtick_takeplace[i][1] = '\0';
    }
}

void resolve_backticks(ASTNode *ast)
{
    BackTickNode *cur = ast->backtick_command_lst;
    int argc_i = 0;
    while(cur != NULL)
    {
        // char *outbuf = alloc_string_func();
        char outbuf[1024];
        int p[2];
        if(pipe(p) < 0) {
            debugf("failed to create pipe\n");
            exit(1);
        }
        int pid = fork();
        if(pid == 0)
        {
            dup(p[1], 1);
            close(p[0]);
            close(p[1]);
            int r = execute_ast(cur->src);
            exit(r);
        }
        close(p[1]);
        memset(outbuf, 0, sizeof(outbuf));
        int offset = 0;
        int read_num = 0;
        while((read_num = read(p[0], outbuf + offset, sizeof(outbuf) - offset - 5)) > 0) {
            offset += read_num;
        }
        // if (read_num < 0)
        // {
        //     debugf("error in resolve backtick\n");
        //     exit(1);
        // }
        close(p[0]);
        u_int caller;
        wait(pid);

        while(argc_i < ast->argc)
        {
            char *s = ast->argv[argc_i];
            if(strcmp(s,"`") == 0)
            {
                strcpy(ast->argv[argc_i],outbuf);
            }
            argc_i++;
        }
        cur = cur->lnk;
        // ast->backtick_command_lst = cur;
    }
    if (ast->left)resolve_backticks(ast->left);
    if (ast->right) resolve_backticks(ast->right);
}

char *alloc_backtick_takeplace()
{
    return backtick_takeplace[backtick_takcplace_alloc_id++];
}

char *alloc_string_func()
{
    return alloc_string[alloc_string_id++];
}

int _exit(int argc, char **argv)
{
    // syscall_sync_penv_var();
    int id = syscall_getenvid();
    // printf("env %d\n",id);
    // printf("bye~\n");
    exit(0);
}

int _envid(int argc, char **argv)
{
    printf("now env[%d]\n",syscall_getenvid());
    return 0;
}

int _parent_envid(int argc, char **argv)
{
    if(argc == 2)
    {
        printf("branch\n");
        char* nums = argv[1];
        printf("branch %s\n",nums);
        
        int x = 0;
        for(int i =0; argv[1][i]; i++)
        {
            x = (10 * x) + (argv[1][i] ^ 48);
        }
        printf("ok\n");
        printf("x 's parent%d\n",x);
        printf("parent env[%d]\n",syscall_get_env_parent_id(x));
        return 0;
    }
    printf("parent env[%d]\n",syscall_get_env_parent_id(0));
    return 0;
}

int add_variable(const char *name, const char *value, int is_export, int is_readonly)
{
    // 检查变量名有效性
    if (!is_valid_var_name(name)) {
        fprintf(stderr, "Invalid variable name: %s\n", name);
        return 1;
    }

    // 检查长度限制
    if (strlen(name) > MAX_VAR_LEN || strlen(value) > MAX_VAR_LEN) {
        fprintf(stderr, "Variable name or value too long\n");
        return -1;
    }

    for(int i = 0;i < shell_state.var_count;i++)
    {
        if (strcmp(shell_state.vars[i].name, name) == 0)
        {
            if (shell_state.vars[i].is_readonly)
            {
                debugf(stderr, "%s: readonly variable\n", name);
                return -1;
            }

            strcpy(shell_state.vars[i].value, value);
            shell_state.vars[i].is_export = is_export;
            return 0;
        }
    }

    if (shell_state.var_count >= MAX_VARS)
    {
        fprintf(stderr, "Too many variables\n");
        return -1;
    }

    ShellVar *var = &shell_state.vars[shell_state.var_count++];
    strcpy(var->name, name);
    strcpy(var->value,value);
    var->is_export = is_export;
    var->is_readonly = is_readonly;
    return 0;
}

// 变量名验证
int is_valid_var_name(const char *name) {
    if (!name || !*name) return 0;
    
    // 首字符必须是字母或下划线
    if (!isalpha(name[0]) && name[0] != '_') {
        return 0;
    }
    
    // 后续字符可以是字母、数字或下划线
    for (int i = 1; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '_') {
            return 0;
        }
    }
    
    return 1;
}

int _declare(int argc, char **argv)
{
    int export_flag = 0;
    int readonly_flag = 0;
    int fl = 0;
    // 解析命令中的选项
	ARGBEGIN {
	case 'x':
		export_flag = 1;
        fl = 1;
		break;
	case 'r':
		readonly_flag = 1;
        fl = 1;
		break;
	default:
		printf("usage: declare [-xr] [variable]\n");
    	exit(1);
	}
	ARGEND
    char name_buf[100],val_buf[100];

    int r;
    if(argc == 0)
    {
        char output_buf[500];
        output_buf[0] = '\0';
        r = syscall_list_env_var(output_buf);
        printf("%s",output_buf);
        return r;
    }

    // int argc_i = fl ? 1 : 0;
    int argc_i = 0;
    char *p = argv[argc_i];
    int id = 0;
    while((*p) != '\0' && (*p) != '=')
    {
        name_buf[id++] = (*p);
        p++;
    }
    name_buf[id] = '\0';
    if(*p != '\0')
    {
        while((*p) == '=')p++;
        id = 0;
        while((*p) != '\0')
        {
            val_buf[id++] = (*p);
            p++;
        }
        val_buf[id] = '\0';
        // printf("val buf = %s\n",val_buf);
        r = syscall_declare_env_var(name_buf, val_buf, export_flag, readonly_flag);
    }
    else 
    {
        r = syscall_declare_env_var(name_buf, NULL, export_flag, readonly_flag);
    }
    if(r != 0)
    {
        printf("%s is readonly\n",name_buf);
    }
    return r;
}

int _unset(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: unset [variable]\n");
    	return 1;
    }
    char unset_name[20];
    strcpy(unset_name,argv[1]);
    int r = syscall_unset_env_var(unset_name);
    return r;
}

int _queryvar(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: queryvar [variable]\n");
        return 1;
    }
    char name[100], val[100];
    strcpy(name, argv[1]);
    val[0] = '\0';
    int r = syscall_find_env_var(name, val);
    if(r == 0)
    {
        printf("%s=%s",name,val);
    }
    return r;
}

void replace_token_var()
{
    current_token = 0;
    int var_cnt = save_var2buf();
    char *v;
    while(true)
    {
        Token *token = peek_token();
        if (token->type == TOKEN_END) {
            break;
        }
        if(token->type == TOKEN_WORD && token->value[0] == '$')
        {
            for(int i = 0;i < var_cnt;i++)
            {
                char tmp_name[100];
                tmp_name[0] = '$', tmp_name[1] = 0;
                strcat(tmp_name, sys_vars_namebuf[i]);
                replace_substr(token->value, tmp_name, sys_vars_valbuf[i]);
            }
        }
        next_token();
    }
    for(int i = 0;i < token_count;i ++)
    {
        Token *token = &tokens[i];
        if(token->type == TOKEN_WORD && token->value[0] == '\0')
        {
            for(int j = i;j < token_count - 1;j++)
            {
                tokens[j] = tokens[j + 1];
            }
            token_count--;
        }
    }
    current_token = 0;
}

int save_var2buf()
{
    char *nameptr[30];
    char *valptr[30];
    for(int i = 0;i < 20;i++)
    {
        sys_vars_namebuf[i][0] = sys_vars_valbuf[i][0] = '\0';
        nameptr[i] = sys_vars_namebuf[i];
        valptr[i] = sys_vars_valbuf[i];
    }
    int cnt;
    syscall_list_env_var1(nameptr, valptr,&cnt);
    return cnt;
}

int _list_var(int argc, char **argv)
{
    char *nameptr[30];
    char *valptr[30];
    for(int i = 0;i < 20;i++)
    {
        sys_vars_namebuf[i][0] = sys_vars_valbuf[i][0] = '\0';
        nameptr[i] = sys_vars_namebuf[i];
        valptr[i] = sys_vars_valbuf[i];
    }
    int cnt;
    syscall_list_env_var1(nameptr, valptr,&cnt);
    for(int i = 0;i < cnt;i++)
    {
        printf("%s=%s\n",sys_vars_namebuf[i], sys_vars_valbuf[i]);
    }
}

void init_history()
{
    int history_fd = -1,r;
	if ((history_fd = open("/.mos_history", O_RDWR)) < 0) {
		if ((r = create("/.mos_history", FTYPE_REG)) != 0) {
			debugf("canbit create /.mos_history: %d\n", r);
        }
	}
    // history
	hisContain = 0;
	curLine = hisContain + 1;
}

void save_curline_history(char *buf)
{
    int history_fd;
    curLine = hisContain + 1;
    if(history_buf[(hisCount - 1 + HISTORY_SIZE) % HISTORY_SIZE][0] != '\0' && 
    strcmp(history_buf[(hisCount - 1 + HISTORY_SIZE) % HISTORY_SIZE], buf) == 0)return;
    
    strcpy(history_buf[hisCount], buf);
    hisCount = (hisCount + 1) % HISTORY_SIZE;
    
    hisContain = hisContain + 1;
    curLine = hisContain + 1;
    if(hisContain > HISTORY_SIZE)hisContain = HISTORY_SIZE;
    if ((history_fd = open("/.mos_history", O_RDWR)) >= 0) {
        int i;
        // 每次从 hisCount + i 开始写入 .mos_history 文件中
        for (i = 0; i < HISTORY_SIZE; i++) {
            char *history_cmd = history_buf[(hisCount + i) % HISTORY_SIZE];
            if (history_cmd[0] == '\0') {
                continue;
            }
            fprintf(history_fd, "%s\n", history_cmd);
        }
        // fprintf(history_fd, "%c", -1);
        close(history_fd);
    }
}

void save_line_history(char *buf, int line)
{
    // printf("\n save %s line %d\n",buf,line);
    int history_fd;
    strcpy(history_buf[line],buf);
    hisBuf[line] = strlen(buf);
    // printf("below\n");
    if ((history_fd = open("/.mos_history", O_RDWR)) >= 0)
    {
        int i;
        for(i = 0;i < HISTORY_SIZE;i++)
        {
            char *history_cmd = history_buf[(hisCount + i) % HISTORY_SIZE];
            if(history_cmd[0] == '\0')
            {
                continue;
            }
            fprintf(history_fd, "%s\n", history_cmd);
        }
        // fprintf(history_fd, "%c", -1);
        close(history_fd);
    } 
}

int readPast(int target, char *code) {
	// history
	int r, fd, spot = 0;
	char buff[10240];
	if ((fd = open("/.mos_history", O_RDWR)) < 0 && (fd != 0)) {
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

int _history(int argc, char **argv)
{
	if(argc != 1)
	{
		return 1;
	}
    int history_fd = -1;
    // 打开 .mos_history 文件
    if ((history_fd = open("/.mos_history", O_RDONLY)) < 0) {
        debugf("can not open /.mos_history: %d\n", history_fd);
        return 2;
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
        if(*(history_buf + i) == '\0')
        {
            *(history_buf+i) = '\0';
            break;
        }
    }
    // 把文件内容print出来
    printf("%s",history_buf);
    close(history_fd);
    return 0;
}

int is_blank(char *str)
{
    for(int i = 0;str[i];i++)
    {
        if(strchr(WHITESPACE,str[i])==NULL)
        {
            return 0;
        }
    }
    return 1;
}
