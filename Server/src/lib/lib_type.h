#ifndef KRAH_LIB_H_
#define KRAH_LIB_H_

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
#define PRINT_OUT(...)					fprintf(stdout, __VA_ARGS__)
#define PRINT_ERR(...)					fprintf(stderr, __VA_ARGS__)

#endif
