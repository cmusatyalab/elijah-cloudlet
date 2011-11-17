#ifndef KRAH_LIB_H_
#define KRAH_LIB_H_

#include <stdio.h>
/*
 * Global Constant
 */
#define TRUE		1
#define FALSE		0
#define SUCCESS		1
#define FAIL		0
#define EMPTY		0

/*
 * Log Macro
 */

#define PRINT_OUT(fmt, args...) fprintf(stdout, "[%s][%d] "fmt, __FILE__, __LINE__, ##args);
#define PRINT_ERR(fmt, args...) fprintf(stderr, "[%s][%d] "fmt, __FILE__, __LINE__, ##args);

#endif
