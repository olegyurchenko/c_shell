/*----------------------------------------------------------------------------*/
/**
* @pkg c_sh_pars
*/
/**
* Simple shell parser functions.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2021.<br>
* started 08.05.2021  7:57:11<br>
* @pkgdoc c_sh_pars
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef C_SH_PARS_H_1620449831
#define C_SH_PARS_H_1620449831
/*----------------------------------------------------------------------------*/
#include "c_sh_int.h"
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

int lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size);
void lex_print(C_SHELL *sh, const LEX_ELEM *args, int size);
unsigned string_prepare(C_SHELL *sh, const LEX_ELEM *src, char *buffer, unsigned buffer_size);
int args_prepare(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv);
void args_print(C_SHELL *sh, int argc, char **argv);
int make_substitutions(C_SHELL *sh, const char *src, unsigned size, char *buffer, unsigned buffer_size);

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_PARS_H_1620449831*/

