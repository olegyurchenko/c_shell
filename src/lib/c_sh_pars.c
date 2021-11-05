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
int sh_lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size)
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
int sh_make_argv(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv)
{
  static const char empty = 0;
  int i, ret = SHELL_OK;
  LEX_ELEM lex;
  char *buffer1, *buffer2 = NULL;
  unsigned sz;

  for(i = 0; i < size; i++) {
    argv[i] = NULL;
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

    for(i = 0; i < size; i++) {
      lex = args[i];
      if(lex.type == LEX_STR && *lex.start != '\'') {
        ret = sh_make_substs(sh, lex.start, lex.end - lex.start, buffer1, BUFFER_SIZE);
        if(ret < 0) {
          break;
        }
        lex.start = buffer1;
        lex.end = lex.start + ret;
        ret = SHELL_OK;
        sz = string_prepare(sh, &lex, buffer2, BUFFER_SIZE);
      } else {
        sz = string_prepare(sh, &args[i], buffer2, BUFFER_SIZE);
      }
      if(sz) {
        argv[i] = cache_alloc(sh->cache, sz + 1);
        if(argv[i] == NULL) {
          ret = SHELL_ERR_MALLOC;
          break;
        }
        memcpy(argv[i], buffer2, sz);
        argv[i][sz] = 0;
      }
      else {
        argv[i] =(char *) &empty;
      }
    }

  } while (0);
  cache_free(sh->cache, buffer1);
  if(buffer2 != NULL) {
    cache_free(sh->cache, buffer2);
  }

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

    r = sh_lexer(start, end-start, &end, data->parser.lex, MAX_LEXER_SIZE);
    if(r < 0) {
      break;
    }

    if(r) {
      if(data->parser.lex[r - 1].type == LEX_COMMENT)
        r --;
    }

    if(r) {
      data->parser.argc = r;

      r = sh_make_argv(sh, data->parser.lex, data->parser.argc, data->parser.argv);
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
static const char *subst_find(const char *src, unsigned size, int what)
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
    SIMPLE_SUBS  //$xxx
  } type;

  p = src;

  do {

    while (1) {
      type = NO_SUBS;
      p = subst_find(p, size - (p - src), '$');
      start = end = p;

      if(p == NULL) {
        p = subst_find(src, size, '`');
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
          if((end = subst_find(p, size - (p - src), '}')) == NULL) {
            type = DUMMY_SUBS;
            end = ++ p;
          } else {
            end ++;
          }


        } else if(*p == '(') {
          type = BRACES_SUBS;
          if((end = subst_find(p, size - (p - src), ')')) == NULL) {
            type = DUMMY_SUBS;
            end = ++ p;
          } else {
            end ++;
            next = subst_find(p, size - (p - src), '$');
            if(next != NULL) {
              p = next;
              continue;
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
        if((end = subst_find(p, size - (p - src), '`')) == NULL) {
          type = DUMMY_SUBS;
          end = p;
        } else {
          end ++;
          next = subst_find(p, size - (p - src), '$');
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

