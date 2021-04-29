/*----------------------------------------------------------------------------*/
/**
* @pkg c_sh
*/
/**
* Simple shell.
*
* (C) T&T team, Kiev, Ukraine 2020.<br>
* started 08.12.2020 11:29:31<br>
* @pkgdoc c_sh
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#include "c_sh.h"
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
#ifdef STDLIB
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#else //STDLIB
#endif //STDLIB
/*----------------------------------------------------------------------------*/
#include "c_cache.h"
#include "loop_buff.h"
#include "nprintf.h"
/*----------------------------------------------------------------------------*/
#ifndef MALLOC
#define MALLOC malloc
#endif
#ifndef FREE
#define FREE free
#endif
/*----------------------------------------------------------------------------*/
//#define USE_SH_TEST //Use external test function
#ifdef USE_SH_TEST
  #include "c_sh_test.h"
#endif //USE_SH_TEST
/*----------------------------------------------------------------------------*/
#define DEBUG_MOODE
/*----------------------------------------------------------------------------*/
#define VAR_NAME_LENGTH 16
#define MAX_LEXER_SIZE 256
#define DEFAULT_CACHE_SIZE 64*1024
#define BUFFER_SIZE 1024
/*----------------------------------------------------------------------------*/
typedef enum {
  LEX_ASSIGN //'='
  , LEX_NOT //!
  , LEX_AMP //&
  , LEX_VERBAR //|
  , LEX_AND //&&
  , LEX_OR //||
  , LEX_EQ  // ==
  , LEX_NE  // !=
  , LEX_GT  //>
  , LEX_GE  //>=
  , LEX_LT  //<
  , LEX_LE  //<=
  , LEX_LSQB //[
  , LEX_LDSQB //[[
  , LEX_RSQB //]
  , LEX_RDSQB //]
  , LEX_LPAREN //(
  , LEX_RPAREN //)
  , LEX_STR //string
  , LEX_COMMENT //#comm
} LEX_CODES;
/*----------------------------------------------------------------------------*/
typedef enum {
  OP_UNKNOWN = 0,
  OP_IF,
  OP_FOR,
  OP_WHILE,
  OP_UNTIL
} OP_CODE;
/*----------------------------------------------------------------------------*/
struct C_SHELL_VAR_TAG {
  struct C_SHELL_VAR_TAG *next;
  unsigned hash;
  char name[VAR_NAME_LENGTH];
  char value[1];
};
typedef struct C_SHELL_VAR_TAG C_SHEL_VAR;
/*----------------------------------------------------------------------------*/
typedef struct {
  const char *start;
  const char *end;
  int type;
  int reserved; //Alignmemnt
} LEX_ELEM;
/*----------------------------------------------------------------------------*/
struct C_SHELL_PARSER_TAG {
  LEX_ELEM lex[MAX_LEXER_SIZE];
  char *argv[MAX_LEXER_SIZE];
  int argc;
  int arg0; //Start argument (may be > 0)
  //int reserved; //Alignmemnt
};
typedef struct C_SHELL_PARSER_TAG C_SHELL_PARSER;
/*----------------------------------------------------------------------------*/
struct C_SHELL_LINE_CACHE_TAG {
  int op_id;
  struct C_SHELL_LINE_CACHE_TAG *next;
  char text[1];
};
typedef struct C_SHELL_LINE_CACHE_TAG C_SHELL_LINE_CACHE;
/*----------------------------------------------------------------------------*/
struct C_SHELL_CONTEXT_TAG {
  struct C_SHELL_CONTEXT_TAG *child;
  struct C_SHELL_CONTEXT_TAG *parent;
  struct {
    C_SHELL_LINE_CACHE *first;
    C_SHELL_LINE_CACHE *last;
    C_SHELL_LINE_CACHE *root;
  } cache;
  int op_id;
  OP_CODE op_code;
  int for_index;
  int condition:1;
  int should_store_cache:1;
  int is_loop:1;
  int is_continue:1;
};
typedef struct C_SHELL_CONTEXT_TAG C_SHELL_CONTEXT;
/*----------------------------------------------------------------------------*/
struct C_SHELL_TAG {
  struct {
    SHELL_PRINT_CB cb;
    void *arg;
  } print_cb;

  struct  {
    SHELL_EXEC_CB cb;
    void *arg;
  } exec_cb;

  struct  {
    SHELL_STEP_CB cb;
    void *arg;
  } step_cb;

  struct {
    SHELL_STREAM_HANDLER *ext_handler;
    int f[3]; /*File h f[0] - stdin, f[1] - stdout, f[2] - stderr*/
  } stream;

  C_CACHE *cache;
  C_SHELL_CONTEXT context;
  C_SHELL_PARSER *parser;
  C_SHEL_VAR *vars;

  int op_id;
  int cache_stack;
  int cache_mode:1;
  int terminate:1;
};
/*----------------------------------------------------------------------------*/
static unsigned calc_hash(const char *str);
/*----------------------------------------------------------------------------*/
static int lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size);
static void lex_print(C_SHELL *sh, const LEX_ELEM *args, int size);
static unsigned string_prepare(C_SHELL *sh, const LEX_ELEM *src, char *buffer, unsigned buffer_size);
static int args_prepare(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv);
static void args_print(C_SHELL *sh, int argc, char **argv);
/*----------------------------------------------------------------------------*/
static int context_stack(C_SHELL *sh);
static int push_context(C_SHELL *sh);
static int pop_context(C_SHELL *sh);
static C_SHELL_CONTEXT *root_context(C_SHELL *sh);
static C_SHELL_CONTEXT *current_context(C_SHELL *sh);
static int set_var(C_SHELL *sh, const char *name, const char *value);
static const char *get_var(C_SHELL *sh, const char *name);
static void clear_vars(C_SHELL *sh);
static int add_src_to_cache(C_SHELL *sh, const char *src, unsigned size);
static void set_context_should_store_cache(C_SHELL *sh, int should_store);
static int play_cache(C_SHELL *sh);
static int remove_cache(C_SHELL *sh);
static void set_cache_mode(C_SHELL *sh, int mode);
static int is_cache_mode(C_SHELL *sh);
static void set_opcode(C_SHELL *sh, OP_CODE c);
static OP_CODE current_opcode(C_SHELL *sh);
/*----------------------------------------------------------------------------*/
#define TRUE_CONDITION 0
#define NOT_TRUE_CONDITION 1

