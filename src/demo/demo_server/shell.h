/**
* Demo shell functions.
*/
/*----------------------------------------------------------------------------*/
#ifndef SHELL_H_1608977904
#define SHELL_H_1608977904
/*----------------------------------------------------------------------------*/
#include "c_sh.h"
#include "c_tty.h"

#define MAX_USER_LENGTH 32
typedef struct {
  int fd;
  C_SHELL *sh;
  TTY *tty;
  enum {
    LOGIN_MODE,
    PASSWORD_MODE,
    SHELL_MODE
  } mode;
  int watchdog;
  char user[MAX_USER_LENGTH];
  int terminated : 1;
  int telnet_settings : 1;
} SHELL_DATA;
/*----------------------------------------------------------------------------*/
typedef struct {
  int (*cb)(SHELL_DATA *data, int argc, char **argv);
  const char *name;
  const char *description;
  const char *help;
} SHELL_FUNCTION;
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/
  int shell_init(SHELL_DATA *data);
  void shell_destroy(SHELL_DATA *data);
#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*SHELL_H_1608977904*/

