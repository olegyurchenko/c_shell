/*----------------------------------------------------------------------------*/
/**
* @pkg c_sh_embed
*/
/**
* Simple shell: built-in commands.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2021.<br>
* started 05.05.2021  8:45:03<br>
* @pkgdoc c_sh_embed
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef C_SH_EMBED_H_1620193503
#define C_SH_EMBED_H_1620193503
#include "c_sh.h"
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

int sh_embed_exec(C_SHELL *sh, int argc, char **argv);
/**Embeded functions*/
int sh_exit(C_SHELL *sh, int argc, char **argv);
int sh_break(C_SHELL *sh, int argc, char **argv);
int sh_continue(C_SHELL *sh, int argc, char **argv);
int sh_if(C_SHELL *sh, int argc, char **argv);
int sh_then(C_SHELL *sh, int argc, char **argv);
int sh_fi(C_SHELL *sh, int argc, char **argv);
int sh_else(C_SHELL *sh, int argc, char **argv);
int sh_elif(C_SHELL *sh, int argc, char **argv);
int sh_true(C_SHELL *sh, int argc, char **argv);
int sh_false(C_SHELL *sh, int argc, char **argv);
int sh_while(C_SHELL *sh, int argc, char **argv);
int sh_until(C_SHELL *sh, int argc, char **argv);
int sh_for(C_SHELL *sh, int argc, char **argv);
int sh_do(C_SHELL *sh, int argc, char **argv);
int sh_done(C_SHELL *sh, int argc, char **argv);
int sh_echo(C_SHELL *sh, int argc, char **argv);
int sh_test(C_SHELL *sh, int argc, char **argv);
int sh_set(C_SHELL *sh, int argc, char **argv);
int sh_assign(C_SHELL *sh, int argc, char **argv);
int sh_and(C_SHELL *sh, int argc, char **argv);
int sh_or(C_SHELL *sh, int argc, char **argv);

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_EMBED_H_1620193503*/

