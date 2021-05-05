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

int sh_stream_open(void *data, const char* name, SHELL_STREAM_MODE mode);
int sh_stream_close(void *data, int f);
int sh_stream_read(void *data, int f, void* buf, unsigned size);
int sh_stream_write(void *data, int f, const void* buf, unsigned size);

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_EMBED_H_1620193503*/

