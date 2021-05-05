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

#include <unistd.h>
#include <sys/stat.h>
 #include <fcntl.h>
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
  , "read"
//  , "test_1" //TEST
//  , "test_2" //TEST
  , "rr" //test
};
/*----------------------------------------------------------------------------*/
typedef struct {
  int mode;
  int fd[2];
} stream_t;
/*----------------------------------------------------------------------------*/
#define MAX_STREAM_COUNT 4 //For one shell may be opened 3: cmd < STDIN > STDOUT 2> STDERR
/*----------------------------------------------------------------------------*/
typedef struct {
  SHELL_STREAM_HANDLER handler;
  stream_t streams[MAX_STREAM_COUNT];
} stream_handler_t;

static stream_handler_t stream_handler;
static int _open(void *data, const char* name, SHELL_STREAM_MODE mode);
static int _close(void *data, int f);
static int _read(void *data, int f, void* buf, unsigned size);
static int _write(void *data, int f, const void* buf, unsigned size);
/*----------------------------------------------------------------------------*/
void ts_init(TEST_SHELL_DATA *data)
{
  int i;
  memset(&stream_handler, 0, sizeof(stream_handler_t));
  stream_handler.handler.data = &stream_handler;
  stream_handler.handler._open = _open;
  stream_handler.handler._close = _close;
  stream_handler.handler._read = _read;
  stream_handler.handler._write = _write;
  //shell_set_stream_handler(data->sh, &stream_handler.handler);
  for(i = 0; i < MAX_STREAM_COUNT; i++) {
    stream_handler.streams[i].mode = -1;
  }
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
  if(!strcmp(argv[0], "rr")) {
    return cmd_read(arg, argc, argv);
  }
  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
static int cmd_read(void *arg, int argc, char **argv)
{
  TEST_SHELL_DATA *data;
  int i, r = SHELL_OK, c;

  data = (TEST_SHELL_DATA *) arg;

  //To check stderr
  for(i = 1; i < argc; i++) {
    shell_fprintf(data->sh, SHELL_STDERR, "Invalid option '%s'\n", argv[i]);
  }

  while(1) {
    c  = 0;
    r = shell_read(data->sh, &c, 1);
    if(r <= 0) {
      break;
    }
    r = shell_putc(data->sh, c);
    if(r <= 0) {
      break;
    }
  }

  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _open(void *data, const char* name, SHELL_STREAM_MODE mode)
{
  stream_handler_t *handler;
  int i, h, r = SHELL_OK;

  handler = (stream_handler_t *) data;

  for(i = 0; i < MAX_STREAM_COUNT; i++) {
    if(handler->streams[i].mode == -1) {
      break;
    }
  }

  if(i == MAX_STREAM_COUNT) {
    return SHELL_ERR_MALLOC;
  }

  //Read end
  handler->streams[i].fd[0] = 0;
  //Write end
  handler->streams[i].fd[1] = 0;
  switch (mode) {
    case SHELL_IN:
      h = open(name, O_RDONLY);
      if(h < 0) {
        r = SHELL_ERROR_OPEN_FILE;
        break;
      }
      //Read end
      handler->streams[i].fd[0] = h;
      break;
    case SHELL_OUT:
      h = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if(h < 0) {
        r = SHELL_ERROR_OPEN_FILE;
        break;
      }
      //Write end
      handler->streams[i].fd[1] = h;
      break;
    case SHELL_APPEND:
      h = open(name, O_WRONLY | O_CREAT, 0666);
      if(h < 0) {
        r = SHELL_ERROR_OPEN_FILE;
        break;
      }
      //Write end
      handler->streams[i].fd[1] = h;
      //Seek to end
      lseek(h, 0, SEEK_END);
      break;
    case SHELL_FIFO:
      //Read / write end
      if((h = pipe(handler->streams[i].fd)) < 0) {
        r = SHELL_ERROR_OPEN_FILE;
      }
      break;
    default:
      r = SHELL_ERR_INVALID_ARG;
      break;
  }

  if(SHELL_OK == r) {
    handler->streams[i].mode = mode;
    r = i + 1;
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int _close(void *data, int f)
{
  stream_handler_t *handler;
  int i;

  handler = (stream_handler_t *) data;
  if(f <= 0 || f > MAX_STREAM_COUNT) {
    return SHELL_ERR_INVALID_ARG;
  }
  i = f - 1;
  if(handler->streams[i].fd[0]) {
    //Close read end
    close(handler->streams[i].fd[0]);
    handler->streams[i].fd[0] = 0;
  }
  if(handler->streams[i].fd[1]) {
    //Close write end
    close(handler->streams[i].fd[1]);
    handler->streams[i].fd[1] = 0;
  }
  handler->streams[i].mode = -1;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int _read(void *data, int f, void* buf, unsigned size)
{
  stream_handler_t *handler;
  int i, r = 0;

  handler = (stream_handler_t *) data;
  if(f <= 0 || f > MAX_STREAM_COUNT) {
    return SHELL_ERR_INVALID_ARG;
  }
  i = f - 1;


  if(handler->streams[i].fd[1]) {
    //Close unused write end
    close(handler->streams[i].fd[1]);
    handler->streams[i].fd[1] = 0;
  }

  if(handler->streams[i].fd[0]) {
    r = read(handler->streams[i].fd[0], buf, size);
  }
  return r;
}
/*----------------------------------------------------------------------------*/
static int _write(void *data, int f, const void* buf, unsigned size)
{
  stream_handler_t *handler;
  int i, r = 0;

  handler = (stream_handler_t *) data;
  if(f <= 0 || f > MAX_STREAM_COUNT) {
    return SHELL_ERR_INVALID_ARG;
  }
  i = f - 1;
  if(handler->streams[i].fd[1]) {
    r = write(handler->streams[i].fd[1], buf, size);
  }
  return r;
}
/*----------------------------------------------------------------------------*/