static void set_condition(C_SHELL *sh, int c);
static void set_true_condition(C_SHELL *sh) {set_condition(sh, TRUE_CONDITION);}
static void set_false_condition(C_SHELL *sh) {set_condition(sh, NOT_TRUE_CONDITION);}
static int get_condition(C_SHELL *sh);
static int is_true_condition(C_SHELL *sh);
/*----------------------------------------------------------------------------*/
static int is_debug_mode(C_SHELL *sh);
static int is_lex_debug_mode(C_SHELL *sh);
static int is_pars_debug_mode(C_SHELL *sh);
static int is_cache_debug_mode(C_SHELL *sh);
/*----------------------------------------------------------------------------*/
static int exec0(C_SHELL *sh, int argc, char **argv);
static int exec1(C_SHELL *sh, int argc, char **argv);
static int exec3(C_SHELL *sh, int argc, char **argv);
static int embed_exec(C_SHELL *sh, int argc, char **argv);
/**Embeded functions*/
static int _eXit(C_SHELL *sh, int argc, char **argv);
static int _break(C_SHELL *sh, int argc, char **argv);
static int _continue(C_SHELL *sh, int argc, char **argv);
static int _if(C_SHELL *sh, int argc, char **argv);
static int _then(C_SHELL *sh, int argc, char **argv);
static int _fi(C_SHELL *sh, int argc, char **argv);
static int _else(C_SHELL *sh, int argc, char **argv);
static int _elif(C_SHELL *sh, int argc, char **argv);
static int _true(C_SHELL *sh, int argc, char **argv);
static int _false(C_SHELL *sh, int argc, char **argv);
static int _while(C_SHELL *sh, int argc, char **argv);
static int _until(C_SHELL *sh, int argc, char **argv);
static int _for(C_SHELL *sh, int argc, char **argv);
static int _do(C_SHELL *sh, int argc, char **argv);
static int _done(C_SHELL *sh, int argc, char **argv);
static int _echo(C_SHELL *sh, int argc, char **argv);
static int _test(C_SHELL *sh, int argc, char **argv);
static int _set(C_SHELL *sh, int argc, char **argv);
static int _assign(C_SHELL *sh, int argc, char **argv);
static int _and(C_SHELL *sh, int argc, char **argv);
static int _or(C_SHELL *sh, int argc, char **argv);
/*----------------------------------------------------------------------------*/
#ifndef USE_SH_TEST
static int intern_test(C_SHELL *sh, int argc, char **argv);
#endif //USE_SH_TEST
/*----------------------------------------------------------------------------*/
typedef struct {
  unsigned hash;
  const char *name;
  int (*handler)(C_SHELL *,int,char**);

} STD_FN;

