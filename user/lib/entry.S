#include <asm/asm.h>

.text
EXPORT(_start)
// 加载了 argc 和 argv
	lw      a0, 0(sp)
	lw      a1, 4(sp)
	jal     libmain
