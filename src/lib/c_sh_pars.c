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
#include "c_cache.h"

#if !defined(STDLIB) || defined(__BORLANDC__)
static int isblank(int c) {return c == ' ' || c == '\t' || c == '\r' || c == '\n';}
#endif

/**Divide source string to lexems*/
int cmdline_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size)
{
  int count = 0, arg_size = 0, quote = 0, lparen = 0;
  unsigned i;
  for(i = 0; i < size && src[i] && count < (int) lex_size; i++) {

    if(src[i] == '\\') {
      i ++;
      continue;
    }

    if(src[i] == '"' || src[i] == '\'' || src[i] == '`') {
      if(!quote) {
        /*
        if(arg_size) {
          dst[count].end = &src[i];
          count ++;
          arg_size = 0;
        }
        */
        quote = src[i];

        if(!arg_size) {
          dst[count].start = &src[i];
        }

        dst[count].end = &src[i];
        dst[count].type = LEX_STR;
        arg_size ++;
      }
      else
        if(src[i] == quote) {
          quote = 0;
/*
          if(arg_size) {
            dst[count].end = &src[i + 1];
            count ++;
            arg_size = 0;
          }
*/
        }
      continue;
    }

    if(quote)
      continue;

    if(isblank(src[i]) && lparen <= 0) {
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

    if(src[i] == '=' && lparen <= 0) {
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

    if(src[i] == '>' && lparen <= 0) {
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

    if(src[i] == '<' && lparen <= 0) {
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

    if(src[i] == '&' && lparen <= 0) {
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

    if(src[i] == '|' && lparen <= 0) {
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

    if(src[i] == '!' && lparen <= 0) {
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
      lparen ++;
    } //(

    if(src[i] == ')') {
      if(lparen > 0) {
        lparen --;
        if(!lparen) {
          if(arg_size) {
            dst[count].end = &src[i + 1];
            count ++;
            arg_size = 0;
          }
          continue;
        }
      }
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
/**Divide source string to lexems*/
int test_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size)
{
  int count = 0, arg_size = 0, quote = 0;
  unsigned i;
  for(i = 0; i < size && src[i] && count < (int) lex_size; i++) {
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

    if(isblank(src[i])) {
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
    case LEX_STR:      return "STR";
    case LEX_COMMENT:   return "COMMENT";
  }
  return "???";
}
/*----------------------------------------------------------------------------*/
void lex_printf(C_SHELL *sh, const LEX_ELEM *args, int size)
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
static int string_prepare(C_SHELL *sh, const LEX_ELEM *src, char *buffer, unsigned buffer_size)
{
  const char *p, *end;
  unsigned i;

  (void) sh;
  p = src->start;
  end = src->end;
  if(src->type == LEX_STR) {
    if(*src->start == '\'') {
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
int sh_make_argv(C_SHELL *sh, C_SHELL_PARSER *parser)
{
  typedef struct {
    C_SHELL_PARSER parser;

  } sh_make_argv_t;

  sh_make_argv_t *data = NULL;

  static const char empty = 0;
  int i, j, ret = SHELL_OK;
  LEX_ELEM lex;
  char *buffer1, *buffer2 = NULL;
  const char *end;
  unsigned sz;

  for(i = 0; i < parser->argc; i++) {
    parser->argv[i] = NULL;
  }

  buffer1 = (char *) cache_alloc(sh->cache, BUFFER_SIZE);
  if(buffer1 == NULL) {
    return SHELL_ERR_MALLOC;
  }

  do {
    buffer2 = (char *) cache_alloc(sh->cache, BUFFER_SIZE);
    if(buffer2 == NULL) {
      ret = SHELL_ERR_MALLOC;
      break;
    }

    for(i = 0; i < parser->argc; i++) {
      lex = parser->lex[i];
      if(lex.type == LEX_STR && *lex.start != '\'') {
        ret = sh_make_substs(sh, lex.start, lex.end - lex.start, buffer1, BUFFER_SIZE);
        if(ret < 0) {
          break;
        }
        sz = ret;
        ret = SHELL_OK;
        if(*parser->lex[i].start != '"'
           && (lex.end - lex.start != sz || memcmp(lex.start, buffer1, sz)) ) {
          //Split argv[i]
          data = (sh_make_argv_t *) cache_alloc(sh->cache, sizeof(sh_make_argv_t));
          if(data == NULL) {
            ret = SHELL_ERR_MALLOC;
            break;
          }
          memset(data, 0, sizeof(sh_make_argv_t));

          do {
            data->parser.arg0 = 0;
            data->parser.argc = 0;
            ret = cmdline_lexer(buffer1, sz, &end, data->parser.lex, MAX_LEXER_SIZE);
            if(ret < 0) {
              break;
            }
            j = 0; //For i += j;
            if(ret < 2) {
              ret = SHELL_OK;
              break;
            }
            data->parser.argc = ret;
            ret = sh_make_argv(sh, &data->parser);
            if(SHELL_OK != ret) {
              break;
            }

            for(j = 0; j < data->parser.argc; j++) {
              parser->argv[i + j] = data->parser.argv[j];
              data->parser.argv[j] = NULL;
              lex = parser->lex[i + j + 1];

              //INvalid lex ?
              parser->lex[i + j] = data->parser.lex[j];
              parser->lex[i + j].start = NULL;
              parser->lex[i + j].end = NULL;

              parser->lex[i + j + data->parser.argc + 1] = lex;
            }
            parser->argc += data->parser.argc - 1;
          } while(0);

          cache_free(sh->cache, data);
          if(SHELL_OK != ret) {
            break;
          }

          i += j;
          if(j > 1) {
            continue;
          }
          //Split argv[i]
        }

        lex.start = buffer1;
        lex.end = lex.start + sz;
        sz = string_prepare(sh, &lex, buffer2, BUFFER_SIZE);
      } else {
        sz = string_prepare(sh, &parser->lex[i], buffer2, BUFFER_SIZE);
      }
      if(sz) {
        parser->argv[i] = cache_alloc(sh->cache, sz + 1);
        if(parser->argv[i] == NULL) {
          ret = SHELL_ERR_MALLOC;
          break;
        }
        memcpy(parser->argv[i], buffer2, sz);
        parser->argv[i][sz] = 0;
      }
      else {
        parser->argv[i] =(char *) &empty;
      }
    }

  } while (0);
  cache_free(sh->cache, buffer1);
  if(buffer2 != NULL) {
    cache_free(sh->cache, buffer2);
  }

  if(ret != SHELL_OK) {
    for(i = 0; i < parser->argc; i++) {
      if(parser->argv[i] != NULL) {
        cache_free(sh->cache, parser->argv[i]);
        parser->argv[i] = NULL;
      }
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------*/
void argv_printf(C_SHELL *sh, int argc, char **argv)
{
  int i;
  shell_fprintf(sh, SHELL_STDERR, "PARSER:argc:%d\n", argc);
  for(i = 0; i < argc; i++) {
    shell_fprintf(sh, SHELL_STDERR, "args[%d]:'%s'\n", i, argv[i]);
  }
  shell_fprintf(sh, SHELL_STDERR, "%10s\n", "}");
}
/*----------------------------------------------------------------------------*/
static int var_subst(C_SHELL *sh, const char *start, const char *end, char *buffer, unsigned buffer_size)
{
  const char *value;
  char *name;
  unsigned size = 0;


  if(start >= end) {
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

  while(*value && size < buffer_size) {
    buffer[size] = *value;
    size ++;
    value ++;
  }
  return size;
}
/*----------------------------------------------------------------------------*/
static int cmd_subst(C_SHELL *sh, const char *start, const char *end, char *buffer, unsigned buffer_size)
{
  int ret = SHELL_OK, f = 0, r, i, last_cr;
  unsigned size = 0;
  typedef struct {
    C_SHELL_PARSER parser;

  } command_substitution_t;

  command_substitution_t *data = NULL;


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

    r = cmdline_lexer(start, end-start, &end, data->parser.lex, MAX_LEXER_SIZE);
    if(r < 0) {
      break;
    }

    if(r) {
      if(data->parser.lex[r - 1].type == LEX_COMMENT)
        r --;
    }

    if(r) {
      data->parser.argc = r;

      r = sh_make_argv(sh, &data->parser);
      if(r < 0) {
        break;
      }

      if(sh_is_pars_debug_mode(sh)) {
        argv_printf(sh, data->parser.argc, data->parser.argv);
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

    if(ret < 0) {
      break;
    }

    //Read command output
    while (size < (buffer_size - 1)) {
      r = sh->stream.ext_handler->_read(sh->stream.ext_handler->data, f, &buffer[size], 1);
      if(r < 0) {
        ret = r;
        break;
      }

      if(r == 0 || !buffer[size]) {
        break;
      }

      last_cr = 0;
      if(buffer[size] == '\n' || buffer[size] == '\r') {
        buffer[size] = ' ';
        last_cr = 1;
      }

      size ++;
    }
    if(size && last_cr) {
      size --;
    }
    buffer[size] = 0;

    if(ret < 0) {
      break;
    }
    //Read command output end

    ret = size;

  } while(0);

  if(f > 0) {
    sh->stream.ext_handler->_close(sh->stream.ext_handler->data, f);
  }
  cache_free(sh->cache, data);
  return ret;
}
/*----------------------------------------------------------------------------*/
static int arithmetic_subst(C_SHELL *sh, const char *start, const char *end, char *buffer, unsigned buffer_size)
{
  unsigned size;

  size = end - start;

  if(sh_is_lex_debug_mode(sh)) {
    unsigned i;
    shell_puts(sh, "arithmetic:`");
    for(i = 0; i < size; i++) {
      shell_putc(sh, start[i]);
    }
    shell_puts(sh, "'\n");
  }

  return arithmetic(sh, start, size, buffer, buffer_size);
}
/*----------------------------------------------------------------------------*/
static const char *subst_find_char(const char *src, unsigned size, int what)
{
  int quote = 0;
  unsigned i;
  for(i = 0; i < size; i++) {

    if(src[i] == '\\') {
      i ++;
      continue;
    }

    if(src[i] == '\'') {
      if(!quote) {
        quote = src[i];
      }
      else {
        quote = 0;
      }
      continue;
    }

    if(quote)
      continue;

    if(src[i] == what) {
      return &src[i];
    }
  }
  return NULL;
}
/*----------------------------------------------------------------------------*/
static const char *subst_find_str(const char *src, unsigned size, const char *what)
{
  int quote = 0;
  unsigned i, sz;
  sz = strlen(what);
  for(i = 0; i < size; i++) {

    if(src[i] == '\\') {
      i ++;
      continue;
    }

    if(src[i] == '\'') {
      if(!quote) {
        quote = src[i];
      }
      else {
        quote = 0;
      }
      continue;
    }

    if(quote)
      continue;

    if(size - i >= sz
       && !memcmp(&src[i], what, sz)) {
      return &src[i];
    }
  }
  return NULL;
}
/*----------------------------------------------------------------------------*/
static const char *subst_find_rparen(const char *src, unsigned size, int init_count)
{
  int quote = 0, lparen;
  unsigned i;

  lparen = init_count;
  for(i = 0; i < size; i++) {

    if(src[i] == '\\') {
      i ++;
      continue;
    }

    if(src[i] == '\'') {
      if(!quote) {
        quote = src[i];
      }
      else {
        quote = 0;
      }
      continue;
    }

    if(quote)
      continue;

    if(src[i] == '(') {
      lparen ++;
    }

    if(src[i] == ')') {
      lparen --;
      if(!lparen) {
        return &src[i];
      }
    }

  }
  return NULL;
}
/*----------------------------------------------------------------------------*/
int sh_make_substs(C_SHELL *sh, const char *src, unsigned size, char *dst, unsigned dst_size)
{
  static const char delimiters[] = ";|{}/\\+*-=><!$&()\"\'`";
  char *in_buffer = NULL, *out_buffer = NULL, *buf;
  const char *p, *start, *end, *next;
  unsigned buffer_size = 16 * 1024, index;

  int ret = 0;

  enum {
    NO_SUBS,
    DUMMY_SUBS,
    PAREN_SUBS,  //${name}
    BRACES_SUBS, //$(cmd)
    BACKTICK_SUBS, //`cmd`
    SIMPLE_SUBS,  //$xxx
    ARITHMETIC_SUBS //$((expression))
  } type;

  p = src;

  do {

    while (1) {
      type = NO_SUBS;
      p = subst_find_char(p, size - (p - src), '$');
      start = end = p;

      if(p == NULL) {
        p = subst_find_char(src, size, '`');
        start = end = p;
      }

      if(p == NULL) {
        if(size >= dst_size) {
          size = dst_size - 1;
        }
        memcpy(dst, src, size);
        dst[size] = 0;
        ret = size;
        break;
      }

      if(*p == '$') { //Dollar substitutions
        end = ++p;
        if(*p == '{') {
          type = PAREN_SUBS;
          if((end = subst_find_char(p, size - (p - src), '}')) == NULL) {
            type = DUMMY_SUBS;
            end = ++ p;
          } else {
            end ++;
          }


        } else if(*p == '(') {
          if(size > 1 && p[1] == '(') {
            p ++;
            type = ARITHMETIC_SUBS;
            if((end = subst_find_rparen(p + 1, size - (p - src) - 1, 2)) == NULL) {
              type = DUMMY_SUBS;
              end = ++ p;
            } else {
              end ++;
              next = subst_find_char(p, size - (p - src), '$');
              if(next != NULL) {
                p = next;
                continue;
              }
            }

          } else {
            type = BRACES_SUBS;
            if((end = subst_find_rparen(p + 1, size - (p - src) - 1, 1)) == NULL) {
              type = DUMMY_SUBS;
              end = ++ p;
            } else {
              end ++;
              next = subst_find_char(p, size - (p - src), '$');
              if(next != NULL) {
                p = next;
                continue;
              }
            }
          }

        } else {
          type = SIMPLE_SUBS;
          while (end < &src[size]) {
            if(!*end || isblank(*end) || strchr(delimiters, *end) != NULL) {
              break;
            }
            end ++;
          }
        }
        //Dollar substitutions end
      } else {
        //Backstick substitution
        end = ++p;
        type = BACKTICK_SUBS;
        if((end = subst_find_char(p, size - (p - src), '`')) == NULL) {
          type = DUMMY_SUBS;
          end = p;
        } else {
          end ++;
          next = subst_find_char(p, size - (p - src), '$');
          if(next != NULL) {
            p = next;
            continue;
          }
        }
        //Backstick substitution end
      }

      if(out_buffer == NULL) {
        out_buffer = cache_alloc(sh->cache, buffer_size);
      } else {
        out_buffer = cache_realloc(sh->cache, out_buffer, buffer_size);
      }

      if(out_buffer == NULL) {
        ret = SHELL_ERR_MALLOC;
        break;
      }
      index = start - src;
      memcpy(out_buffer, src, index);
      out_buffer[index] = 0;
      ret = 0;
      switch(type) {
        case PAREN_SUBS:  //${}
          ret = var_subst(sh, start + 2, end - 1, &out_buffer[index], buffer_size - index);
          break;
        case BRACES_SUBS: //$()
          ret = cmd_subst(sh, start + 2, end - 1, &out_buffer[index], buffer_size - index);
          break;
        case ARITHMETIC_SUBS: //$(())
          ret = arithmetic_subst(sh, start + 3, end - 2, &out_buffer[index], buffer_size - index);
          break;
        case BACKTICK_SUBS: //`xxx`
          ret = cmd_subst(sh, start + 1, end - 1, &out_buffer[index], buffer_size - index);
          break;
        case SIMPLE_SUBS:  //$xxx
          ret = var_subst(sh, start + 1, end, &out_buffer[index], buffer_size - index);
          break;
        default:
          break;
      }

      if(ret < 0) {
        break;
      }
      index += ret;
      p = end;
      size -= p - src;
      if(size >= buffer_size - index) {
        size = buffer_size - index - 1;
      }
      memcpy(&out_buffer[index], p, size);
      size += index;
      out_buffer[size] = 0;



      out_buffer = cache_realloc(sh->cache, out_buffer, size + 1);

      //in_buffer <-> out_buffer
      buf = in_buffer;
      in_buffer = out_buffer;
      out_buffer = buf;
      p = src = in_buffer;
    }


  } while(0);

  if(in_buffer != NULL) {
    cache_free(sh->cache, in_buffer);
  }
  if(out_buffer != NULL) {
    cache_free(sh->cache, out_buffer);
  }
  return ret;
}
/*----------------------------------------------------------------------------*/
/** String argument representation: 123=>123 11 22 => '11 22' */
int to_str(const char *src, char *dst, unsigned buffer_size)
{
  int i, size, have_blank = 0;
  const char *sdst;

  sdst = dst;
  while(*src && isblank(*src)) {
    src ++;
  }

  size = strlen(src);
  if(buffer_size) {
    *dst = 0;
  } else {
    return 0;
  }


  for(i = 0; i < size; i++) {
    if(isblank(src[i])) {
      have_blank ++;
      break;
    }
  }

  if(buffer_size && (have_blank || !size)) {
    *dst = '\'';
    dst ++; buffer_size --;
  }

  if(size < (int) buffer_size) {
    memcpy(dst, src, size);
    dst += size; buffer_size -= size;
  }

  if(buffer_size && (have_blank || !size)) {
    *dst = '\'';
    dst ++; buffer_size --;
  }

  if(buffer_size) {
    *dst = 0;
  }

  return dst - sdst;
}
/*----------------------------------------------------------------------------*/
static int _htoi(const char *src, unsigned size)
{
  unsigned int c = 0;
  const char *p;
  unsigned i;
  p = src;
  for(i = 0; i < size; i++)
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
  return (int) c;
}
/*----------------------------------------------------------------------------*/
static int _otoi(const char *src, unsigned size)
{
  unsigned int c = 0;
  unsigned i;
  const char *p;
  p = src;
  for(i = 0; i < size; i++)
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

  return (int) c;
}
/*----------------------------------------------------------------------------*/
static int _atoi(const char *src, unsigned size)
{
  unsigned int c = 0;
  unsigned i;
  const char *p;
  p = src;

  if(*src == '0') {
    if(size > 2 && (src[1] == 'x' || src[1] == 'X')) {
      return _htoi(&src[2], size - 2);
    }
    return _otoi(src, size);
  }

  for(i = 0; i < size; i++)
  {
    if(*p >= '0' && *p <= '9')
    {
      c *= 10;
      c += (unsigned int)(*p - '0');
    }
    else
      break;
    p ++;
  }

  return (int) c;
}
/*----------------------------------------------------------------------------*/
/** Arithmetic lexer*/
enum ARITHMETIC_LEX {
  AL_BLANK
  , AL_NUMBER
  , AL_VAR
  , AL_LPAREN //(
  , AL_RPAREN //)
  , AL_PLUS //+
  , AL_MINUS //-
  , AL_INCR //++
  , AL_DECR //--
  , AL_MUL //*
  , AL_DIV // /
  , AL_MOD //%
  , AL_AND //&&
  , AL_OR //||
  , AL_BAND //&
  , AL_BOR //|
  , AL_EQ  // ==
  , AL_NE  // !=
  , AL_GT  //>
  , AL_GE  //>=
  , AL_LT  //<
  , AL_LE  //<=
  , AL_ASSIGN //=
  , AL_NOT  // !
  , AL_BNOT  // ~
  , AL_BXOR  // ^
  , AL_BLSHIFT //<<
  , AL_BRSHIFT //>>

  , AL_UNDEFINED
};

static int arithmetic_lex(const char *src, unsigned size, const char **end)
{
  unsigned i;
  *end = src + 1;
  switch(*src) {
    case '(':
      return AL_LPAREN;
    case ')':
      return AL_RPAREN;
    case '+':
      if(size && src[1] == '+') {
        ++ *end;
        return AL_INCR;
      }
      return AL_PLUS;

    case '-':
      if(size && src[1] == '-') {
        ++ *end;
        return AL_DECR;
      }
      return AL_MINUS;

    case '*':
      return AL_MUL;
    case '/':
      return AL_DIV;
    case '%':
      return AL_MOD;

    case '|':
      if(size && src[1] == '|') {
        ++ *end;
        return AL_OR;
      }
      return AL_BOR;

    case '&':
      if(size && src[1] == '&') {
        ++ *end;
        return AL_AND;
      }
      return AL_BAND;

    case '=':
      if(size && src[1] == '=') {
        ++ *end;
        return AL_EQ;
      }
      return AL_ASSIGN;

    case '!':
      if(size && src[1] == '=') {
        ++ *end;
        return AL_NE;
      }
      return AL_NOT;

    case '>':
      if(size && src[1] == '=') {
        ++ *end;
        return AL_GE;
      }
      if(size && src[1] == '>') {
        ++ *end;
        return AL_BRSHIFT;
      }
      return AL_GT;

    case '<':
      if(size && src[1] == '=') {
        ++ *end;
        return AL_LE;
      }
      if(size && src[1] == '<') {
        ++ *end;
        return AL_BLSHIFT;
      }
      return AL_LT;

    case '~':
      return AL_BNOT;

    case '^':
      return AL_BXOR;

    default:
      if(isblank(*src)) {
        return AL_BLANK;
      }

      if(*src >= '0' && *src <= '9') {
        //Hex digit
        if(*src == '0'
           && size > 1
           && (src[1] == 'x' || src[1] == 'X')) {
          src = &src[2];
          size -= 2;
          *end = src;
          for(i = 0; i < size; i++) {
            if( (src[i] >= 'A' && src[i] <= 'F')
               || (src[i] >= 'a' && src[i] <= 'f')
               || (src[i] >= '0'&& src[i] <= '9')) {
              *end = &src[i + 1];
            } else {
              break;
            }
          }
          return AL_NUMBER;
        }

        //Octal digit
        if(*src == '0') {
          for(i = 0; i < size; i++) {
            if(src[i] < '0' || src[i] > '7') {
              break;
            }
            *end = &src[i + 1];
          }
          return AL_NUMBER;
        }

        //Decimal
        for(i = 1; i < size; i++) {
          if(src[i] < '0' || src[i] > '9') {
            break;
          }
          *end = &src[i + 1];
        }
        return AL_NUMBER;
      }

      if( (*src >= 'A' && *src <= 'Z')
          || (*src >= 'a' && *src <= 'z')
          || *src == '_') {
        for(i = 1; i < size; i++) {
          if((src[i] >= 'A' && src[i] <= 'Z')
                 || (src[i] >= 'a' && src[i] <= 'z')
                 || (src[i] >= '0' && src[i] <= '9')
                 || src[i] == '_') {
            *end = &src[i + 1];
          } else {
            break;
          }
        }
        return AL_VAR;
      }
      break;
  }
  return AL_UNDEFINED;
}
/*----------------------------------------------------------------------------*/
typedef struct APARS {
  int operands;
  int operators;
  int operand[2];
  int operator[2];
  int unary_prefix;
  int unary_postfix;
  struct APARS *prev;
} APARS;
/*----------------------------------------------------------------------------*/
static int op_unary(C_SHELL *sh, int arg, int op, int *dst) {
  switch (op) {
    case AL_PLUS:
      *dst = arg;
      break;
    case AL_MINUS:
      *dst = -1 * arg;
      break;
    case AL_INCR:
      *dst = arg + 1;
      break;
    case AL_DECR:
      *dst = arg - 1;
      break;
    case AL_NOT:
      *dst = !arg;
      break;
    case AL_BNOT:
      *dst = ~arg;
      break;
    default:
      shell_fprintf(sh, SHELL_STDERR, "Invalid unary operator\n");
      return SHELL_ERR_INVALID_OP;
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int op_binary(C_SHELL *sh, int arg1, int arg2, int op, int *dst) {
  switch(op) {
    case AL_PLUS:
      *dst = arg1 + arg2;
      break;
    case AL_MINUS:
      *dst = arg1 - arg2;
      break;
    case AL_MUL:
      *dst = arg1 * arg2;
      break;
    case AL_DIV:
      if(!arg2) {
        shell_fprintf(sh, SHELL_STDERR, "Integer divide by zerro\n");
        return SHELL_ERR_INVALID_OP;
      }
      *dst = arg1 / arg2;
      break;
    case AL_MOD:
      if(!arg2) {
        shell_fprintf(sh, SHELL_STDERR, "Integer divide by zerro\n");
        return SHELL_ERR_INVALID_OP;
      }
      *dst = arg1 % arg2;
      break;
    case AL_AND:
      *dst = arg1 && arg2;
      break;
    case AL_OR:
      *dst = arg1 || arg2;
      break;
    case AL_BAND:
      *dst = arg1 & arg2;
      break;
    case AL_BOR:
      *dst = arg1 | arg2;
      break;
    case AL_EQ:
      *dst = arg1 == arg2;
      break;
    case AL_NE:
      *dst = arg1 != arg2;
      break;
    case AL_GT:
      *dst = arg1 > arg2;
      break;
    case AL_GE:
      *dst = arg1 >= arg2;
      break;
    case AL_LT:
      *dst = arg1 < arg2;
      break;
    case AL_LE:
      *dst = arg1 <= arg2;
      break;
    case AL_BXOR:
      *dst = arg1 ^ arg2;
      break;
    case AL_BLSHIFT:
      *dst = arg1 << arg2;
      break;
    case AL_BRSHIFT:
      *dst = arg1 >> arg2;
      break;
    default:
      shell_fprintf(sh, SHELL_STDERR, "Invalid binary operator\n");
      return SHELL_ERR_INVALID_OP;
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int op_precedence(int op) {
  switch(op) {
    case AL_PLUS:
    case AL_MINUS:
      return 4;
    case AL_INCR:
    case AL_DECR:
      return 1;
    case AL_MUL:
    case AL_DIV:
    case AL_MOD:
      return 3;
    case AL_AND:
      return 11;
    case AL_OR:
      return 12;
    case AL_BAND:
      return 8;
    case AL_BOR:
      return 10;
    case AL_EQ:
    case AL_NE:
      return 7;
    case AL_GT:
    case AL_GE:
    case AL_LT:
    case AL_LE:
      return 6;
    case AL_NOT:
    case AL_BNOT:
      return 2;
    case AL_BXOR:
      return 9;
    case AL_BLSHIFT:
    case AL_BRSHIFT:
      return 5;
  }
  return -1;
}
/*----------------------------------------------------------------------------*/
static int op_operand(C_SHELL *sh, APARS *state, int operand)
{
  int ret = 0, precedence;

  if(state->unary_prefix) {
    ret = op_unary(sh, operand, state->unary_prefix, &operand);
    if(ret < 0) {
      return ret;
    }
  }
  state->unary_prefix = 0;

  if(state->operands < 2) {
    state->operand[state->operands] = operand;
    state->operands ++;
  } else {
    //Compare op priority - precedence
    precedence = op_precedence(state->operator[0]);
    if(precedence <= op_precedence(state->operator[1])) {
      ret = op_binary(sh, state->operand[0], state->operand[1], state->operator[0], &state->operand[0]);
      state->operand[1] = operand;
      state->operator[0] = state->operator[1];
      state->operands = 2;
      state->operators = 1;
    } else {
      ret = op_binary(sh, state->operand[1], operand, state->operator[1], &state->operand[1]);
      state->operands = 2;
      state->operators = 1;
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------*/
/**Run arithmetic expression*/
int arithmetic(C_SHELL *sh, const char *src, unsigned size, char *buffer, unsigned buffer_size)
{
  int ret = 0, lex, operand, lex_count = 0;
  unsigned sz;
  APARS *state, *n;
  const char *p, *end, *v;
  char name[VAR_NAME_LENGTH] = {0};
  char assign_var[VAR_NAME_LENGTH] = {0};


  p = src;
  state = (APARS *) cache_alloc(sh->cache, sizeof(APARS));
  if(state == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(state, 0, sizeof(APARS));
  state->prev = NULL;
  do {

    while (p - src < size) {
      lex = arithmetic_lex(p, size - (p - src), &end);

      switch(lex) {
        case AL_BLANK:
          break;

        case AL_ASSIGN:
          if(state->operands == 1
             && !state->operators
             && lex_count == 1
             && name[0]) {
            //Previos lex was variable
            strncpy(assign_var, name, sizeof(assign_var));
            state->operands = 0;
          } else {
            ret = SHELL_ERR_INVALID_OP;
          }
          break;

        case AL_VAR:
        case AL_NUMBER:
          if(state->operands && state->operators < state->operands) {
            ret = SHELL_ERR_INVALID_OP;
            break;
          }

          operand = 0;
          if(lex == AL_VAR) {
            sz = end - p;
            if(sz >= sizeof(name) - 1) {
              sz = sizeof(name) - 1;
            }
            memcpy(name, p, sz);
            name[sz] = '\0';
            v = sh_get_var(sh, name);
            if(v != NULL && *v) {
              sz = strlen(v);
              operand = _atoi(v, sz);
            }
          }

          if(lex == AL_NUMBER) {
            operand = _atoi(p, end - p);
          }

          ret = op_operand(sh, state, operand);
          break;

        //Binary
        case AL_PLUS:
        case AL_MINUS:
          if(!state->operands || state->operators >= state->operands) {
            state->unary_prefix = lex;
          } else {
            state->operator[state->operators] = lex;
            state->operators ++;
          }
          break;
        case AL_MUL:
        case AL_DIV:
        case AL_MOD:
        case AL_AND:
        case AL_OR:
        case AL_BAND:
        case AL_BOR:
        case AL_EQ:
        case AL_NE:
        case AL_GT:
        case AL_GE:
        case AL_LT:
        case AL_LE:
        case AL_BXOR:
        case AL_BLSHIFT:
        case AL_BRSHIFT:
          if(!state->operands || state->operators >= state->operands) {
            ret = SHELL_ERR_INVALID_OP;
          } else {
            state->operator[state->operators] = lex;
            state->operators ++;
          }
          break;

          //Unary
        case AL_NOT:
        case AL_BNOT:
          if(!state->operands || state->operators >= state->operands) {
            //Prefix
            state->unary_prefix = lex;
          } else {
            //Postfix
            ret = SHELL_ERR_INVALID_OP;
          }
          break;

        case AL_INCR:
        case AL_DECR:
          if(!state->operands || state->operators >= state->operands) {
            //Prefix
            state->unary_prefix = lex;
          } else {
            //Postfix
            ret = op_unary(sh, state->operand[state->operands - 1], lex, &state->operand[state->operands - 1]);
          }
          break;

        case AL_LPAREN:
          n = (APARS *) cache_alloc(sh->cache, sizeof(APARS));
          if(n == NULL) {
            ret = SHELL_ERR_MALLOC;
            break;
          }
          memset(n, 0, sizeof(APARS));
          n->prev = state;
          state = n;
          break;

        case AL_RPAREN:
          if(state->prev == NULL) {
            ret = SHELL_ERR_INVALID_OP;
            break;
          }
          if(state->operators && state->operands <= state->operators) {
            ret = SHELL_ERR_INVALID_OP;
            break;
          }
          if(state->operands > 1) {
            ret = op_binary(sh, state->operand[0], state->operand[1], state->operator[0], &state->operand[0]);
            state->operands = 1;
          }

          if(ret < 0) {
            break;
          }

          if(state->operands) {
            operand = state->operand[0];
            n = state->prev;
            cache_free(sh->cache, state);
            state = n;
            ret = op_operand(sh, state, operand);
          } else {
            n = state->prev;
            cache_free(sh->cache, state);
            state = n;
          }
          break;


        default:
          ret = SHELL_ERR_INVALID_OP;
      }


      if(ret < 0) {
        break;
      }

      p = end;
      if(lex != AL_BLANK) {
        lex_count ++;
      }
    }

    if(ret < 0) {
      break;
    }

    if(state->prev != NULL) {
      shell_fprintf(sh, SHELL_STDERR, "Not closed (\n");
      ret = SHELL_ERR_INVALID_OP;
      break;
    }

    if(state->operators && state->operands <= state->operators) {
      ret = SHELL_ERR_INVALID_OP;
      break;
    }

    if(state->operands) {
      if(state->operands > 1) {
        ret = op_binary(sh, state->operand[0], state->operand[1], state->operator[0], &state->operand[0]);
        state->operands = 1;
      }
      if(ret < 0) {
        break;
      }
      ret = snprintf(buffer, buffer_size, "%d", state->operand[0]);
      buffer[buffer_size - 1] = 0;
    } else {
      if(buffer_size) {
        buffer[0] = 0;
      }
    }

    if(*assign_var) {
      sh_set_var(sh, assign_var, buffer);
    }

  } while(0);

  //FREE states
  while(state != NULL) {
    n = state->prev;
    cache_free(sh->cache, state);
    state = n;
  }

  if(ret < 0) {
    shell_fprintf(sh, SHELL_STDERR, "\n`");
    for(sz = 0; sz < size; sz++) {
      shell_fputc(sh, SHELL_STDERR, src[sz]);
    }
    shell_fprintf(sh, SHELL_STDERR, "'\n ");
    for(sz = 0; sz < size; sz++) {
      if(&src[sz] == p) {
        shell_fputc(sh, SHELL_STDERR, '^');
        break;
      }
      shell_fputc(sh, SHELL_STDERR, ' ');
    }
    shell_fputs(sh, SHELL_STDERR, "\r\n");
  }

  return ret;
}
/*----------------------------------------------------------------------------*/

