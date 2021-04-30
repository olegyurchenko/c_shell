/*----------------------------------------------------------------------------*/
/**
* @pkg test_shell
*/
/**
* Test shell functions.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2021.<br>
* started 29.04.2021  9:30:03<br>
* @pkgdoc test_shell
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef TEST_SHELL_H_1619677803
#define TEST_SHELL_H_1619677803
/*----------------------------------------------------------------------------*/
#include "c_sh.h"
#include "c_tty.h"

typedef struct  {
  TTY *tty;
  C_SHELL *sh;
} TEST_SHELL_DATA;

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

void ts_init(TEST_SHELL_DATA *);
const char* ts_keyword(void *arg, unsigned index);
int ts_exec(void *arg, int argc, char **argv);


#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*TEST_SHELL_H_1619677803*/