static const STD_FN std_fn[] = {
    {0x7c967e3f, "exit", _eXit}
  , {0x0f2c9f4a, "break", _break}
  , {0x42aefb8a, "continue", _continue}
  , {0x00597834, "if", _if}
  , {0x7c9e7354, "then", _then}
  , {0x005977d4, "fi", _fi}
  , {0x7c964c6e, "else", _else}
  , {0x7c964b25, "elif", _elif}
  , {0x7c9e9fe5, "true", _true}
  , {0x0f6bcef0, "false", _false}
  , {0x10a3387e, "while", _while}
  , {0x10828031, "until", _until}
  , {0x0b88738c, "for", _for}
  , {0x00597798, "do", _do}
  , {0x7c95cc2b, "done", _done}
  , {0x7c9624c4, "echo", _echo}
  , {0x7c9e6865, "test", _test}
  , {0x0002b600, "[", _test}
  , {0x0059765b, "[[", _test}
  , {0x0b88a991, "set", _set}
  , {0x00596f51, "&&", _and}
  , {0x00597abd, "||", _or}
};
/*----------------------------------------------------------------------------
"""
Python script to compute hashes
"""
def hash(str):
  h = 5381
  for c in str:
    h = (h * 33 + ord(c)) & 0xffffffff
  return h

names = ("exit"
, "break"
, "continue"
, "if"
, "then"
, "fi"
, "else"
, "elif"
, "true"
, "false"
, "while"
, "until"
, "for"
, "do"
, "done"
, "echo"
, "test"
, "["
, "[["
, "set")

comma = " "
print()
for name in names:
  print( '{0} {{ 0x{1:08x}, "{2}", _{3} }}'.format(comma, hash(name), name, name))
  comma=','
----------------------------------------------------------------------------*/
const char *shell_err_string(C_SHELL *sh, int err)
{
  const char *str = "Unknown error";

  (void) sh;

  if(err <= SHELL_EXIT) {
    static char buffer[24];
    snprintf(buffer, sizeof(buffer), "exit %d", SHELL_EXIT - err);
    str = buffer;
  }

  switch(err) {
    case SHELL_OK:
      str = "";
      break;
    case SHELL_ERR_MALLOC:
      str = "Memory allocation error";
      break;
    case SHELL_ERR_INVALID_ARG:
      str = "Invalid argument error";
      break;
    case SHELL_ERR_COMMAND_NOT_FOUND:
      str = "Command not found";
      break;
    case SHELL_ERR_NOT_IMPLEMENT:
      str = "Command not implemented";
      break;
    case SHELL_ERR_INVALID_OP:
      str = "Invalid operation";
      break;
    case SHELL_STACK_ERROR:
      str = "Stack error";
      break;

  }
  return str;
}
/*----------------------------------------------------------------------------*/
C_SHELL *shell_alloc()
{
  C_SHELL *sh;
  void *buf;

  unsigned cache_size = DEFAULT_CACHE_SIZE;
  unsigned size = cache_size + sizeof(C_SHELL);

  sh = (C_SHELL *)MALLOC(size);
  if(sh == NULL) {
    return NULL;
  }
  memset(sh, 0, size);
  buf = (char *)sh + sizeof(C_SHELL);
  sh->cache = cache_init(buf, cache_size);
  if(sh->cache == NULL) {
    FREE(sh);
    return NULL;
  }

  return sh;
}
/*----------------------------------------------------------------------------*/
void shell_reset(C_SHELL *sh)
{
  while(context_stack(sh)) {
    pop_context(sh);
  }
  set_cache_mode(sh, 0);
  set_context_should_store_cache(sh, 0);
  remove_cache(sh);
  clear_vars(sh);
}
/*----------------------------------------------------------------------------*/
void shell_free(C_SHELL *sh)
{
  if(sh != NULL) {
    FREE(sh);
  }
}
/*----------------------------------------------------------------------------*/
void shell_set_print_cb(C_SHELL *sh, SHELL_PRINT_CB cb, void *cb_arg)
{
  sh->print_cb.cb = cb;
  sh->print_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
void shell_set_exec_cb(C_SHELL *sh, SHELL_EXEC_CB cb, void *cb_arg)
{
  sh->exec_cb.cb = cb;
  sh->exec_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
void shell_set_step_cb(C_SHELL *sh, SHELL_STEP_CB cb, void *cb_arg)
{
  sh->step_cb.cb = cb;
  sh->step_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
void shell_set_stream_handler(C_SHELL *sh, SHELL_STREAM_HANDLER *h)
{
  sh->stream.ext_handler = h;
}
/*----------------------------------------------------------------------------*/
typedef struct {
  C_SHELL *sh;
  SHELL_STREAM_ID f;
} SHELL_WRITER_ARG;
static int shell_writer(void *arg, const char *txt, int size)
{
  SHELL_WRITER_ARG *data;
  int i, r, ret = 0;
  data = (SHELL_WRITER_ARG *) arg;

  switch (data->f) {
    case SHELL_STDOUT:
    case SHELL_STDERR:
      break;
    default:
      return SHELL_ERR_INVALID_OP;
  }

  if(data != NULL
     && data->sh != NULL) {

    if(data->sh->stream.ext_handler != NULL
       && data->sh->stream.f[data->f]) {
      r = data->sh->stream.ext_handler->_write(data->sh->stream.ext_handler->data,
          data->sh->stream.f[data->f],
          txt,
          size);
    } else {
      if(data->sh->print_cb.cb != NULL) {
        for(i = 0; i < size; i++) {
          if((r = data->sh->print_cb.cb(data->sh->print_cb.arg, txt[i])) <= 0)
            return r;
          ret += r;
        }
      }
    }
  }
  return ret;
}
/*----------------------------------------------------------------------------*/
int shell_fputc(C_SHELL *sh, SHELL_STREAM_ID f, int c)
{
  SHELL_WRITER_ARG arg;
  arg.sh = sh;
  arg.f = f;
  return shell_writer(&arg, (char *) &c, 1);
}
/*----------------------------------------------------------------------------*/
int shell_fputs(C_SHELL *sh, SHELL_STREAM_ID f, const char *text)
{
  SHELL_WRITER_ARG arg;
  arg.sh = sh;
  arg.f = f;
  return shell_writer(&arg, text, (int) strlen(text));
}
/*----------------------------------------------------------------------------*/
int shell_fprintf(C_SHELL *sh, SHELL_STREAM_ID f, const char *format, ...)
{
  int r;
  SHELL_WRITER_ARG arg;
  va_list ap;
  arg.sh = sh;
  arg.f = f;

  va_start(ap, format);
  r = vnprintf(shell_writer, &arg, format, ap);
  va_end(ap);

  return r;
}
/*----------------------------------------------------------------------------*/
int shell_vfprintf(C_SHELL *sh, SHELL_STREAM_ID f, const char *format, va_list ap)
{
  SHELL_WRITER_ARG arg;
  arg.sh = sh;
  arg.f = f;
  return vnprintf(shell_writer, &arg, format, ap);
}
/*----------------------------------------------------------------------------*/
int shell_write(C_SHELL *sh, SHELL_STREAM_ID f,  const void *data, unsigned size)
{
  SHELL_WRITER_ARG arg;
  arg.sh = sh;
  arg.f = f;
  return shell_writer(&arg, (const char *)data, size);
}
/*----------------------------------------------------------------------------*/
int shell_read(C_SHELL *sh, void *data, unsigned size)
{
  int r = 0;
  if(sh->stream.ext_handler != NULL
     && sh->stream.f[SHELL_STDIN]) {
    r = sh->stream.ext_handler->_read(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDIN], data, size);
  }
  return r;
}
/*----------------------------------------------------------------------------*/
int shell_putc(C_SHELL *sh, int c)
{
  return shell_fputc(sh, SHELL_STDOUT, c);
}
/*----------------------------------------------------------------------------*/
int shell_puts(C_SHELL *sh, const char *text)
{
  return shell_fputs(sh, SHELL_STDOUT, text);
}
/*----------------------------------------------------------------------------*/
int shell_printf(C_SHELL *sh, const char *format, ...)
{
  int r;
  SHELL_WRITER_ARG arg;
  va_list ap;
  arg.sh = sh;
  arg.f = SHELL_STDOUT;

  va_start(ap, format);
  r = vnprintf(shell_writer, &arg, format, ap);
  va_end(ap);

  return r;
}
/*----------------------------------------------------------------------------*/
int shell_vprintf(C_SHELL *sh, const char *format, va_list ap)
{
  SHELL_WRITER_ARG arg;
  arg.sh = sh;
  arg.f = SHELL_STDOUT;
  return vnprintf(shell_writer, &arg, format, ap);
}
/*----------------------------------------------------------------------------*/
static C_SHELL_CONTEXT *root_context(C_SHELL *sh)
{
  return &sh->context;
}
/*----------------------------------------------------------------------------*/
static C_SHELL_CONTEXT *current_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  c = root_context(sh);
  while(c->child != NULL) {
    c = c->child;
  }
  return c;
}
/*----------------------------------------------------------------------------*/
static C_SHEL_VAR *find_var(C_SHELL *sh, unsigned hash, C_SHEL_VAR **prior /*=NULL*/)
{
  C_SHEL_VAR *n;
  if(prior != NULL)
    *prior = NULL;

  n = sh->vars;
  while(n != NULL) {
    if(n->hash == hash)
      break;

    if(prior != NULL)
      *prior = n;

    n = n->next;
  }
  return n;
}
/*----------------------------------------------------------------------------*/
static int set_var(C_SHELL *sh, const char *name, const char *value)
{
  C_SHEL_VAR *prev, *n, *r;
  unsigned size = 0, hash;

  if(value != NULL) {
    size = strlen(value);
  }

  hash = calc_hash(name);
  n = find_var(sh, hash, &prev);
  if(!size) {
    //Remove var
    if(n != NULL) {
      if(n == sh->vars) {
        sh->vars = n->next;
      } else {
        prev->next = n->next;
      }
      cache_free(sh->cache, n);
    }
  } else {
    if(n == NULL) {
      n = cache_alloc(sh->cache, sizeof(C_SHEL_VAR) + size);
      if(n != NULL) {
        n->hash = hash;
        strncpy(n->name, name, sizeof(n->name) - 1);
        n->name[sizeof(n->name) - 1] = 0;
        /*
        n->next = sh->vars;
        sh->vars = n;
        */
        /*Insert with sort*/
        prev = NULL;
        r = sh->vars;
        while(r != NULL) {
          if(strncmp(n->name, r->name, sizeof(n->name)) <= 0)
            break;
          prev = r;
          r = r->next;
        }

        if(prev == NULL) {
          n->next = sh->vars;
          sh->vars = n;
        } else {
          n->next = prev->next;
          prev->next = n;
        }
      }
    } else {
      r = cache_realloc(sh->cache, n, sizeof(C_SHEL_VAR) + size);
      if(r != n) {
        //If new pointer
        if(prev != NULL) {
          prev->next = r;
        } else {
          sh->vars = r;
        }
        n = r;
      }
    }

    if(n == NULL) {
      return SHELL_ERR_MALLOC;
    }

    memcpy(n->value, value, size);
    n->value[size] = 0;
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static const char *get_var(C_SHELL *sh, const char *name)
{
  static const char empy = 0;
  C_SHEL_VAR *v;
  unsigned hash;

  hash = calc_hash(name);
  v = find_var(sh, hash, NULL);
  if(v == NULL) {
    return &empy;
  }
  return v->value;
}
/*----------------------------------------------------------------------------*/
static void clear_vars(C_SHELL *sh)
{
  C_SHEL_VAR *v, *tmp;

  v = sh->vars;
  while (v != NULL) {
    tmp = v->next;
    cache_free(sh->cache, v);
    v = tmp;
  }
  sh->vars = NULL;
}
/*----------------------------------------------------------------------------*/
int shell_set_var(C_SHELL *sh, const char *name, const char *value)
{
  if(is_debug_mode(sh)) {
    shell_printf(sh, "set '%s' = '%s'\n", name, (value == NULL || !*value) ? "NULL" : value);
  }

  return set_var(sh, name, value);
}
/*----------------------------------------------------------------------------*/
int shell_set_int_var(C_SHELL *sh, const char *name, int value)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", value);
  return shell_set_var(sh, name, buf);
}
/*----------------------------------------------------------------------------*/
const char *shell_get_var(C_SHELL *sh, const char *name)
{
  return get_var(sh, name);
}
/*----------------------------------------------------------------------------*/
int shell_get_int_var(C_SHELL *sh, const char *name, int *value)
{
  const char *v;

  v = shell_get_var(sh, name);
  if(!*v)
    return SHELL_ERR_INVALID_ARG;
  if(value != NULL) {
    *value = atoi(v);
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int exec_str(C_SHELL *sh, const char *str, int from_cache)
{
  int i, r = SHELL_OK;
  const char *end;
  unsigned size;
  C_SHELL_PARSER *parser = NULL;

  parser = (C_SHELL_PARSER *) cache_alloc(sh->cache, sizeof(C_SHELL_PARSER));
  if(parser == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(parser, 0, sizeof(C_SHELL_PARSER));

  while(1) {
    end = NULL;
    size = strlen(str);
    for(i = 0; i < MAX_LEXER_SIZE; i++) {
      parser->argv[i] = NULL;
    }
    parser->arg0 = 0;
    parser->argc = 0;
    sh->parser = parser;

    r = lexer(str, size, &end, parser->lex, MAX_LEXER_SIZE);
    if(r < 0) {
      break;
    }

    if(r) {
      if(parser->lex[r - 1].type == LEX_COMMENT)
        r --;
    }

    if(r) {
      parser->argc = r;
      if(is_lex_debug_mode(sh)) {
        lex_print(sh, parser->lex, parser->argc);
      }

      r = args_prepare(sh, parser->lex, parser->argc, parser->argv);
      if(r < 0) {
        break;
      }

      if(is_pars_debug_mode(sh)) {
        args_print(sh, parser->argc, parser->argv);
      }

      if(!from_cache) {
        sh->op_id ++;
      }

      r = exec0(sh, parser->argc, parser->argv);

      for(i = 0; i < parser->argc; i++) {
        cache_free(sh->cache, parser->argv[i]);
        parser->argv[i] = NULL;
      }

      if(r < 0) {
        break;
      }


      if(parser->argc > 0 //Check: argc may be changed by exec
         && !from_cache
         ) {
        r = add_src_to_cache(sh,
                             parser->lex[0].start,
            parser->lex[parser->argc - 1].end - parser->lex[0].start
            );
        if(r < 0) {
          break;
        }
      }
    }

    if(end != NULL && *end) {
      str = end;
      continue;
    }
    break;
  }

  cache_free(sh->cache, parser);
  sh->parser = NULL;
  return r;
}
/*----------------------------------------------------------------------------*/
int shell_rx(C_SHELL *sh, const char *str)
{
  return exec_str(sh, str, 0);
}
/*----------------------------------------------------------------------------*/
int shell_stack_size(C_SHELL *sh)
{
  return context_stack(sh);
}
/*----------------------------------------------------------------------------*/
void shell_terminate(C_SHELL *sh)
{
  sh->terminate = 1;
}
/*----------------------------------------------------------------------------*/
static unsigned calc_hash(const char *str)
{
  unsigned h = 5381;
  int c;

  while ((c = *str++) != 0)
      h = ((h << 5) + h) + c; /* hash * 33 + c */

  return h;
}
/*----------------------------------------------------------------------------*/
/*Lexer must used in test!!!*/
static int lexer(const char *src, unsigned size, const char **end, LEX_ELEM *dst, unsigned lex_size)
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
static int is_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, SHELL_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static int is_lex_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, LEX_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static int is_pars_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, PARS_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static int is_cache_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, CACHE_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static int is_true_condition(C_SHELL *sh)
{
  return !current_context(sh)->condition;
}
/*----------------------------------------------------------------------------*/
static void set_condition(C_SHELL *sh, int c)
{
  current_context(sh)->condition = c ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
static int get_condition(C_SHELL *sh)
{
  return current_context(sh)->condition ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**Handle input/output FIFO*/
/*
  cmd1 | cmd2 | cmd 3 | cmd4
  OUT    INOUT  INOUT    IN
*/
static int exec0(C_SHELL *sh, int argc, char **argv)
{
  int i, i0 = 0, arg0, r = SHELL_OK, f = 0;

  arg0 = sh->parser->arg0;

  for(i = 0; i < argc; i++) {
    if(sh->parser->lex[arg0 + i].type == LEX_VERBAR) {
      if(sh->stream.ext_handler != NULL) {
        if(i0) {
          sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDIN]);
          sh->stream.f[SHELL_STDIN] = sh->stream.f[SHELL_STDOUT];
          sh->stream.f[SHELL_STDOUT] = 0;
        }
        f = sh->stream.ext_handler->_open(sh->stream.ext_handler->data, "", SHELL_FIFO);
        if(f < 0) {
          r = f;
          break;
        }
        sh->stream.f[SHELL_STDOUT] = f;
      }

      if(i > i0) {
        sh->parser->arg0 = arg0 + i0;
        r = exec1(sh, i - i0, &argv[i0]);
        if(r < 0) {
          break;
        }
      }
      i0 = i + 1;
    }
  }

  if(SHELL_OK == r && i > i0) {
    if(sh->stream.ext_handler != NULL) {
      if(i0) {
        sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDIN]);
        sh->stream.f[SHELL_STDIN] = sh->stream.f[SHELL_STDOUT];
      }
      sh->stream.f[SHELL_STDOUT] = 0;
    }

    sh->parser->arg0 = arg0 + i0;
    r = exec1(sh, i - i0, &argv[i0]);
  }


  if(sh->stream.ext_handler != NULL) {
    if(sh->stream.f[SHELL_STDIN] > 0) {
      sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDIN]);
    }
    if(sh->stream.f[SHELL_STDOUT] > 0) {
      sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDOUT]);
    }
    if(sh->stream.f[SHELL_STDERR] > 0) {
      sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDERR]);
    }
  }
  sh->stream.f[SHELL_STDIN] = 0;
  sh->stream.f[SHELL_STDOUT] = 0;
  sh->stream.f[SHELL_STDERR] = 0;

  return r;
}
/*----------------------------------------------------------------------------*/
static int exec1(C_SHELL *sh, int argc, char **argv)
{
  return exec3(sh, argc, argv);
}
/*----------------------------------------------------------------------------*/
static int exec3(C_SHELL *sh, int argc, char **argv)
{
  int r, i, not = 0, argc0 = 0;
  char buffer[16];

  if(sh->terminate) {
    return SHELL_EXIT - 100;
  }

  if(sh->step_cb.cb != NULL) {
    r = sh->step_cb.cb(sh->step_cb.arg, argc, argv);
    if(r < 0) {
      return r;
    }
  }

  if(is_debug_mode(sh)) {
    shell_printf(sh, "[%c] [s:%d] [c:%d] [id:%d]", is_true_condition(sh) ? 'x' : ' ', context_stack(sh), sh->cache_stack, sh->op_id);
    for(i = 0; i < argc; i++) {
      shell_printf(sh, " %s", argv[i]);
    }

    if(is_cache_debug_mode(sh)) {
      unsigned allocated = 0, free = 0, count = 0;
      cache_stat(sh->cache, &allocated, &free, &count);
      shell_printf(sh, " cache:a:%u:f:%u:c:%u\n", allocated, free, count);
    }
    else {
      shell_putc(sh, '\n');
    }

  }

  do {
    if(argc >= 2
       && sh->parser->argc >= sh->parser->arg0 + 2
       && sh->parser->lex[sh->parser->arg0 + 0].type == LEX_STR
       && sh->parser->lex[sh->parser->arg0 + 1].type == LEX_ASSIGN) {
      r = _assign(sh, argc, argv);
      break;
    }

    while (argc >= 1
       && sh->parser->argc >= sh->parser->arg0 + 1
       && sh->parser->lex[sh->parser->arg0 + 0].type == LEX_NOT) {
      //Shift args
      sh->parser->arg0 ++;
      argc0 ++;
      argc --;
      not = !not;
    }


    r = embed_exec(sh, argc, &argv[argc0]);
    if(r == SHELL_ERR_COMMAND_NOT_FOUND) {
      if(is_true_condition(sh) && sh->exec_cb.cb != NULL) {
        r = sh->exec_cb.cb(sh->exec_cb.arg, argc, &argv[argc0]);
      } else {
        r = SHELL_OK;
      }
    }

  } while (0);

  if(not && r >= 0)
    r = !r;

  if(r >= 0 && sh->terminate) {
    r = SHELL_EXIT - 100;
  }

  if(r >= 0) {
    snprintf(buffer, sizeof(buffer), "%d", r);
    set_var(sh, "_", buffer);
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int embed_exec(C_SHELL *sh, int argc, char **argv)
{
  unsigned i, hash;

  if(argc < 1)
    return SHELL_ERR_INVALID_ARG;

  hash = calc_hash(argv[0]);
  for(i = 0; i < sizeof(std_fn)/sizeof(std_fn[0]); i++) {
    if(std_fn[i].hash == hash) {
       return std_fn[i].handler(sh, argc, argv);
    }
  }
  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
static int _eXit(C_SHELL *sh, int argc, char **argv)
{
  int exit_code = 0;

  (void) sh;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;
  if(argc > 1)
    exit_code = atoi(argv[1]);

  exit_code %= 1000;

  return SHELL_EXIT - (exit_code > 0 ? exit_code : - exit_code);
}
/*----------------------------------------------------------------------------*/
static int _if(C_SHELL *sh, int argc, char **argv)
{
  int r1, r2;

  if(argc < 2)
    return SHELL_ERR_INVALID_ARG;

  if(is_true_condition(sh)) {
    sh->parser->arg0 ++; //Shift args
    r1 = exec3(sh, argc - 1, &argv[1]);
    if(r1 < 0)
      return r1;
  }
  else
    r1 = 1;

  r2 = push_context(sh);
  if(r2 < 0)
    return r2;

  set_condition(sh, r1);
  set_opcode(sh, OP_IF);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _then(C_SHELL *sh, int argc, char **argv)
{
  if(current_opcode(sh) != OP_IF) {
    return SHELL_ERR_INVALID_OP;
  }

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    return   exec3(sh, argc - 1, &argv[1]);
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _fi(C_SHELL *sh, int argc, char **argv)
{
  (void) argc;
  (void) argv;

  if(current_opcode(sh) == OP_IF) {
    return pop_context(sh);
  }
  return SHELL_ERR_INVALID_OP;
}
/*----------------------------------------------------------------------------*/
static int _else(C_SHELL *sh, int argc, char **argv)
{

  (void) argc;
  (void) argv;

  if(current_opcode(sh) == OP_IF) {
    if(!current_context(sh)->parent->condition) { //Parent condition
      set_condition(sh, !get_condition(sh));
      if(argc > 1) {
        sh->parser->arg0 ++; //Shift args
        return   exec3(sh, argc - 1, &argv[1]);
      }
    }
    return SHELL_OK;
  }
  return SHELL_ERR_INVALID_OP;
}
/*----------------------------------------------------------------------------*/
static int _elif(C_SHELL *sh, int argc, char **argv)
{
  int r;
  r = _else(sh, argc, argv);
  if(r < 0)
    return r;
  return _if(sh, argc, argv);
}
/*----------------------------------------------------------------------------*/
static int _true(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int _false(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;
  return 1;
}
/*----------------------------------------------------------------------------*/
static int _while(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;

  c = current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = push_context(sh)) < 0)
      return r;

    c = current_context(sh);
    c->op_code = OP_WHILE;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }


  if(is_true_condition(sh))
  {
    r = 0;
    if(argc > 1) {
      sh->parser->arg0 ++; //Shift args
      r = exec3(sh, argc - 1, &argv[1]);
      if(r < 0) {
        return r;
      }
    }

    set_condition(sh, r);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _until(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;

  c = current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = push_context(sh)) < 0)
      return r;

    c = current_context(sh);
    c->op_code = OP_WHILE;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }


  if(is_true_condition(sh))
  {
    r = 0;
    if(argc > 1) {
      sh->parser->arg0 ++; //Shift args
      r = exec3(sh, argc - 1, &argv[1]);
      if(r < 0) {
        return r;
      }
    }

    set_condition(sh, !r);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _for(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *c;
  int r;
  if(argc < 4
    || argv[2][0] != 'i'
    || argv[2][1] != 'n'
    || argv[2][2] != '\0')
    return SHELL_ERR_INVALID_ARG;

  c = current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = push_context(sh)) < 0)
      return r;
    c = current_context(sh);
    c->op_code = OP_IF;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }
  else
  {
    c->for_index ++;
  }


  if(c->for_index < argc - 3 && is_true_condition(sh))
  {
    set_true_condition(sh);
    set_var(sh, argv[1], argv[c->for_index + 3]);
  }
  else
  {
    set_false_condition(sh);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _do(C_SHELL *sh, int argc, char **argv)
{

  if(!current_context(sh)->is_loop) {
    return SHELL_ERR_INVALID_OP;
  }

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    return   exec3(sh, argc - 1, &argv[1]);
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _and(C_SHELL *sh, int argc, char **argv)
{
  int cond = NOT_TRUE_CONDITION;
  if(!is_true_condition(sh))
    return NOT_TRUE_CONDITION;

  shell_get_int_var(sh, "_", &cond);

  if(argc > 1 && !cond) {
    sh->parser->arg0 ++; //Shift args
    cond = exec3(sh, argc - 1, &argv[1]);
  }
  return cond;
}
/*----------------------------------------------------------------------------*/
static int _or(C_SHELL *sh, int argc, char **argv)
{
  int cond = NOT_TRUE_CONDITION;
  if(!is_true_condition(sh))
    return NOT_TRUE_CONDITION;

  shell_get_int_var(sh, "_", &cond);

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    cond = exec3(sh, argc - 1, &argv[1]);
  }
  return cond;
}
/*----------------------------------------------------------------------------*/
static int _done(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;
  (void) argc;
  (void) argv;

  c = current_context(sh);

  if(!c->is_loop) {
    return SHELL_ERR_INVALID_OP;
  }

  if(c->is_continue) {
    set_true_condition(sh);
    c->is_continue = 0;
  }

  while(is_true_condition(sh)) {
    r = play_cache(sh);
    if(r < 0)
      return r;

    if(c->is_continue) {
      set_true_condition(sh);
      c->is_continue = 0;
    }
  }

  pop_context(sh);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _break(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *loop_context, *c;

  (void) argc;
  (void) argv;

  loop_context = current_context(sh);
  while(loop_context != NULL) {
    if(loop_context->is_loop) {
      break;
    }
    loop_context = loop_context->parent;
  }

  if(loop_context == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  if(!is_true_condition(sh))
    return 0;


  //Handle if
  c = loop_context->child;
  while (c != NULL) {
    c->condition = NOT_TRUE_CONDITION;
    c = c->child;
  }


  loop_context->condition = NOT_TRUE_CONDITION;
  loop_context->is_continue = 0;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _continue(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *loop_context, *c;

  (void) argc;
  (void) argv;

  loop_context = current_context(sh);
  while(loop_context != NULL) {
    if(loop_context->is_loop) {
      break;
    }
    loop_context = loop_context->parent;
  }

  if(loop_context == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  if(!is_true_condition(sh))
    return 0;


  //Handle if
  c = loop_context->child;
  while (c != NULL) {
    c->condition = NOT_TRUE_CONDITION;
    c = c->child;
  }


  loop_context->condition = NOT_TRUE_CONDITION;
  loop_context->is_continue = 1;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _echo(C_SHELL *sh, int argc, char **argv)
{
  int i = 1, endl=1, first = 1;
  const char *p;

  (void) sh;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;

  if(argc > 1 && !strcmp(argv[1], "-n")) {
    i ++;
    endl = 0;
  }


  for(; i < argc; i++) {
    if(!first) {
      if(sh->parser != NULL && sh->parser->arg0 + i - 1 < sh->parser->argc) {
        p = sh->parser->lex[sh->parser->arg0 + i - 1].end;
        while(p < sh->parser->lex[sh->parser->arg0 + i].start) {
          shell_putc(sh, *p);
          p ++;
        }

      }
    }
    shell_puts(sh, argv[i]);
    first = 0;
  }

  if(endl)
    shell_putc(sh, '\n');

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _test(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;

#ifdef USE_SH_TEST
  return test_main(argc, argv);
#else //USE_SH_TEST
  return intern_test(sh, argc, argv);
#endif //USE_SH_TEST

  return SHELL_ERR_NOT_IMPLEMENT;
}
/*----------------------------------------------------------------------------*/
static int _set(C_SHELL *sh, int argc, char **argv)
{
  C_SHEL_VAR *v;
  unsigned allocated=0, free=0, count=0;
  (void) argc;
  (void) argv;
  if(!is_true_condition(sh))
    return 0;

  //Output vars
  v = sh->vars;
  while (v != NULL) {
    shell_printf(sh, "%s='%s'\n", v->name, v->value);
    v = v->next;
  }
  //Get cache stat
  cache_stat(sh->cache, &allocated, &free, &count);
  shell_puts(sh, "#cache\n");
  shell_printf(sh, "%s=%u\n", "allocated", allocated);
  shell_printf(sh, "%s=%u\n", "free", free);
  shell_printf(sh, "%s=%u\n", "count", count);



  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _assign(C_SHELL *sh, int argc, char **argv)
{
  if(!is_true_condition(sh) || argc < 2)
    return TRUE_CONDITION;

  //TODO: addition substitute vars
  shell_set_var(sh, argv[0], argv[2]);

  if(argc > 3) {
    sh->parser->arg0 += 3; //Shift args
    return   exec3(sh, argc - 3, &argv[3]);
  }

  return SHELL_OK;
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
static void lex_print(C_SHELL *sh, const LEX_ELEM *args, int size)
{
  int i;
  const char *p;
  shell_printf(sh, "LEX:argc:%d {\n", size);
  for(i = 0; i < size; i++) {
    shell_printf(sh, "%16s%8s`", lex_name(args[i].type), "");
    for(p = args[i].start; p < args[i].end; p++) {
      shell_putc(sh, *p);
    }
    shell_putc(sh, '\'');
    shell_putc(sh, '\n');
  }
  shell_printf(sh, "%10s\n", "}");
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
static unsigned var_subst(C_SHELL *sh, const char *begin, const char *end, char *buffer, unsigned buffer_size)
{
  char *name = NULL;
  const char *value;
  unsigned size, i;
  if(*begin == '{') {
    begin ++;
  }

  //if(*end == '}') {
  //  end --;
  //}

  if(end <= begin)
    return 0;

  size = end - begin;
  name = cache_alloc(sh->cache, size + 1);
  if(name == NULL)
    return 0;
  memcpy(name, begin, size);
  name[size] = 0;
  value = shell_get_var(sh, name);
  cache_free(sh->cache, name);

  size = strlen(value);
  for(i = 0; i < size && i < buffer_size; i++) {
    buffer[i] = value[i];
  }
  return i;
}
/*----------------------------------------------------------------------------*/
static unsigned var_substitution(C_SHELL *sh, const char **src, unsigned size, char *buffer, unsigned buffer_size)
{
  static const char delimiters[] = "}$/\\ \t+*-=><!$&()";
  const char *p;
  unsigned i, dst_index = 0;
  p = *src;

  for(i = 0; i < size; i++) {
    if(strchr(delimiters, p[i]) != NULL) {
      dst_index = var_subst(sh, *src, &p[i], buffer, buffer_size);
      break;
    }
  }

  if(i >= size) {
    dst_index = var_subst(sh, *src, &p[size], buffer, buffer_size);
  }

  *src = &p[i];
  return dst_index;
}
/*----------------------------------------------------------------------------*/
static unsigned string_prepare(C_SHELL *sh, const LEX_ELEM *src, char *buffer, unsigned buffer_size)
{
  const char *p, *end;
  unsigned i;
  int use_vars = 1;

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

    if(*p == '$' && use_vars) {
      p ++;
      i += var_substitution(sh, &p, end - p, &buffer[i], buffer_size - i);
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
static int args_prepare(C_SHELL *sh, const LEX_ELEM *args, int size, char **argv)
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
static void args_print(C_SHELL *sh, int argc, char **argv)
{
  int i;
  shell_printf(sh, "PARSER:argc:%d\n", argc);
  for(i = 0; i < argc; i++) {
    shell_printf(sh, "args[%d]:'%s'\n", i, argv[i]);
  }
  shell_printf(sh, "%10s\n", "}");
}
/*----------------------------------------------------------------------------*/
static int context_stack(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  int s = 0;

  c = root_context(sh)->child;
  while(c != NULL) {
    s ++;
    c = c->child;
  }
  return s;
}
/*----------------------------------------------------------------------------*/
static int push_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  c = current_context(sh);
  c->child = cache_alloc(sh->cache, sizeof(C_SHELL_CONTEXT));
  if(c->child == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(c->child, 0, sizeof(C_SHELL_CONTEXT));
  c->child->parent = c;
  c->child->condition = c->condition;
  c->child->op_id = sh->op_id;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int pop_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c, *parent;

  c = current_context(sh);
  parent = c->parent;

  if(parent == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  remove_cache(sh);
  parent->child = NULL;
  cache_free(sh->cache, c);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int add_src_to_cache(C_SHELL *sh, const char *src, unsigned size)
{
  C_SHELL_LINE_CACHE *lc = NULL;
  C_SHELL_CONTEXT *c;

  c = root_context(sh);
  while(c != NULL) {
    if(c->should_store_cache) {
      if(lc == NULL) {
        lc = cache_alloc(sh->cache, sizeof(C_SHELL_LINE_CACHE) + size);
        if(lc == NULL) {
          return SHELL_ERR_MALLOC;
        }
        memset(lc, 0, sizeof(C_SHELL_LINE_CACHE) + size);
        memcpy(lc->text, src, size);
        lc->text[size] = 0;
        lc->op_id = sh->op_id;
        lc->next = NULL;
        if(c->cache.root == NULL) {
          c->cache.root = lc;
        }
      }

      if(c->cache.first == NULL) {
        c->cache.first = lc;
      } else {
        c->cache.last->next = lc;
      }
      c->cache.last = lc;
    }
    c = c->child;
  }
  if(is_debug_mode(sh) && lc != NULL) {
    shell_printf(sh, "Cashed:*:s:%d:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
static void set_context_should_store_cache(C_SHELL *sh, int should_store)
{
  current_context(sh)->should_store_cache = should_store ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
static void add_cache_to_cache(C_SHELL *sh, C_SHELL_CONTEXT *parent, C_SHELL_LINE_CACHE *lc)
{
  C_SHELL_CONTEXT *c;
  int add = 0;

  c = parent->child;
  while(c != NULL) {
    if(c->should_store_cache) {
      add = 1;
      if(c->cache.first == NULL) {
        c->cache.first = lc;
      } else {
        c->cache.last->next = lc;
      }
      c->cache.last = lc;
    }
    c = c->child;
  }

  if(is_debug_mode(sh) && add) {
    shell_printf(sh, "Cashed:s:%d:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
  }
}
/*----------------------------------------------------------------------------*/
static int play_cache(C_SHELL *sh)
{
  int r = SHELL_OK;
  int cache_mode;
  int op_id, stack;
  C_SHELL_CONTEXT *c;
  C_SHELL_LINE_CACHE *lc;

  c = current_context(sh);
  stack = context_stack(sh);
  lc = c->cache.first;
  if(lc == NULL) {
    return SHELL_ERR_INVALID_OP;
  }
  cache_mode = is_cache_mode(sh);
  op_id = sh->op_id;
  set_cache_mode(sh, 1);
  sh->cache_stack ++;
  while (lc != NULL) {
    sh->op_id = lc->op_id;
    r = exec_str(sh, lc->text, 1);
    if(context_stack(sh) < stack) {
      return SHELL_ERR_INVALID_OP;
    }
    add_cache_to_cache(sh, c, lc);
    if(r < 0) {
      break;
    }

    if(lc == c->cache.last)
      break;

    lc = lc->next;
  }
  set_cache_mode(sh, cache_mode);
  sh->cache_stack --;
  if(!cache_mode) {
    sh->op_id = op_id;
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int remove_cache(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  C_SHELL_LINE_CACHE *lc, *next;

  c = current_context(sh);
  if(c->cache.root != NULL) {
    //cache root
    lc = c->cache.root;
    c->cache.root = NULL;
    while (lc != NULL) {
      next = lc->next;
      if(is_debug_mode(sh)) {
        shell_printf(sh, "Rm cache[%d]:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
      }
      cache_free(sh->cache, lc);
      lc = next;
    }
  }

  while (c != NULL) {
    c->cache.first = NULL;
    c->cache.last = NULL;
    c = c->child;
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static void set_cache_mode(C_SHELL *sh, int mode)
{
  sh->cache_mode = mode ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
static int is_cache_mode(C_SHELL *sh)
{
  return   sh->cache_mode  ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
static void set_opcode(C_SHELL *sh, OP_CODE c)
{
  current_context(sh)->op_code = c;
}
/*----------------------------------------------------------------------------*/
static OP_CODE current_opcode(C_SHELL *sh)
{
  return   current_context(sh)->op_code;
}
/*----------------------------------------------------------------------------*/
#ifndef USE_SH_TEST
/*----------------------------------------------------------------------------*/
typedef struct {
  C_SHELL *sh;
  int arg0;
  int argc;
  LEX_ELEM lex[MAX_LEXER_SIZE];
  char *argv[MAX_LEXER_SIZE];
} TEST_DATA;
/*----------------------------------------------------------------------------*/
static int error_msg(TEST_DATA *data, const char *fmt, ...)
{
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = shell_vprintf(data->sh, fmt, ap);
  va_end(ap);
  r += shell_putc(data->sh, '\n');
  return r;
}
/*----------------------------------------------------------------------------*/
static void syntax(TEST_DATA *data, const char *op, const char *msg)
{
  if (op && *op) {
    error_msg(data, "%s: %s", op, msg);
  } else {
    error_msg(data, "%s", msg);
  }
}
/*----------------------------------------------------------------------------*/
static void debug_msg(TEST_DATA *data, const char *op)
{
  int i;
  if(!is_debug_mode(data->sh))
    return;
  shell_printf(data->sh, "<<op:%s:", op == NULL ? "" : op);
  shell_printf(data->sh, "arg0:%d:argc:%d", data->arg0, data->argc);

  for(i = data->arg0; i < data->argc; i++) {
    shell_printf(data->sh, ":%s", data->argv[i]);
  }
  shell_printf(data->sh, ">>\n");
}
/*----------------------------------------------------------------------------*/
/* test(1) accepts the following grammar:
  oexpr	::= aexpr | aexpr "-o" oexpr ;
  aexpr	::= nexpr | nexpr "-a" aexpr ;
  nexpr	::= primary | "!" primary
  primary ::= unary-operator operand
    | operand binary-operator operand
    | operand
    | "(" oexpr ")"
    ;
  unary-operator ::= "!";

  binary-operator ::= "="|"=="|"!="|">="|">"|"<="|"<"
  operand ::= <any legal UNIX file name>
*/
/*----------------------------------------------------------------------------*/
static int oexpr(TEST_DATA *data);
static int aexpr(TEST_DATA *data);
static int nexpr(TEST_DATA *data);
static int binop(TEST_DATA *data);
static int operand(TEST_DATA *data);
static int primary(TEST_DATA *data);
enum {
  LEX_IEQ = 100   // -eq
  , LEX_INE = 101 // -ne
  , LEX_IGT = 102 // -gt
  , LEX_IGE = 103 // -ge
  , LEX_ILT = 104 // -lt
  , LEX_ILE = 105 // -li
} NUM_COMPARE;
/*----------------------------------------------------------------------------*/
static int intern_test(C_SHELL *sh, int argc, char **argv)
{
  int res, i;
  TEST_DATA *data;


  /*Use arggs from shell parser*/
  (void) argc;
  (void) argv;

  data = (TEST_DATA *) cache_alloc(sh->cache, sizeof(TEST_DATA));
  if(data == NULL) {
    return SHELL_ERR_MALLOC;
  }

  memset(data, 0, sizeof(TEST_DATA));

  do {
    data->sh = sh;
    data->argc = argc;
    data->arg0 = 0;

    for(i = 0; i < sh->parser->argc - sh->parser->arg0; i++) {
      data->lex[i] = sh->parser->lex[i + sh->parser->arg0];
      data->argv[i] = sh->parser->argv[i + sh->parser->arg0];

      if(data->lex[i].type == LEX_STR) {
        //Addition parse for short option
        if(!strcmp(data->argv[i], "-a")) {
          data->lex[i].type = LEX_AND;
          continue;
        }
        if(!strcmp(data->argv[i], "-o")) {
          data->lex[i].type = LEX_OR;
          continue;
        }
        if(!strcmp(data->argv[i], "-eq")) {
          data->lex[i].type = LEX_IEQ;
          continue;
        }
        if(!strcmp(data->argv[i], "-ne")) {
          data->lex[i].type = LEX_INE;
          continue;
        }
        if(!strcmp(data->argv[i], "-gt")) {
          data->lex[i].type = LEX_IGT;
          continue;
        }
        if(!strcmp(data->argv[i], "-ge")) {
          data->lex[i].type = LEX_IGE;
          continue;
        }
        if(!strcmp(data->argv[i], "-lt")) {
          data->lex[i].type = LEX_ILT;
          continue;
        }
        if(!strcmp(data->argv[i], "-le")) {
          data->lex[i].type = LEX_ILE;
          continue;
        }
      }
    }

    //Addition string handle: skeep spaces
    for(i = 0; i < data->argc; i++) {
      if(data->lex[i].type == LEX_STR) {
        while(data->argv[i][0] == ' ') {
          data->argv[i] ++;
        }
      }
    }

    debug_msg(data, "test");
    if(data->lex[0].type == LEX_LSQB) {
      data->arg0 ++;
      data->argc --;
      if(data->lex[data->argc].type != LEX_RSQB) {
        error_msg(data, "missing ]");
        res = 2;
        break;
      }
    }
    else
    if(data->lex[0].type == LEX_LDSQB) {
      data->argc --;
      data->arg0 ++;
      if(data->lex[data->argc].type != LEX_RDSQB) {
        error_msg(data, "missing ]]");
        res = 2;
        break;
      }
    }
    else {
      data->arg0 ++;
    }

    if (data->argc <= data->arg0) {
      res = 1;
      break;
    }

    res = !oexpr(data);
  } while(0);

  cache_free(sh->cache, data);
  return res;
}
/*----------------------------------------------------------------------------*/
static int oexpr(TEST_DATA *data)
{
  int res;
  debug_msg(data, "oexpr");
  res = aexpr(data);
  if (data->lex[data->arg0].type == LEX_OR) {
    data->arg0 ++;
    return oexpr(data) || res;
  }
  return res;
}
/*----------------------------------------------------------------------------*/
static int aexpr(TEST_DATA *data)
{
  int res;

  debug_msg(data, "aexpr");
  res = nexpr(data);
  if (data->lex[data->arg0].type == LEX_AND) {
    data->arg0 ++;
    return aexpr(data) && res;
  }
  return res;
}
/*----------------------------------------------------------------------------*/
static int nexpr(TEST_DATA *data)
{
  debug_msg(data, "nexpr");
  if (data->lex[data->arg0].type == LEX_NOT) {
    data->arg0 ++;
    return !nexpr(data);
  }
  return primary(data);
}
/*----------------------------------------------------------------------------*/
static int primary(TEST_DATA *data)
{
  int res;
  debug_msg(data, "primary");
  if (data->argc <= data->arg0) {
    syntax(data, NULL, "Argument expected");
    return 0;
  }

  if(data->lex[data->arg0].type == LEX_LPAREN) {
    data->arg0 ++;
    res = oexpr(data);
    if(data->lex[data->arg0].type != LEX_RPAREN) {
      syntax(data, NULL, "Closing paren expected");
    }
    data->arg0 ++;
    return res;
  }

  if(data->arg0 < data->argc + 2
     && data->lex[data->arg0].type == LEX_STR
     && data->lex[data->arg0 + 2].type == LEX_STR)
     {
    res = binop(data);
    data->arg0 += 3;
    return res;
  }

  if(data->lex[data->arg0].type == LEX_STR) {
    res = operand(data);
    data->arg0 ++;
    return res;
  }

  syntax(data, data->argv[data->arg0], "Invalid operation");
  return 2;
}
/*----------------------------------------------------------------------------*/
static int binop(TEST_DATA *data)
{
  debug_msg(data, "binop");
  switch (data->lex[data->arg0 + 1].type) {
    case LEX_EQ: case LEX_ASSIGN:
      return !strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]);
    case LEX_NE:
      return strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]);
    case LEX_GT:
      return strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]) > 0;
    case LEX_GE:
      return strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]) >= 0;
    case LEX_LT:
      return strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]) < 0;
    case LEX_LE:
      return strcmp(data->argv[data->arg0], data->argv[data->arg0 + 2]) <= 0;

    case LEX_IEQ:
      return atoi(data->argv[data->arg0]) == atoi(data->argv[data->arg0 + 2]);
    case LEX_INE:
      return atoi(data->argv[data->arg0]) != atoi(data->argv[data->arg0 + 2]);
    case LEX_IGT:
      return atoi(data->argv[data->arg0]) > atoi(data->argv[data->arg0 + 2]);
    case LEX_IGE:
      return atoi(data->argv[data->arg0]) >= atoi(data->argv[data->arg0 + 2]);
    case LEX_ILT:
      return atoi(data->argv[data->arg0]) < atoi(data->argv[data->arg0 + 2]);
    case LEX_ILE:
      return atoi(data->argv[data->arg0]) <= atoi(data->argv[data->arg0 + 2]);
  }
  syntax(data, data->argv[data->arg0 + 1], "Invalid operator");
  return 2;
}
/*----------------------------------------------------------------------------*/
static int operand(TEST_DATA *data)
{
  debug_msg(data, "operand");
  return data->argv[data->arg0] != NULL && *data->argv[data->arg0] != 0;
}
/*----------------------------------------------------------------------------*/
#endif //USE_SH_TEST
