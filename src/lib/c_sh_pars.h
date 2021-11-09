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
int cmdline_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size);
/**Divide source string to lexems for test*/
int test_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size);
/**Lexems printf*/
void lex_printf(C_SHELL *sh, const LEX_ELEM *args, int size);
/**Make *argv[] from lexems*/
int sh_make_argv(C_SHELL *sh, C_SHELL_PARSER *parser);
/**Print arggumsens *argv[]*/
void argv_printf(C_SHELL *sh, int argc, char **argv);
/**Make substitutions: $xxx, $(cmd) ${xxx} `xx`*/
int sh_make_substs(C_SHELL *sh, const char *src, unsigned size, char *buffer, unsigned buffer_size);
/** String argument representation: 123=>123 11 22 => '11 22' */
int to_str(const char *src, char *dst, unsigned buffer_size);
/**Run arithmetic expression*/
int arithmetic(C_SHELL *sh, const char *src, unsigned size, char *buffer, unsigned buffer_size);


#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_PARS_H_1620449831*/

