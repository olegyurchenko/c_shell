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

/**Divide source string to lexems*/
int sh_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size);
/**Lexems printf*/
void lex_printf(C_SHELL *sh, const LEX_ELEM *args, int size);
/**Make *argv[] from lexems*/
int sh_make_argv(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv);
/**Print arggumsens *argv[]*/
void argv_printf(C_SHELL *sh, int argc, char **argv);
/**Make substitutions: $xxx, $(cmd) ${xxx} `xx`*/
int sh_make_substs(C_SHELL *sh, const char *src, unsigned size, char *buffer, unsigned buffer_size);

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_PARS_H_1620449831*/

