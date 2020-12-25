/*----------------------------------------------------------------------------*/
/**
* @pkg nprintf
*/
/**
* nprintf - printf like function for use in embeded app with custom output.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2020.<br>
* started 08.12.2020 11:48:21<br>
* @pkgdoc nprintf
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef NPRINTF_H_1607420901
#define NPRINTF_H_1607420901
/*----------------------------------------------------------------------------*/
#include <stdarg.h>

typedef int (*nprintf_writer_t)(void *, const char *, int);

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

int nprintf(nprintf_writer_t writer, void *writer_arg, const char *fmt, ...);
int vnprintf(nprintf_writer_t writer, void *writer_arg, const char *fmt, va_list ap);

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*NPRINTF_H_1607420901*/

