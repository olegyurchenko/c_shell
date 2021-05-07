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
#include "c_sh_embed.h"
#include "c_sh_int.h"
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
#ifndef USE_SH_TEST
static int intern_test(C_SHELL *sh, int argc, char **argv);
#endif //USE_SH_TEST
/*----------------------------------------------------------------------------*/
/**Embeded functions*/
static int sh_exit(C_SHELL *sh, int argc, char **argv);
static int sh_break(C_SHELL *sh, int argc, char **argv);
static int sh_continue(C_SHELL *sh, int argc, char **argv);
static int sh_if(C_SHELL *sh, int argc, char **argv);
static int sh_then(C_SHELL *sh, int argc, char **argv);
static int sh_fi(C_SHELL *sh, int argc, char **argv);
static int sh_else(C_SHELL *sh, int argc, char **argv);
static int sh_elif(C_SHELL *sh, int argc, char **argv);
static int sh_true(C_SHELL *sh, int argc, char **argv);
static int sh_false(C_SHELL *sh, int argc, char **argv);
static int sh_while(C_SHELL *sh, int argc, char **argv);
static int sh_until(C_SHELL *sh, int argc, char **argv);
static int sh_for(C_SHELL *sh, int argc, char **argv);
static int sh_do(C_SHELL *sh, int argc, char **argv);
static int sh_done(C_SHELL *sh, int argc, char **argv);
static int sh_echo(C_SHELL *sh, int argc, char **argv);
static int sh_test(C_SHELL *sh, int argc, char **argv);
static int sh_set(C_SHELL *sh, int argc, char **argv);
static int sh_and(C_SHELL *sh, int argc, char **argv);
static int sh_or(C_SHELL *sh, int argc, char **argv);
static int sh_read(C_SHELL *sh, int argc, char **argv);
static int sh_tea(C_SHELL *sh, int argc, char **argv);
static int sh_seq(C_SHELL *sh, int argc, char **argv);
/*----------------------------------------------------------------------------*/
typedef struct {
  unsigned hash;
  const char *name;
  int (*handler)(C_SHELL *,int,char**);

} STD_FN;

static const STD_FN std_fn[] = {
    {0x7c967e3f, "exit", sh_exit}
  , {0x0f2c9f4a, "break", sh_break}
  , {0x42aefb8a, "continue", sh_continue}
  , {0x00597834, "if", sh_if}
  , {0x7c9e7354, "then", sh_then}
  , {0x005977d4, "fi", sh_fi}
  , {0x7c964c6e, "else", sh_else}
  , {0x7c964b25, "elif", sh_elif}
  , {0x7c9e9fe5, "true", sh_true}
  , {0x0f6bcef0, "false", sh_false}
  , {0x10a3387e, "while", sh_while}
  , {0x10828031, "until", sh_until}
  , {0x0b88738c, "for", sh_for}
  , {0x00597798, "do", sh_do}
  , {0x7c95cc2b, "done", sh_done}
  , {0x7c9624c4, "echo", sh_echo}
  , {0x7c9e6865, "test", sh_test}
  , {0x0002b600, "[", sh_test}
  , {0x0059765b, "[[", sh_test}
  , {0x0b88a991, "set", sh_set}
  , {0x00596f51, "&&", sh_and}
  , {0x00597abd, "||", sh_or}
  , {0x7c9d4d41, "read", sh_read}
  , {0x0b88adbf, "tea", sh_tea}
  , {0x0b88a98e, "seq", sh_seq}
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
, "set"
, "read"
, "tea"
)

comma = " "
print()
for name in names:
  print( '{0} {{ 0x{1:08x}, "{2}", _{3} }}'.format(comma, hash(name), name, name))
  comma=','
