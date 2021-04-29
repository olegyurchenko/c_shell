/*----------------------------------------------------------------------------*/
/**
* @pkg test_shell
*/
/**
* Test shell functions.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2021.<br>
* started 29.04.2021  9:30:03<br>
* @pkgdoc test_shell
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_shell.h"
/*----------------------------------------------------------------------------*/
static const char *shell_commands[] = {
    "exit"
  , "quit"
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
//  , "test_1" //TEST
//  , "test_2" //TEST
  , "read" //test
};
/*----------------------------------------------------------------------------*/
typedef struct {
  SHELL_STREAM_HANDLER handler;
} stream_handler_t;

static stream_handler_t stream_handler;
static int _open(void *data, const char* name, SHELL_STREAM_MODE mode);
static int _close(void *data, int f);
static int _read(void *data, int f, void* buf, unsigned size);
static int _write(void *data, int f, const void* buf, unsigned size);
/*----------------------------------------------------------------------------*/
void ts_init(TEST_SHELL_DATA *data)
{
  memset(&stream_handler, 0, sizeof(stream_handler_t));
  stream_handler.handler.data = &stream_handler;
  stream_handler.handler._open = _open;
  stream_handler.handler._close = _close;
  stream_handler.handler._read = _read;
  stream_handler.handler._write = _write;
  shell_set_stream_handler(data->sh, &stream_handler.handler);
}
/*----------------------------------------------------------------------------*/
const char* ts_keyword(void *arg, unsigned index)
{
  (void) arg;

  if(index < sizeof (shell_commands) / sizeof (shell_commands[0])) {
    return shell_commands[index];
  }
  return NULL;
}
/*----------------------------------------------------------------------------*/
static int cmd_read(void *arg, int argc, char **argv);
/*----------------------------------------------------------------------------*/
int ts_exec(void *arg, int argc, char **argv)
{
  if(!strcmp(argv[0], "read")) {
    return cmd_read(arg, argc, argv);
  }
  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
static int cmd_read(void *arg, int argc, char **argv)
{
  TEST_SHELL_DATA *data;

  data = (TEST_SHELL_DATA *) arg;
  (void) argc;
  (void) argv;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _open(void *data, const char* name, SHELL_STREAM_MODE mode)
{
}
/*----------------------------------------------------------------------------*/
static int _close(void *data, int f)
{
}
/*----------------------------------------------------------------------------*/
static int _read(void *data, int f, void* buf, unsigned size)
{
}
/*----------------------------------------------------------------------------*/
static int _write(void *data, int f, const void* buf, unsigned size)
{
}
/*----------------------------------------------------------------------------*/
