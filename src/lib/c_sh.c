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
//#define USE_SH_TEST //Use external test function
#ifdef USE_SH_TEST
  #include "c_sh_test.h"
#endif //USE_SH_TEST
/*----------------------------------------------------------------------------*/
#define DEBUG_MOODE
/*----------------------------------------------------------------------------*/
static int context_stack(C_SHELL *sh);
static void clear_vars(C_SHELL *sh);
static int add_src_to_cache(C_SHELL *sh, const char *src, unsigned size);
static void set_context_should_store_cache(C_SHELL *sh, int should_store);
/*----------------------------------------------------------------------------*/
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
    case SHELL_ERROR_OPEN_FILE:
      str = "Error open file";
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
  //Intern stream handler;
  sh->stream.ext_handler = cache_alloc(sh->cache, sizeof(SHELL_STREAM_HANDLER));
  if(sh->stream.ext_handler != NULL) {
    sh->stream.ext_handler->data = sh;
    sh->stream.ext_handler->_open = sh_stream_open;
    sh->stream.ext_handler->_close = sh_stream_close;
    sh->stream.ext_handler->_read = sh_stream_read;
    sh->stream.ext_handler->_write = sh_stream_write;
  }

  return sh;
}
/*----------------------------------------------------------------------------*/
void shell_reset(C_SHELL *sh)
{
  C_SHELL_INTERN_STREAM *s;
  while(context_stack(sh)) {
    sh_pop_context(sh);
  }
  sh_set_cache_mode(sh, 0);
  set_context_should_store_cache(sh, 0);
  sh_remove_cache(sh);
  clear_vars(sh);
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

  while(sh->intern_stream != NULL) {
    s = sh->intern_stream;
    sh->intern_stream = s->next;
    if(s->buffer != NULL) {
      cache_free(sh->cache, s->buffer);
    }
    cache_free(sh->cache, s);
  }
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
       && data->sh->stream.f[data->f] > 0) {
      r = data->sh->stream.ext_handler->_write(data->sh->stream.ext_handler->data,
          data->sh->stream.f[data->f],
          txt,
          size);
      if(r <= 0) {
        return r;
      }
      ret += r;
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
int shell_isaprint(C_SHELL *sh, SHELL_STREAM_ID f)
{
  switch (f) {
    case SHELL_STDOUT:
    case SHELL_STDERR:
      if(sh->stream.ext_handler != NULL
         && sh->stream.f[f] > 0) {
        return 0;
      }
      return sh->print_cb.cb != NULL;
      break;
    default:
      break;
  }

  return 0;
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
     && sh->stream.f[SHELL_STDIN] > 0) {
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
C_SHELL_CONTEXT *sh_root_context(C_SHELL *sh)
{
  return &sh->context;
}
/*----------------------------------------------------------------------------*/
C_SHELL_CONTEXT *sh_current_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  c = sh_root_context(sh);
  while(c->child != NULL) {
    c = c->child;
  }
  return c;
}
/*----------------------------------------------------------------------------*/
C_SHEL_VAR *sh_find_var(C_SHELL *sh, unsigned hash, C_SHEL_VAR **prior /*=NULL*/)
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
int sh_set_var(C_SHELL *sh, const char *name, const char *value)
{
  C_SHEL_VAR *prev, *n, *r;
  unsigned size = 0, hash;

  if(value != NULL) {
    size = strlen(value);
  }

  hash = sh_hash(name);
  n = sh_find_var(sh, hash, &prev);
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
const char *sh_get_var(C_SHELL *sh, const char *name)
{
  static const char empy = 0;
  C_SHEL_VAR *v;
  unsigned hash;

  hash = sh_hash(name);
  v = sh_find_var(sh, hash, NULL);
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
  if(sh_is_debug_mode(sh)) {
    shell_fprintf(sh, SHELL_STDERR, "set '%s' = '%s'\n", name, (value == NULL || !*value) ? "NULL" : value);
  }

  return sh_set_var(sh, name, value);
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
  return sh_get_var(sh, name);
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
  const char *end, *p;
  unsigned size, buffer_size = 16*1024;
  char *buffer = NULL;

  typedef struct {
    C_SHELL_PARSER parser;
    const char *src_start;
    const char *src_end;
  } exec_str_t;

  exec_str_t *data;

  size = strlen(str);
  data = (exec_str_t *) cache_alloc(sh->cache, sizeof(exec_str_t));
  if(data == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(data, 0, sizeof(exec_str_t));

  while(1) {
    end = NULL;
    size = strlen(str);
    data->parser.arg0 = 0;
    data->parser.argc = 0;

    //Lexer only to find end of command
    r = cmdline_lexer(str, size, &end, data->parser.lex, MAX_LEXER_SIZE);
    if(r < 0) {
      break;
    }

    if(r) {
      if(data->parser.lex[r - 1].type == LEX_COMMENT)
        r --;
    }

    if(r) {
      data->src_start = data->parser.lex[0].start;
      data->src_end = data->parser.lex[r - 1].end;

      if(buffer == NULL) {
        buffer = cache_alloc(sh->cache, buffer_size);
      } else {
        buffer = cache_realloc(sh->cache, buffer, buffer_size);
      }
      if(buffer == NULL) {
        r = SHELL_ERR_MALLOC;
        break;
      }
/*
      r = sh_make_substs(sh, data->src_start, data->src_end - data->src_start, buffer, buffer_size);
      if(r < 0) {
        break;
      }
      size = r;
      buffer = cache_realloc(sh->cache, buffer, size + 1);
      r = sh_lexer(buffer, size, &p, data->parser.lex, MAX_LEXER_SIZE);
      if(r < 0) {
        break;
      }
*/
    }


    if(r) {
      data->parser.argc = r;

      if(sh_is_lex_debug_mode(sh)) {
        lex_printf(sh, data->parser.lex, data->parser.argc);
      }

      r = sh_make_argv(sh, &data->parser);
      if(r < 0) {
        break;
      }

      if(sh_is_pars_debug_mode(sh)) {
        argv_printf(sh, data->parser.argc, data->parser.argv);
      }

      if(!from_cache) {
        sh->op_id ++;
      }

      sh->parser = &data->parser;
      r = sh_exec1(sh, data->parser.argc, data->parser.argv, 0);
      sh->parser = NULL;

      for(i = 0; i < MAX_LEXER_SIZE; i++) {
        if(data->parser.argv[i] != NULL) {
          cache_free(sh->cache, data->parser.argv[i]);
          data->parser.argv[i] = NULL;
        }
      }

      if(r < 0) {
        break;
      }


      if(!from_cache) {
        r = add_src_to_cache(sh, data->src_start, data->src_end - data->src_start);
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
  if(buffer != NULL) {
    cache_free(sh->cache, buffer);
  }
  cache_free(sh->cache, data);
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
unsigned sh_hash(const char *str)
{
  unsigned h = 5381;
  int c;

  while ((c = *str++) != 0)
      h = ((h << 5) + h) + c; /* hash * 33 + c */

  return h;
}
/*----------------------------------------------------------------------------*/
int sh_is_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, SHELL_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
int sh_is_lex_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, LEX_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
int sh_is_pars_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, PARS_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
int sh_is_cache_debug_mode(C_SHELL *sh)
{
#ifdef DEBUG_MOODE
  return atoi(shell_get_var(sh, CACHE_DEBUG_VAR_NAME));
#else
  return 0;
#endif
}
/*----------------------------------------------------------------------------*/
int sh_is_true_condition(C_SHELL *sh)
{
  return !sh_current_context(sh)->condition;
}
/*----------------------------------------------------------------------------*/
void sh_set_condition(C_SHELL *sh, int c)
{
  sh_current_context(sh)->condition = c ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
void sh_set_true_condition(C_SHELL *sh)
{
  sh_set_condition(sh, TRUE_CONDITION);
}
/*----------------------------------------------------------------------------*/
void sh_set_false_condition(C_SHELL *sh)
{
  sh_set_condition(sh, NOT_TRUE_CONDITION);
}
/*----------------------------------------------------------------------------*/
int sh_get_condition(C_SHELL *sh)
{
  return sh_current_context(sh)->condition ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**Handle input/output FIFO*/
/*
  cmd1 | cmd2 | cmd 3 | cmd4
  OUT    INOUT  INOUT    IN
*/
//Param stdout - File descriptor - where to redirect the latter in the chain output
int sh_exec1(C_SHELL *sh, int argc, char **argv, int f_stdout)
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
        r = sh_exec2(sh, i - i0, &argv[i0]);
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
      sh->stream.f[SHELL_STDOUT] = f_stdout;
    }

    sh->parser->arg0 = arg0 + i0;
    r = sh_exec2(sh, i - i0, &argv[i0]);
  }


  if(sh->stream.ext_handler != NULL) {
    if(sh->stream.f[SHELL_STDIN] > 0) {
      sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[SHELL_STDIN]);
    }
    if(sh->stream.f[SHELL_STDOUT] > 0 && sh->stream.f[SHELL_STDOUT] != f_stdout) {
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
/*Handle STDIN/STDOUT/STDERR redirect
 cmd < IN
 cmd < IN 1> OUT 2> ERR
 cmd  1>> OUT 2>> ERR < IN
 cmd  2> &1
*/
int sh_exec2(C_SHELL *sh, int argc, char **argv)
{
  int i, i0, redirect, f;
  int argc_new;
  SHELL_STREAM_MODE mode;
  SHELL_STREAM_ID stream1, stream2;
  const char *filename;

  argc_new = argc;

  //Hanlde > file and < file
  for(i = 0; i < argc - 1; i++) {
    redirect = 0;
    // > file
    if(sh->parser->lex[sh->parser->arg0 + i].type == LEX_GT
       && sh->parser->lex[sh->parser->arg0 + i + 1].type == LEX_STR
       ) {
      mode = SHELL_OUT;
      stream1 = SHELL_STDOUT;
      i0 = i;
      filename = argv[i + 1];

      if(i0 && sh->parser->lex[sh->parser->arg0 + i0 - 1].type == LEX_GT) { //>>
        mode = SHELL_APPEND;
        i0 --;
      }

      //1>
      if(i0
         && sh->parser->lex[sh->parser->arg0 + i0 - 1].type == LEX_STR
         && sh->parser->lex[sh->parser->arg0 + i0].end
          - sh->parser->lex[sh->parser->arg0 + i0 - 1].start == 2) {
          switch(*sh->parser->lex[sh->parser->arg0 + i0 - 1].start) {
            case '1':
              i0 --;
              stream1 = SHELL_STDOUT;
              break;
            case '2':
              i0 --;
              stream1 = SHELL_STDERR;
              break;
          }
        }

      if(argc_new > i0) {
        argc_new = i0;
      }
      redirect = 1;
    } //> file

    // < file
    if(sh->parser->lex[sh->parser->arg0 + i].type == LEX_LT
       && sh->parser->lex[sh->parser->arg0 + i + 1].type == LEX_STR
       ) {
      mode = SHELL_IN;
      stream1 = SHELL_STDIN;
      i0 = i;
      filename = argv[i + 1];

      // 0<
      if(i0
         && sh->parser->lex[sh->parser->arg0 + i0 - 1].type == LEX_STR
         && sh->parser->lex[sh->parser->arg0 + i0].end
          - sh->parser->lex[sh->parser->arg0 + i0 - 1].start == 2) {
        switch(*sh->parser->lex[sh->parser->arg0 + i0 - 1].start) {
          case '0':
            i0 --;
            break;
        }
      }

      if(argc_new > i0) {
        argc_new = i0;
      }
      redirect = 1;
    } // < file


    if(redirect
      && sh->stream.ext_handler != NULL) {

      if(sh->stream.f[stream1] > 0) {
        sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[stream1]);
        sh->stream.f[stream1] = 0;
      }
      f = sh->stream.ext_handler->_open(sh->stream.ext_handler->data, filename, mode);
      if(f < 0) {
        return f;
      }
      sh->stream.f[stream1] = f;
    }
  }

  //Hanlde >&
  for(i = 0; i < argc - 1; i++) {
    // >&
    if(sh->parser->lex[sh->parser->arg0 + i].type == LEX_GT
       && sh->parser->lex[sh->parser->arg0 + i + 1].type == LEX_AMP
       ) {
      stream1 = stream2 = SHELL_STDOUT;
      i0 = i;

      if(i0 && sh->parser->lex[sh->parser->arg0 + i0 - 1].type == LEX_GT) { //>>
        i0 --;
      }

      //1>
      if(i0
         && sh->parser->lex[sh->parser->arg0 + i0 - 1].type == LEX_STR
         && sh->parser->lex[sh->parser->arg0 + i0].end
          - sh->parser->lex[sh->parser->arg0 + i0 - 1].start == 2) {
          switch(*sh->parser->lex[sh->parser->arg0 + i0 - 1].start) {
            case '1':
              i0 --;
              stream1 = SHELL_STDOUT;
              break;
            case '2':
              i0 --;
              stream1 = SHELL_STDERR;
              break;
          }
        } //1>

      //&1
      if(i < argc - 2
         && sh->parser->lex[sh->parser->arg0 + i + 2].type == LEX_STR
         && sh->parser->lex[sh->parser->arg0 + i + 2].end
          - sh->parser->lex[sh->parser->arg0 + i + 1].start == 2
         ) {

        switch(*sh->parser->lex[sh->parser->arg0 + i + 2].start) {
          case '1':
            stream2 = SHELL_STDOUT;
            break;
          case '2':
            stream2 = SHELL_STDERR;
            break;
        }
      } //&1


      if(stream1 != stream2
         && sh->stream.ext_handler != NULL) {

        if(sh->stream.f[stream1] > 0) {
          sh->stream.ext_handler->_close(sh->stream.ext_handler->data, sh->stream.f[stream1]);
          sh->stream.f[stream1] = 0;
        }
        sh->stream.f[stream1] = sh->stream.f[stream2];
      }

      if(argc_new > i0) {
        argc_new = i0;
      }

    } //>&
  }

  argc = argc_new;
  return sh_exec(sh, argc, argv);
}
/*----------------------------------------------------------------------------*/
int sh_exec(C_SHELL *sh, int argc, char **argv)
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

  if(sh_is_debug_mode(sh)) {
    shell_fprintf(sh, SHELL_STDERR, "[%c] [s:%d] [c:%d] [id:%d]", sh_is_true_condition(sh) ? 'x' : ' ', context_stack(sh), sh->cache_stack, sh->op_id);
    for(i = 0; i < argc; i++) {
      shell_fprintf(sh, SHELL_STDERR, " %s", argv[i]);
    }

    if(sh_is_cache_debug_mode(sh)) {
      unsigned allocated = 0, free = 0, count = 0;
      cache_stat(sh->cache, &allocated, &free, &count);
      shell_fprintf(sh, SHELL_STDERR, " cache:a:%u:f:%u:c:%u\n", allocated, free, count);
    }
    else {
      shell_fputc(sh, SHELL_STDERR, '\n');
    }

  }

  do {
    if(argc >= 2
       && sh->parser->argc >= sh->parser->arg0 + 2
       && sh->parser->lex[sh->parser->arg0 + 0].type == LEX_STR
       && sh->parser->lex[sh->parser->arg0 + 1].type == LEX_ASSIGN) {
      r = sh_assign(sh, argc, argv);
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


    r = sh_embed_exec(sh, argc, &argv[argc0]);
    if(r == SHELL_ERR_COMMAND_NOT_FOUND) {
      if(sh_is_true_condition(sh) && sh->exec_cb.cb != NULL) {
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
    sh_set_var(sh, "_", buffer);
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int context_stack(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  int s = 0;

  c = sh_root_context(sh)->child;
  while(c != NULL) {
    s ++;
    c = c->child;
  }
  return s;
}
/*----------------------------------------------------------------------------*/
int sh_push_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  c = sh_current_context(sh);
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
int sh_pop_context(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c, *parent;

  c = sh_current_context(sh);
  parent = c->parent;

  if(parent == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  sh_remove_cache(sh);
  parent->child = NULL;
  cache_free(sh->cache, c);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int add_src_to_cache(C_SHELL *sh, const char *src, unsigned size)
{
  C_SHELL_LINE_CACHE *lc = NULL;
  C_SHELL_CONTEXT *c;

  c = sh_root_context(sh);
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
  if(sh_is_debug_mode(sh) && lc != NULL) {
    shell_fprintf(sh, SHELL_STDERR, "Cashed:*:s:%d:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
static void set_context_should_store_cache(C_SHELL *sh, int should_store)
{
  sh_current_context(sh)->should_store_cache = should_store ? 1 : 0;
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

  if(sh_is_debug_mode(sh) && add) {
    shell_fprintf(sh, SHELL_STDERR, "Cashed:s:%d:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
  }
}
/*----------------------------------------------------------------------------*/
int sh_play_cache(C_SHELL *sh)
{
  int r = SHELL_OK;
  int cache_mode;
  int op_id, stack;
  C_SHELL_CONTEXT *c;
  C_SHELL_LINE_CACHE *lc;

  c = sh_current_context(sh);
  stack = context_stack(sh);
  lc = c->cache.first;
  if(lc == NULL) {
    return SHELL_ERR_INVALID_OP;
  }
  cache_mode = sh_is_cache_mode(sh);
  op_id = sh->op_id;
  sh_set_cache_mode(sh, 1);
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
  sh_set_cache_mode(sh, cache_mode);
  sh->cache_stack --;
  if(!cache_mode) {
    sh->op_id = op_id;
  }

  return r;
}
/*----------------------------------------------------------------------------*/
int sh_remove_cache(C_SHELL *sh)
{
  C_SHELL_CONTEXT *c;
  C_SHELL_LINE_CACHE *lc, *next;

  c = sh_current_context(sh);
  if(c->cache.root != NULL) {
    //cache root
    lc = c->cache.root;
    c->cache.root = NULL;
    while (lc != NULL) {
      next = lc->next;
      if(sh_is_debug_mode(sh)) {
        shell_fprintf(sh, SHELL_STDERR, "Rm cache[%d]:id:%d:'%s'\n", context_stack(sh), lc->op_id, lc->text);
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
void sh_set_cache_mode(C_SHELL *sh, int mode)
{
  sh->cache_mode = mode ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
int sh_is_cache_mode(C_SHELL *sh)
{
  return   sh->cache_mode  ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
void sh_set_opcode(C_SHELL *sh, OP_CODE c)
{
  sh_current_context(sh)->op_code = c;
}
/*----------------------------------------------------------------------------*/
OP_CODE sh_opcode(C_SHELL *sh)
{
  return   sh_current_context(sh)->op_code;
}
/*----------------------------------------------------------------------------*/