----------------------------------------------------------------------------*/
int sh_embed_exec(C_SHELL *sh, int argc, char **argv)
{
  unsigned i, hash;

  if(argc < 1)
    return SHELL_ERR_INVALID_ARG;

  hash = sh_hash(argv[0]);
  for(i = 0; i < sizeof(std_fn)/sizeof(std_fn[0]); i++) {
    if(std_fn[i].hash == hash) {
       return std_fn[i].handler(sh, argc, argv);
    }
  }
  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
int sh_exit(C_SHELL *sh, int argc, char **argv)
{
  int exit_code = 0;

  (void) sh;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
    return 0;
  if(argc > 1)
    exit_code = atoi(argv[1]);

  exit_code %= 1000;

  return SHELL_EXIT - (exit_code > 0 ? exit_code : - exit_code);
}
/*----------------------------------------------------------------------------*/
int sh_if(C_SHELL *sh, int argc, char **argv)
{
  int r1, r2;

  if(argc < 2)
    return SHELL_ERR_INVALID_ARG;

  if(sh_is_true_condition(sh)) {
    sh->parser->arg0 ++; //Shift args
    r1 = sh_exec(sh, argc - 1, &argv[1]);
    if(r1 < 0)
      return r1;
  }
  else
    r1 = 1;

  r2 = sh_push_context(sh);
  if(r2 < 0)
    return r2;

  sh_set_condition(sh, r1);
  sh_set_opcode(sh, OP_IF);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_then(C_SHELL *sh, int argc, char **argv)
{
  if(sh_opcode(sh) != OP_IF) {
    return SHELL_ERR_INVALID_OP;
  }

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    return   sh_exec(sh, argc - 1, &argv[1]);
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_fi(C_SHELL *sh, int argc, char **argv)
{
  (void) argc;
  (void) argv;

  if(sh_opcode(sh) == OP_IF) {
    return sh_pop_context(sh);
  }
  return SHELL_ERR_INVALID_OP;
}
/*----------------------------------------------------------------------------*/
int sh_else(C_SHELL *sh, int argc, char **argv)
{

  (void) argc;
  (void) argv;

  if(sh_opcode(sh) == OP_IF) {
    if(!sh_current_context(sh)->parent->condition) { //Parent condition
      sh_set_condition(sh, !sh_get_condition(sh));
      if(argc > 1) {
        sh->parser->arg0 ++; //Shift args
        return   sh_exec(sh, argc - 1, &argv[1]);
      }
    }
    return SHELL_OK;
  }
  return SHELL_ERR_INVALID_OP;
}
/*----------------------------------------------------------------------------*/
int sh_elif(C_SHELL *sh, int argc, char **argv)
{
  int r;
  r = sh_else(sh, argc, argv);
  if(r < 0)
    return r;
  return sh_if(sh, argc, argv);
}
/*----------------------------------------------------------------------------*/
int sh_true(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
    return 0;
  return 0;
}
/*----------------------------------------------------------------------------*/
int sh_false(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
    return 0;
  return 1;
}
/*----------------------------------------------------------------------------*/
int sh_while(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;

  c = sh_current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = sh_push_context(sh)) < 0)
      return r;

    c = sh_current_context(sh);
    c->op_code = OP_WHILE;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }


  if(sh_is_true_condition(sh))
  {
    r = 0;
    if(argc > 1) {
      sh->parser->arg0 ++; //Shift args
      r = sh_exec(sh, argc - 1, &argv[1]);
      if(r < 0) {
        return r;
      }
    }

    sh_set_condition(sh, r);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_until(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;

  c = sh_current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = sh_push_context(sh)) < 0)
      return r;

    c = sh_current_context(sh);
    c->op_code = OP_WHILE;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }


  if(sh_is_true_condition(sh))
  {
    r = 0;
    if(argc > 1) {
      sh->parser->arg0 ++; //Shift args
      r = sh_exec(sh, argc - 1, &argv[1]);
      if(r < 0) {
        return r;
      }
    }

    sh_set_condition(sh, !r);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_for(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *c;
  int r;
  if(argc < 4
    || argv[2][0] != 'i'
    || argv[2][1] != 'n'
    || argv[2][2] != '\0')
    return SHELL_ERR_INVALID_ARG;

  c = sh_current_context(sh);
  if(sh->op_id != c->op_id)
  {
    //First call
    if((r = sh_push_context(sh)) < 0)
      return r;
    c = sh_current_context(sh);
    c->op_code = OP_IF;
    c->is_loop = 1;
    c->should_store_cache = 1;
  }
  else
  {
    c->for_index ++;
  }


  if(c->for_index < argc - 3 && sh_is_true_condition(sh))
  {
    sh_set_true_condition(sh);
    sh_set_var(sh, argv[1], argv[c->for_index + 3]);
  }
  else
  {
    sh_set_false_condition(sh);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_do(C_SHELL *sh, int argc, char **argv)
{

  if(!sh_current_context(sh)->is_loop) {
    return SHELL_ERR_INVALID_OP;
  }

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    return   sh_exec(sh, argc - 1, &argv[1]);
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_and(C_SHELL *sh, int argc, char **argv)
{
  int cond = NOT_TRUE_CONDITION;
  if(!sh_is_true_condition(sh))
    return NOT_TRUE_CONDITION;

  shell_get_int_var(sh, "_", &cond);

  if(argc > 1 && !cond) {
    sh->parser->arg0 ++; //Shift args
    cond = sh_exec(sh, argc - 1, &argv[1]);
  }
  return cond;
}
/*----------------------------------------------------------------------------*/
int sh_or(C_SHELL *sh, int argc, char **argv)
{
  int cond = NOT_TRUE_CONDITION;
  if(!sh_is_true_condition(sh))
    return NOT_TRUE_CONDITION;

  shell_get_int_var(sh, "_", &cond);

  if(argc > 1) {
    sh->parser->arg0 ++; //Shift args
    cond = sh_exec(sh, argc - 1, &argv[1]);
  }
  return cond;
}
/*----------------------------------------------------------------------------*/
int sh_done(C_SHELL *sh, int argc, char **argv)
{
  int r;
  C_SHELL_CONTEXT *c;
  (void) argc;
  (void) argv;

  c = sh_current_context(sh);

  if(!c->is_loop) {
    return SHELL_ERR_INVALID_OP;
  }

  if(c->is_continue) {
    sh_set_true_condition(sh);
    c->is_continue = 0;
  }

  while(sh_is_true_condition(sh)) {
    r = sh_play_cache(sh);
    if(r < 0)
      return r;

    if(c->is_continue) {
      sh_set_true_condition(sh);
      c->is_continue = 0;
    }
  }

  sh_pop_context(sh);
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_break(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *loop_context, *c;

  (void) argc;
  (void) argv;

  loop_context = sh_current_context(sh);
  while(loop_context != NULL) {
    if(loop_context->is_loop) {
      break;
    }
    loop_context = loop_context->parent;
  }

  if(loop_context == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  if(!sh_is_true_condition(sh))
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
int sh_continue(C_SHELL *sh, int argc, char **argv)
{
  C_SHELL_CONTEXT *loop_context, *c;

  (void) argc;
  (void) argv;

  loop_context = sh_current_context(sh);
  while(loop_context != NULL) {
    if(loop_context->is_loop) {
      break;
    }
    loop_context = loop_context->parent;
  }

  if(loop_context == NULL) {
    return SHELL_ERR_INVALID_OP;
  }

  if(!sh_is_true_condition(sh))
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
int sh_echo(C_SHELL *sh, int argc, char **argv)
{
  int i = 1, endl=1, first = 1;
  const char *p;

  (void) sh;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
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
int sh_test(C_SHELL *sh, int argc, char **argv)
{
  (void) sh;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
    return 0;

#ifdef USE_SH_TEST
  return test_main(argc, argv);
#else //USE_SH_TEST
  return intern_test(sh, argc, argv);
#endif //USE_SH_TEST

  return SHELL_ERR_NOT_IMPLEMENT;
}
/*----------------------------------------------------------------------------*/
int sh_set(C_SHELL *sh, int argc, char **argv)
{
  C_SHEL_VAR *v;
  unsigned allocated=0, free=0, count=0;
  (void) argc;
  (void) argv;
  if(!sh_is_true_condition(sh))
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
int sh_assign(C_SHELL *sh, int argc, char **argv)
{
  if(!sh_is_true_condition(sh) || argc < 2)
    return TRUE_CONDITION;

  //TODO: addition substitute vars
  shell_set_var(sh, argv[0], argv[2]);

  if(argc > 3) {
    sh->parser->arg0 += 3; //Shift args
    return   sh_exec(sh, argc - 3, &argv[3]);
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_read(C_SHELL *sh, int argc, char **argv)
{
  char *buffer, *p0, *p1;
  unsigned buffer_size = 16 * 1024;
  int ret = SHELL_OK, r, eof, i = 0;
  const char *IFS;
  const char blanks[] = " \t\r\n";

  buffer = (char *) cache_alloc(sh->cache, buffer_size);
  if(buffer == NULL) {
    return SHELL_ERR_MALLOC;
  }

  do {

    while (i < (int) (buffer_size - 1)) {
      r = shell_read(sh, &buffer[i], 1);
      if(r == 0 || buffer[i] == '\n') {
        break;
      }
      if(r < 0) {
        ret = r;
        break;
      }
      i ++;
    }

    if(ret < 0) {
      break;
    }

    buffer[i] = 0;
    p0 = buffer;
    IFS = sh_get_var(sh, "IFS");
    if(!*IFS) {
      IFS = blanks;
    }
    for(i = 1; i < argc; i++) {
      p1 = p0;
      while (*p1 && !strchr(IFS, *p1)) {
        p1 ++;
      }
      eof = *p1 == 0;
      *p1 = 0;
      if((r = sh_set_var(sh, argv[i], p0)) < 0) {
        ret = r;
        break;
      }
      if(eof) {
        break;
      }
      p0 = p1 + 1;
      while (*p0 && strchr(IFS, *p0)) {
        p0 ++;
      }
      eof = *p0 == 0;
      if(eof) {
        break;
      }
    }

    if(!eof) {
      if((r = sh_set_var(sh, "REPLY", p0)) < 0) {
        ret = r;
        break;
      }
    } else {
      if((r = sh_set_var(sh, "REPLY", "")) < 0) {
        ret = r;
        break;
      }
      for(i++ ; i < argc; i++) {
        if((r = sh_set_var(sh, argv[i], "")) < 0) {
          ret = r;
          break;
        }
      }
    }
  } while(0);
  cache_free(sh->cache, buffer);
  return ret;
}
/*----------------------------------------------------------------------------*/
static int sh_tea(C_SHELL *sh, int argc, char **argv)
{
  int i, count = 0, ret = SHELL_OK, r, size;
  SHELL_STREAM_MODE mode = SHELL_OUT;

  typedef struct {
    char buffer[16*1024];
    int fd[16];
  } sh_tea_t;

  sh_tea_t *data;
  if(sh->stream.ext_handler == NULL) {
    return SHELL_ERR_NOT_IMPLEMENT;
  }

  data = (sh_tea_t *) cache_alloc(sh->cache, sizeof(sh_tea_t));
  if(data == NULL) {
    return SHELL_ERR_MALLOC;
  }
  memset(data, 0, sizeof(sh_tea_t));

  do {

    for(i = 1; i < argc && count < (int) (sizeof(data->fd)/ sizeof(data->fd[0])); i++) {
      if(!strcmp(argv[i], "-a") || !strcmp(argv[i], "--append")) {
        mode = SHELL_APPEND;
        continue;
      }

      data->fd[count] = sh->stream.ext_handler->_open(sh->stream.ext_handler->data, argv[i], mode);
      if(data->fd[count] <= 0) {
        ret = SHELL_ERROR_OPEN_FILE;
        break;
      }
      count ++;
    }

    while(1) {
      r = shell_read(sh, data->buffer, sizeof(data->buffer));
      if(r < 0) {
        ret = r;
        break;
      }
      if(!r) {
        break;
      }

      size = r;
      for(i = 0; i < count; i++) {
        if(data->fd[i] > 0) {
          if((r = sh->stream.ext_handler->_write(sh->stream.ext_handler->data, data->fd[i], data->buffer, size)) < 0) {
            ret = r;
            break;
          }
        }
      }

      if((r = shell_write(sh, SHELL_STDOUT, data->buffer, size)) < 0) {
        ret = r;
        break;
      }
    }

  } while(0);
  for(i = 0; i < count; i++) {
    if(data->fd[i] > 0) {
      sh->stream.ext_handler->_close(sh->stream.ext_handler->data, data->fd[i]);
    }
  }
  cache_free(sh->cache, data);
  return ret;
}
/*----------------------------------------------------------------------------*/
static int sh_seq(C_SHELL *sh, int argc, char **argv)
{
  int ret = SHELL_OK, r;
  //All numbers can be integers, not reals.
  int i, first = 1, step = 1, last, count = 0;
  int values[3];

  for(i = 1; i < argc; i++) {
    if(count < 3) {
      values[count ++] = atoi(argv[i]);
    }
  }

  switch (count) {
    case 1:
      last = values[0];
      break;
    case 2:
      first = values[0];
      last = values[1];
      break;
    case 3:
      first = values[0];
      step = values[1];
      last = values[2];
      break;
    default:
      shell_fprintf(sh, SHELL_STDERR, "Invalid argument count\n");
      return 1;
  }

  if( (first < last && step > 0)
      || (first > last && step < 0)) {
    for(i = first; i <= last; i += step) {
      if((r = shell_printf(sh, "%d\n", i)) <= 0) {
        ret = r;
        break;
      }
    }
  }

  return ret;
}
/*----------------------------------------------------------------------------*/
int sh_stream_open(void *data, const char* name, SHELL_STREAM_MODE mode)
{
  int fd = 1;
  C_SHELL *sh;
  C_SHELL_INTERN_STREAM *stream;
  unsigned h = 0;

  sh = (C_SHELL *) data;

  if(mode != SHELL_FIFO) {
    h = sh_hash(name);
  }

  stream = sh->intern_stream;
  while (stream != NULL) {
    if(fd <= stream->fd) {
      fd = stream->fd + 1;
    }

    if(mode != SHELL_FIFO && h == stream->hash) {
       break;
    }

    stream = stream->next;
  }

  if(stream == NULL) {
    stream = (C_SHELL_INTERN_STREAM *) cache_alloc(sh->cache, sizeof(C_SHELL_INTERN_STREAM));
    if(stream == NULL) {
      return SHELL_ERR_MALLOC;
    }
    memset(stream, 0, sizeof(C_SHELL_INTERN_STREAM));
    stream->fd = fd;
    stream->hash = h;
    if(name != NULL && mode != SHELL_FIFO) {
      strncpy(stream->name, name, sizeof(stream->name));
      stream->name[sizeof(stream->name) - 1] = 0;
    }
    stream->next = sh->intern_stream;
    sh->intern_stream = stream;
  }

  if(mode == SHELL_OUT) {
    stream->size = 0;
  }
  stream->position = 0;
  return stream->fd;
}
/*----------------------------------------------------------------------------*/
int sh_stream_close(void *data, int f)
{
  C_SHELL *sh;
  C_SHELL_INTERN_STREAM *stream, *prev = NULL;

  sh = (C_SHELL *) data;
  stream = sh->intern_stream;
  while (stream != NULL) {
    if(f == stream->fd) {
      if(!*stream->name) {
        //FIFO
        if(prev != NULL) {
          prev->next = stream->next;
        } else {
          sh->intern_stream = stream->next;
        }
        if(stream->buffer != NULL) {
          cache_free(sh->cache, stream->buffer);
        }
        cache_free(sh->cache, stream);
      }
      break;
    }


    prev = stream;
    stream = stream->next;
  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
int sh_stream_read(void *data, int f, void* buf, unsigned size)
{
  C_SHELL *sh;
  C_SHELL_INTERN_STREAM *stream;
  char *dst;
  int r = 0;

  sh = (C_SHELL *) data;
  stream = sh->intern_stream;
  dst = (char *)buf;
  while (stream != NULL) {
    if(f == stream->fd) {
      break;
    }
    stream = stream->next;
  }
  if(stream == NULL) {
    return SHELL_ERROR_OPEN_FILE;
  }

  while(size && stream->position < stream->size) {
    dst[r] = stream->buffer[stream->position];
    size --;
    stream->position ++;
    r ++;
  }
  return r;
}
/*----------------------------------------------------------------------------*/
int sh_stream_write(void *data, int f, const void* buf, unsigned size)
{
  C_SHELL *sh;
  C_SHELL_INTERN_STREAM *stream;
  const char *src;
  char *buffer;
  int r = 0;
  unsigned allocated;

  sh = (C_SHELL *) data;
  stream = sh->intern_stream;
  src = (const char *)buf;
  while (stream != NULL) {
    if(f == stream->fd) {
      break;
    }
    stream = stream->next;
  }
  if(stream == NULL) {
    return SHELL_ERROR_OPEN_FILE;
  }

  while(size) {
    if(stream->size >= stream->allocated || stream->buffer == NULL) {
      allocated = stream->allocated + (size < 1024 ? 1024 : size);
      if(stream->buffer == NULL) {
        buffer = (char *) cache_alloc(sh->cache, allocated);
      } else {
        buffer = (char *) cache_realloc(sh->cache, stream->buffer, allocated);
      }
      if(buffer == NULL) {
        return SHELL_ERR_MALLOC;
      }
      stream->allocated = allocated;
      stream->buffer = buffer;
    }
    stream->buffer[stream->size] = src[r];
    stream->size ++;
    size --;
    r ++;
  }

  return r;
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
  r += shell_fputc(data->sh, SHELL_STDERR, '\n');
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
  if(!sh_is_debug_mode(data->sh))
    return;
  shell_fprintf(data->sh, SHELL_STDERR, "<<op:%s:", op == NULL ? "" : op);
  shell_fprintf(data->sh, SHELL_STDERR, "arg0:%d:argc:%d", data->arg0, data->argc);

  for(i = data->arg0; i < data->argc; i++) {
    shell_fprintf(data->sh, SHELL_STDERR, ":%s", data->argv[i]);
  }
  shell_fprintf(data->sh, SHELL_STDERR, ">>\n");
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

