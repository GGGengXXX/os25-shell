#include <types.h>

void *memcpy(void *dst, const void *src, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;

	if (((u_long)src & 3) != ((u_long)dst & 3)) {
		while (dst < max) {
			*(char *)dst++ = *(char *)src++;
		}
		return dstaddr;
	}

	while (((u_long)dst & 3) && dst < max) {
		*(char *)dst++ = *(char *)src++;
	}

	// copy machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = *(uint32_t *)src;
		dst += 4;
		src += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst++ = *(char *)src++;
	}
	return dstaddr;
}

void *memset(void *dst, int c, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;
	u_char byte = c & 0xff;
	uint32_t word = byte | byte << 8 | byte << 16 | byte << 24;

	while (((u_long)dst & 3) && dst < max) {
		*(u_char *)dst++ = byte;
	}

	// fill machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = word;
		dst += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(u_char *)dst++ = byte;
	}
	return dstaddr;
}

size_t strlen(const char *s) {
	int n;

	for (n = 0; *s; s++) {
		n++;
	}

	return n;
}

char *strcpy(char *dst, const char *src) {
	char *ret = dst;

	while ((*dst++ = *src++) != 0) {
	}

	return ret;
}

const char *strchr(const char *s, int c) {
	for (; *s; s++) {
		if (*s == c) {
			return s;
		}
	}
	return 0;
}

int strcmp(const char *p, const char *q) {
	while (*p && *p == *q) {
		p++, q++;
	}

	if ((u_int)*p < (u_int)*q) {
		return -1;
	}

	if ((u_int)*p > (u_int)*q) {
		return 1;
	}

	return 0;
}


char *strcat(char *dest, const char *src) {
    // 1. 保存目标字符串起始地址（用于返回值）
    char *ptr = dest;
    
    // 2. 找到 dest 的结尾（即 '\0' 的位置）
    while (*dest != '\0') {
        dest++;
    }
    
    // 3. 将 src 的内容复制到 dest 末尾
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    
    // 4. 添加终止空字符
    *dest = '\0';
    
    // 5. 返回目标字符串起始地址
    return ptr;
}



char *strstr(const char *haystack, const char *needle) {
    if (needle == NULL || *needle == '\0') {
        return (char *)haystack;
    }

    for (int i = 0; haystack[i] != '\0'; i++) {
        // 剩余长度不足，提前终止
        int j = 0;
        while (needle[j] != '\0' && haystack[i + j] != '\0' && haystack[i + j] == needle[j]) {
            j++;
        }
        // 检查是否完全匹配
        if (needle[j] == '\0') {
            return (char *)(haystack + i);
        }
        // 如果剩余长度不足，提前结束外层循环
        if (haystack[i + j] == '\0') {
            break;
        }
    }
    return NULL;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s) {
        // 正向拷贝：dest 在 src 之前，安全从头到尾复制
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        // 反向拷贝：dest 在 src 之后，需从尾到头复制
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    // 如果 d == s，则源和目标相同，无需任何操作
    return dest;
}


void replace_substr(char *str, const char *old_sub, const char *new_sub) {
    if (!str || !old_sub || *old_sub == '\0') {
        // 源串或待替换串为空则不做任何操作
        return;
    }
    if (!new_sub) {
        new_sub = "";  // NULL 与空串同理，表示删除
    }

    size_t old_len = strlen(old_sub);
    size_t new_len = strlen(new_sub);
    char *p = str;

    // 不断查找并替换
    while ((p = strstr(p, old_sub))) {
        size_t tail_len = strlen(p + old_len);  // 剩余部分长度

        if (new_len > old_len) {
            // 向右移动，为插入 new_sub 腾出空间
            memmove(p + new_len, p + old_len, tail_len + 1);
        } else if (new_len < old_len) {
            // 向左移动，收缩空间（new_len==0 时即删除）
            memmove(p + new_len, p + old_len, tail_len + 1);
        }
        // 复制 new_sub（当 new_len==0 时相当于不复制）
        memcpy(p, new_sub, new_len);
        // 跳过刚插入（或删除）部分，继续搜索下一个
        p += new_len;
    }
}


#define WHITESPACE " \t\r\n"

static int is_whitespace(char c) {
    /* 检查字符 c 是否在 WHITESPACE 定义的集合中 */
    const char *p = WHITESPACE;
    while (*p) {
        if (c == *p) return 1;
        p++;
    }
    return 0;
}

void trim_inplace(char *str) {
    char *start, *end;

    if (!str || !*str) return;  /* 空指针或空串直接返回 */

    /* 1. 右修剪：移动 end 指针到最后一个非空白 */
    end = str + strlen(str) - 1;
    while (end >= str && is_whitespace(*end)) {
        end--;
    }
    /* 截断尾部空白 */
    *(end + 1) = '\0';

    /* 2. 左修剪：移动 start 指向第一个非空白 */
    start = str;
    while (*start && is_whitespace(*start)) {
        start++;
    }
    /* 如果全是空白，则置空字符串 */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}