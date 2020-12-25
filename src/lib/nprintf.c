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
#include "nprintf.h"
/*----------------------------------------------------------------------------*/
#undef STDLIB
#if !defined(NOSTDLIB)
#if defined (UNIX) || defined (LINUX) || defined (WIDOWS) || defined (WIN32)
  #define STDLIB
#endif //defined (UNIX) || defined (LINUX) || defined (WIDOWS) || defined (WIN32)
#endif //NOSTDLIB
/*----------------------------------------------------------------------------*/
#ifdef STDLIB
#include <stdlib.h>
#include <string.h>
#else //STDLIB
#include "fm_sys.h"
#endif //STDLIB
/*----------------------------------------------------------------------------*/
static int write_str(nprintf_writer_t writer, void *writer_arg, const char *src, int before, int after)
{
  int l, r = 0;
  char buf = ' ';
  l = strlen(src);
  while (l < before) {
    r += writer(writer_arg, &buf, 1);
    before --;
  }
  r += writer(writer_arg, src, l);
  while (l < after) {
    r += writer(writer_arg, &buf, 1);
    after --;
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int write_int(nprintf_writer_t writer, void *writer_arg, char spec, unsigned src, int width, char blank)
{
  const char ascii[] = "0123456789abcdef";
  const char ASCII[] = "0123456789ABCDEF";
  const char *table;
  int r = 0, base, i = 0, sign = 0, len;
  char buffer[20];

  i = sizeof(buffer);
  table = ascii;
  switch (spec) {
  default:
  case 'd':
  case 'i':
    if (src & 0x80000000u) {
      sign = 1;
      src = ~src + 1;
    }
    base = 10;
    break;
  case 'u':
    base = 10;
    break;
  case 'o':
    base = 8;
    break;
  case 'X':
    table = ASCII;
    base = 16;
    break;
  case 'x':
    base = 16;
    break;
  }

  if (!src)
    buffer[--i] = '0';
  else
    while (src && i) {
      buffer[--i] = table[src % base];
      src /= base;
    }
  if (sign && i)
    buffer[--i] = '-';
  len = sizeof(buffer) - i;
  if (width < 0) {
    width = - width;
    r += writer(writer_arg, &buffer[i], len);
    while (len < width) {
      r += writer(writer_arg, &blank, 1);
      width --;
    }
  }
  else {
    while (len < width) {
      r += writer(writer_arg, &blank, 1);
      width --;
    }
    r += writer(writer_arg, &buffer[i], len);
  }
  return r;
}
/*----------------------------------------------------------------------------*/
int nprintf(nprintf_writer_t writer, void *writer_arg, const char *fmt, ...)
{
  int r;
  va_list ap;
  va_start(ap, fmt);
  r = vnprintf(writer, writer_arg, fmt, ap);
  va_end(ap);

  return r;
}
/*----------------------------------------------------------------------------*/
int vnprintf(nprintf_writer_t writer, void *writer_arg, const char *fmt, va_list ap)
{
  char spec[20];
  int r = 0, i, brk;
  const char *fmt0;

  fmt0 = fmt;
  while (*fmt) {
    if (*fmt == '%') {
      r += writer(writer_arg, fmt0, fmt - fmt0);
      fmt ++;
      brk = 0;
      i = 0;
      spec[0] = '\0';
      while (*fmt && !brk) {
        brk = 1;
        switch (*fmt) {
        case '%':
          r += writer(writer_arg, fmt, 1);
          break;
        case 'c':
          spec[0] = va_arg(ap, int);
          r += writer(writer_arg, spec, 1);
          break;

        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
          r += write_int(writer, writer_arg, *fmt, va_arg(ap, unsigned), atoi(spec), spec[0] == '0' ? '0' : ' ');
          break;
        case 's':
          i = atoi(spec);
          r += write_str(writer, writer_arg, va_arg(ap, char*), i > 0 ? i : 0, i < 0 ? -i : 0);
          break;
        default:
          if (i < (int)sizeof(spec)) {
            spec[i++] = *fmt;
            spec[i] = '\0';
          }
          brk = 0;
          break;
        }
        fmt ++;
      }
      fmt0 = fmt;
      continue;
    }
    fmt ++;
  }

  if (fmt > fmt0)
    r += writer(writer_arg, fmt0, fmt - fmt0);
  return r;
}
/*----------------------------------------------------------------------------*/

