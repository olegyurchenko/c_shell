/*----------------------------------------------------------------------------*/
/**
* @pkg c_sh
*/
/**
* Simple shell.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2020.<br>
* started 08.12.2020 11:29:31<br>
* @pkgdoc c_sh
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef C_SH_H_1607419771
#define C_SH_H_1607419771
/*----------------------------------------------------------------------------*/
#include <stdarg.h>

//Error codes
#define SHELL_OK 0
#define SHELL_ERR_MALLOC (-1) //Memory allocation error
#define SHELL_ERR_INVALID_ARG (-2) //Invalid argument error
#define SHELL_ERR_COMMAND_NOT_FOUND (-3) //Command not found
#define SHELL_ERR_NOT_IMPLEMENT (-4) //Command not implemented
#define SHELL_ERR_INVALID_OP (-5) //Invalid operation
#define SHELL_STACK_ERROR (-6) //Stack error
#define SHELL_ERROR_OPEN_FILE (-7) //Error open file
#define SHELL_EXIT (-100000)

#define SHELL_DEBUG_VAR_NAME "__DEBUG__"
#define LEX_DEBUG_VAR_NAME "__DEBUG_LEX__"
#define PARS_DEBUG_VAR_NAME "__DEBUG_PARS__"
#define CACHE_DEBUG_VAR_NAME "__DEBUG_CACHE__"

typedef struct C_SHELL_TAG C_SHELL;
typedef int (*SHELL_PRINT_CB)(void *arg, int c);
typedef int (*SHELL_EXEC_CB)(void *arg, int argc, char **argv);
typedef int (*SHELL_STEP_CB)(void *arg, int argc, char **argv);

typedef enum {
  SHELL_STDIN = 0,
  SHELL_STDOUT = 1,
  SHELL_STDERR = 2,
} SHELL_STREAM_ID;

typedef enum {
  SHELL_IN = 0,
  SHELL_OUT = 1,
  SHELL_APPEND = 2,
  SHELL_FIFO = 3
} SHELL_STREAM_MODE;


typedef struct SHELL_STREAM_HANDLER {
  void *data;
  int (*_open)(void *data, const char* name, SHELL_STREAM_MODE mode);
  int (*_close)(void *data, int f);
  int (*_read)(void *data, int f, void* buf, unsigned size);
  int (*_write)(void *data, int f, const void* buf, unsigned size);
} SHELL_STREAM_HANDLER;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**Allocate new shell*/
C_SHELL *shell_alloc(void);
void shell_free(C_SHELL *sh);
/**Reset the shell: clear vars, stack ...*/
void shell_reset(C_SHELL *sh);
/**Set print (character output proc) callback*/
void shell_set_print_cb(C_SHELL *sh, SHELL_PRINT_CB cb, void *cb_arg);
/**Set exec callback - callback to execute - useful action*/
void shell_set_exec_cb(C_SHELL *sh, SHELL_EXEC_CB cb, void *cb_arg);
/**Set step callback - callback to execute - useful action*/
void shell_set_step_cb(C_SHELL *sh, SHELL_STEP_CB cb, void *cb_arg);
/**Set handler for streams*/
void shell_set_stream_handler(C_SHELL *sh, SHELL_STREAM_HANDLER *h);

/**Get error string by return code*/
const char *shell_err_string(C_SHELL *sh, int err);

/**Print char*/
int shell_fputc(C_SHELL *sh, SHELL_STREAM_ID f, int c);
/**Print string*/
int shell_fputs(C_SHELL *sh, SHELL_STREAM_ID f, const char *text);
/**printf style print*/
int shell_fprintf(C_SHELL *sh, SHELL_STREAM_ID f, const char *format, ...);
/**printf style print*/
int shell_vfprintf(C_SHELL *sh, SHELL_STREAM_ID f, const char *format, va_list ap);
/**Write data to shell stream*/
int shell_write(C_SHELL *sh, SHELL_STREAM_ID f,  const void *data, unsigned size);
/**Read data from shell stdin*/
int shell_read(C_SHELL *sh, void *data, unsigned size);


/**Print char to STDOUT*/
int shell_putc(C_SHELL *sh, int c);
/**Print string to STDOUT*/
int shell_puts(C_SHELL *sh, const char *text);
/**printf style print to STDOUT*/
int shell_printf(C_SHELL *sh, const char *format, ...);
/**printf style print to STDOUT*/
int shell_vprintf(C_SHELL *sh, const char *format, va_list ap);

/**Set shell variable*/
int shell_set_var(C_SHELL *sh, const char *name, const char *value);
/**Set shell variable*/
int shell_set_int_var(C_SHELL *sh, const char *name, int value);
/**Get shell variable: If not foud return ""*/
const char *shell_get_var(C_SHELL *sh, const char *name);
/**Get shell variable: If found return SHELL_OK (0), if not - return SHELL_ERR_INVALID_ARG (-2)*/
int shell_get_int_var(C_SHELL *sh, const char *name, int *value);
/**Return stack dept*/
int shell_stack_size(C_SHELL *sh);
/**Push string to shell - useful action*/
int shell_rx(C_SHELL *sh, const char *str);
/**Terminate shell execution*/
void shell_terminate(C_SHELL *sh);
/**return 1 if stream is a SHELL_PRINT_CB*/
int shell_isaprint(C_SHELL *sh, SHELL_STREAM_ID stream_id);


#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_H_1607419771*/

