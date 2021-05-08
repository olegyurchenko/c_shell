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
#include "c_sh.h"
#include "c_sh_int.h"
#include "c_sh_pars.h"
#include "c_sh_embed.h"
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
#include <stdio.h>
#include <ctype.h>
#else //STDLIB
#include "fm_sys.h"
#endif //STDLIB
/*----------------------------------------------------------------------------*/
#ifndef MALLOC
#define MALLOC malloc
#endif
#ifndef FREE
#define FREE free
#endif
/*----------------------------------------------------------------------------*/
#include "c_cache.h"
#include "loop_buff.h"
#include "nprintf.h"
/*----------------------------------------------------------------------------*/
/*Lexer must used in test!!!*/
int lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size)
{
  int count = 0, arg_size = 0, quote = 0;
  unsigned i;
  for(i = 0; i < size && count < (int) lex_size; i++) {

    if(src[i] == '\\') {
      i ++;
      continue;
    }

    if(src[i] == '"' || src[i] == '\'') {
      if(!quote) {
        if(arg_size) {
          dst[count].end = &src[i];
          count ++;
          arg_size = 0;
        }
        quote = src[i];

        dst[count].type = LEX_STR;
        dst[count].start = dst[count].end = &src[i];
        arg_size ++;
      }
      else
        if(src[i] == quote) {
          quote = 0;
          if(arg_size) {
            dst[count].end = &src[i + 1];
            count ++;
            arg_size = 0;
          }
        }
      continue;
    }

    if(quote)
      continue;

    if(src[i] == ' ' || src[i] == '\t') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }
      continue;
    }

    if(src[i] == ';' || src[i] == '\r' || src[i] == '\n') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }
      i ++;
      break;
    }

    if(src[i] == '#') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_COMMENT;
      dst[count].start = &src[i];
      i = size;
      dst[count].end = &src[i];
      count ++;
      arg_size = 0;
      break;
    }

    if(src[i] == '=') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_ASSIGN;
      dst[count].start = &src[i];
      if(src[i + 1] == '=') {
        dst[count].type = LEX_EQ;
        i ++;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //=

    if(src[i] == '>') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_GT;
      dst[count].start = &src[i];
      if(src[i + 1] == '=') {
        i ++;
        dst[count].type = LEX_GE;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //> >=

    if(src[i] == '<') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_LT;
      dst[count].start = &src[i];
      if(src[i + 1] == '=') {
        i ++;
        dst[count].type = LEX_LE;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //< <=

    if(src[i] == '&') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_AMP;
      dst[count].start = &src[i];
      if(src[i + 1] == '&') {
        i ++;
        dst[count].type = LEX_AND;
        if(count) {
          i --;
          break;
        }
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //& &&

    if(src[i] == '|') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_VERBAR;
      dst[count].start = &src[i];
      if(src[i + 1] == '|') {
        i ++;
        dst[count].type = LEX_OR;
        if(count) {
          i --;
          break;
        }
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //& &&

    if(src[i] == '!') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_NOT;
      dst[count].start = &src[i];
      if(src[i + 1] == '=') {
        dst[count].type = LEX_NE;
        i ++;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //! !=

    if(src[i] == '[') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_LSQB;
      dst[count].start = &src[i];
      if(src[i + 1] == '[') {
        i ++;
        dst[count].type = LEX_LDSQB;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //[ [[

    if(src[i] == ']') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_RSQB;
      dst[count].start = &src[i];
      if(src[i + 1] == ']') {
        i ++;
        dst[count].type = LEX_RDSQB;
      }
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //] ]]

    if(src[i] == '(') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_LPAREN;
      dst[count].start = &src[i];
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //(

    if(src[i] == ')') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_RPAREN;
      dst[count].start = &src[i];
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //(

    if(src[i] == '`') {
      if(arg_size) {
        dst[count].end = &src[i];
        count ++;
        arg_size = 0;
      }

      dst[count].type = LEX_BACKTICK;
      dst[count].start = &src[i];
      dst[count].end = &src[i + 1];
      count ++;
      continue;
    } //(

    if(!arg_size) {
      dst[count].type = LEX_STR;
      dst[count].start = &src[i];
      dst[count].end = &src[i];
      arg_size ++;
    }

  }

  if(arg_size) {
    dst[count].end = &src[i];
    count ++;
  }

  *end = &src[i];
  return count;
}
/*----------------------------------------------------------------------------*/
static const char *lex_name(LEX_CODES l)
{
  switch (l) {
    case LEX_ASSIGN:    return "ASSIGN";
    case LEX_NOT:       return "NOT";
    case LEX_AMP:       return "AMP";
    case LEX_VERBAR:    return "VERBAR";
    case LEX_AND:       return "AND";
    case LEX_OR:        return "OR";
    case LEX_EQ:        return "EQ";
    case LEX_NE:        return "NE";
    case LEX_GT:        return "GT";
    case LEX_GE:        return "GE";
    case LEX_LT:        return "LT";
    case LEX_LE:        return "LE";
    case LEX_LSQB:      return "LSQB";
    case LEX_LDSQB:     return "LDSQB";
    case LEX_RSQB:      return "RSQB";
    case LEX_RDSQB:     return "RDSQB";
    case LEX_LPAREN:    return "LPAREN";
    case LEX_RPAREN:    return "RPAREN";
    case LEX_BACKTICK:    return "BACKTICK";
    case LEX_STR:      return "STR";
    case LEX_COMMENT:   return "COMMENT";
  }
  return "???";
}
/*----------------------------------------------------------------------------*/
void lex_print(C_SHELL *sh, const LEX_ELEM *args, int size)
{
  int i;
  const char *p;
  shell_fprintf(sh, SHELL_STDERR, "LEX:argc:%d {\n", size);
  for(i = 0; i < size; i++) {
    shell_fprintf(sh, SHELL_STDERR, "%16s%8s`", lex_name(args[i].type), "");
    for(p = args[i].start; p < args[i].end; p++) {
      shell_fputc(sh, SHELL_STDERR, *p);
    }
    shell_fputc(sh, SHELL_STDERR, '\'');
    shell_fputc(sh, SHELL_STDERR, '\n');
  }
  shell_fprintf(sh, SHELL_STDERR, "%10s\n", "}");
}
/*----------------------------------------------------------------------------*/
static char htoc(const char **src)
{
  unsigned char c = 0;
  const char *p;
  int i;
  p = *src;
  for(i = 0; i < 2; i++)
  {
    if(*p >= 'A' && *p <= 'Z')
    {
      c <<= 4;
      c |= (unsigned char)(*p - 'A' + 0xa);
    }
    else
    if(*p >= 'a' && *p <= 'z')
    {
      c <<= 4;
      c |= (unsigned char)(*p - 'a' + 0xa);
    }
    else
    if(*p >= '0' && *p <= '9')
    {
      c <<= 4;
      c |= (unsigned char)(*p - '0');
    }
    else
      break;
    p ++;
  }
  (*src) = p - 1;
  return (char) c;
}
/*----------------------------------------------------------------------------*/
static char otoc(const char **src)
{
  unsigned char c = 0;
  int i;
  const char *p;
  p = *src;
  for(i = 0; i < 3; i++)
  {
    if(*p >= '0' && *p <= '7')
    {
      c <<= 3;
      c |= (unsigned char )(*p - '0');
    }
    else
      break;
    p ++;
  }
  *src = p - 1;
  return (char) c;
}
/*----------------------------------------------------------------------------*/
unsigned string_prepare(C_SHELL *sh, const LEX_ELEM *src, char *buffer, unsigned buffer_size)
{
  const char *p, *end;
  unsigned i;
  int use_vars = 1;

  (void) sh;
  p = src->start;
  end = src->end;
  if(src->type == LEX_STR) {
    if(*src->start == '\'') {
      use_vars = 0;
      p = src->start + 1;
      end = src->end - 1;
    }
    else if(*src->start == '"') {
      p = src->start + 1;
      end = src->end - 1;
    }
  }


  for(i = 0; i < buffer_size && p < end; p++) {
    if(*p == '\\') {
      p ++;
      switch(*p)
      {
        case 'a':
          buffer[i ++] = '\a';
          break;
        case 'b':
          buffer[i ++] = '\b';
          break;
        case 'f':
          buffer[i ++] = '\f';
          break;
        case 'n':
          buffer[i ++] = '\n';
          break;
        case 'r':
          buffer[i ++] = '\r';
          break;
        case 't':
          buffer[i ++] = '\t';
          break;
        case 'v':
          buffer[i ++] = '\v';
          break;
        case 'x':
          p ++;
          buffer[i ++] = htoc(&p);
          break;
        case '0':
          buffer[i ++] = otoc(&p);
          break;
        default:
          buffer[i ++] = *p;
          break;

      }
      continue;
    }

    buffer[i ++] = *p;
  }


  if(i >= buffer_size)
    i = buffer_size - 1;
  buffer[i] = 0;
  return i;
}
/*----------------------------------------------------------------------------*/
int args_prepare(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv)
{
  static const char empty = 0;
  int i, ret = SHELL_OK;
  char *buffer;
  unsigned sz;

  for(i = 0; i < size; i++) {
    argv[i] = NULL;
  }

  buffer = (char *) cache_alloc(sh->cache, BUFFER_SIZE);
  if(buffer == NULL) {
    return SHELL_ERR_MALLOC;
  }

  do {

    for(i = 0; i < size; i++) {
      sz = string_prepare(sh, &args[i], buffer, BUFFER_SIZE);
      if(sz) {
        argv[i] = cache_alloc(sh->cache, sz + 1);
        if(argv[i] == NULL) {
          ret = SHELL_ERR_MALLOC;
          break;
        }
        memcpy(argv[i], buffer, sz);
        argv[i][sz] = 0;
      }
      else {
        argv[i] =(char *) &empty;
      }
    }

  } while (0);
  cache_free(sh->cache, buffer);

  if(ret != SHELL_OK) {
    for(i = 0; i < size; i++) {
      if(argv[i] != NULL) {
        cache_free(sh->cache, argv[i]);
        argv[i] = NULL;
      }
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------*/
void args_print(C_SHELL *sh, int argc, char **argv)
{
  int i;
  shell_fprintf(sh, SHELL_STDERR, "PARSER:argc:%d\n", argc);
  for(i = 0; i < argc; i++) {
    shell_fprintf(sh, SHELL_STDERR, "args[%d]:'%s'\n", i, argv[i]);
  }
  shell_fprintf(sh, SHELL_STDERR, "%10s\n", "}");
}
/*----------------------------------------------------------------------------*/
static int var_subst(C_SHELL *sh, const char *start, const char *end, char *buffer, unsigned buffer_size, unsigned *size)
{
  const char *value;
 char *name;

  *size = 0;

  if(start <= end) {
    return SHELL_OK;
  }

  name = (char *) cache_alloc(sh->cache, end - start + 1);
  if(name == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memcpy(name, start, end - start);
  name[end - start] = 0;
  value = sh_get_var(sh, name);
  cache_free(sh->cache, name);

  while(*value && *size < buffer_size) {
    buffer[*size] = *value;
    (*size) ++;
    value ++;
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int cmmd_subst(C_SHELL *sh, const char *start, const char *end, char *buffer, unsigned buffer_size, unsigned *size)
{
  int ret = SHELL_OK, f = 0, r, i;
  typedef struct {
    C_SHELL_PARSER parser;

  } command_substitution_t;

  command_substitution_t *data = NULL;

  *size = 0;

  if(sh->stream.ext_handler == NULL) {
    return SHELL_ERR_NOT_IMPLEMENT;
  }
  data = (command_substitution_t *) cache_alloc(sh->cache, sizeof(command_substitution_t));
  if(data == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(data, 0, sizeof(command_substitution_t));

  do {
    data->parser.arg0 = 0;
    data->parser.argc = 0;
    sh->parser = &data->parser;

    r = lexer(start, end-start, &end, data->parser.lex, MAX_LEXER_SIZE);
    if(r < 0) {
      break;
    }

    if(r) {
      if(data->parser.lex[r - 1].type == LEX_COMMENT)
        r --;
    }

    if(r) {
      data->parser.argc = r;

      r = args_prepare(sh, data->parser.lex, data->parser.argc, data->parser.argv);
      if(r < 0) {
        break;
      }

      if(sh_is_pars_debug_mode(sh)) {
        args_print(sh, data->parser.argc, data->parser.argv);
      }


      //Open output stream
      f = sh->stream.ext_handler->_open(sh->stream.ext_handler->data, "", SHELL_FIFO);
      if(f <= 0) {
        ret = f < 0 ? f : SHELL_ERROR_OPEN_FILE;
        break;
      }

      r = sh_exec1(sh, data->parser.argc, data->parser.argv, f);

      for(i = 0; i < MAX_LEXER_SIZE; i++) {
        if(data->parser.argv[i] != NULL) {
          cache_free(sh->cache, data->parser.argv[i]);
          data->parser.argv[i] = NULL;
        }
      }
    }

    if(SHELL_OK != ret) {
      break;
    }

    //Read command output
    while (*size < (buffer_size - 1)) {
      r = sh->stream.ext_handler->_read(sh->stream.ext_handler->data, f, &buffer[*size], 1);
      if(r < 0) {
        ret = r;
        break;
      }

      if(r == 0 || !buffer[*size]) {
        break;
      }

      if(buffer[*size] == '\n' || buffer[*size] == '\r') {
        buffer[*size] = ' ';
      }

      (*size) ++;
    }
    buffer[*size] = 0;

    if(SHELL_OK != ret) {
      break;
    }
    //Read command output end

  } while(0);

  if(f > 0) {
    sh->stream.ext_handler->_close(sh->stream.ext_handler->data, f);
  }
  cache_free(sh->cache, data);
  return ret;
}
/*----------------------------------------------------------------------------*/

