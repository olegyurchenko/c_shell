#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_tty.h"
#include "c_sh.h"

#define CTRL_C 3  // ^C
#define CTRL_C_EXIT -200

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
};

static int terminated = 0;

typedef struct  {
  TTY *tty;
  C_SHELL *sh;
} TTY_SHELL;

int print_cb(void *arg, int c)
{
  (void) arg;
  if (c == '\n') {
    fputc('\r', stdout);
  }
  fputc(c, stdout);
  return 1;
}


int enter_cb(void *arg, const char *text, unsigned size)
{
  unsigned i;
  int r;
  TTY_SHELL *ts;

  (void) size;
  (void) arg;

  ts = (TTY_SHELL *)arg;

  r = shell_rx(ts->sh, text);
  if(r < 0) {
    if( r != CTRL_C_EXIT)
      shell_printf(ts->sh, ">> %s <<\n", shell_err_string(ts->sh, r));

    if(shell_stack_size(ts->sh)
       || r == SHELL_ERR_MALLOC)
      shell_reset(ts->sh);
  }

  if(r <= SHELL_EXIT) {
    terminated = 1;
  }

  return (int) size;
}

int prompt_cb(void *arg, char *buffer, unsigned buffer_size) {
  TTY_SHELL *data;

  data = (TTY_SHELL *)arg;
  if(shell_stack_size(data->sh)) {
    snprintf(buffer, buffer_size, "> ");
  }
  else {
    snprintf(buffer, buffer_size, "shell$ ");
  }
}


int ctrl_c_cb(void *arg, const char *text, unsigned size)
{
  TTY_SHELL *ts;

  (void) text;
  ts = (TTY_SHELL *)arg;
  if(!size) {
    shell_printf(ts->sh, "<reset>\n");
    shell_reset(ts->sh);
  }
  return 0;
}


int step_cb(void *arg, int argc, char **argv)
{
  TTY_SHELL *ts;
  int c;

  (void) argc;
  (void) argv;

  ts = (TTY_SHELL *) arg;

  if(console_kbhit()) {
    c = console_getchar();
    if(c == CTRL_C)
    {
      return CTRL_C_EXIT;
    }
  }

  return SHELL_OK;
}

static int exec_files(C_SHELL *sh, int argc, char **argv)
{
  int i;
  int ret = SHELL_OK;

  for(i = 1; i < argc; i++)
  {
    FILE *f;

    int line = 0, debug = 0;
    char buffer[1024];

    f = fopen(argv[i], "r");
    if(f == NULL)
    {
      shell_printf(sh, "Error open file '%s'\n", argv[i]);
      return SHELL_ERR_INVALID_ARG;
    }

    ret = 0;
    while (!feof(f)) {
      line ++;
      if(NULL == fgets(buffer, sizeof(buffer), f)) {
        ret = 0;
        break;
      }
      if(SHELL_OK == shell_get_int_var(sh, SHELL_DEBUG_VAR_NAME, &debug) && debug) {
        shell_printf(sh, "%s:%d:%s\n", argv[i], line, buffer);
      }
      ret = shell_rx(sh, buffer);
      if(ret < 0) {
        shell_printf(sh, "%s:%d:%s\n", argv[i], line, shell_err_string(sh, ret));
        break;
      }
    }

    fclose(f);
    if(ret < 0)
      break;
  }
  return ret;
}

int shell_exec(void *arg, int argc, char **argv)
{
  C_SHELL *sh;
  unsigned size;
  sh = (C_SHELL *)arg;
  int found = 0;

  if(argc >= 1) {
    size = strlen(argv[0]);

    if(!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit")) {
      terminated = 1;
      found = 1;
    }

    if(!strcmp(argv[0], ".")) {
      return exec_files(sh, argc, argv);
    }

  }


  return found ? SHELL_OK : SHELL_ERR_COMMAND_NOT_FOUND;
}

int main(int argc, char ** argv)
{
  int r;
  TTY_SHELL data;

  (void) argc;
  (void) argv;


  if (!console_isatty()) {
    fprintf(stderr, "%s must run on a tty\n", argv[0]);
    return 1;
  }

  data.tty = tty_alloc(TTY_DEFAULT_INPUT_SIZE, TTY_DEFAULT_HISTORE_SIZE, TTY_DEFAULT_PROMPT_SIZE);
  tty_set_echo_mode(data.tty, 1);
  tty_set_prompt_cb(data.tty, prompt_cb, &data);
  tty_set_print_cb(data.tty, print_cb, data.tty);
  tty_set_ctrlc_c_cb(data.tty, ctrl_c_cb, &data);
  tty_set_keywords(data.tty, shell_commands, sizeof(shell_commands)/sizeof(shell_commands[0]));


  data.sh = shell_alloc();
  shell_set_print_cb(data.sh, print_cb, data.tty);
  shell_set_exec_cb(data.sh, shell_exec, data.sh);
  shell_set_step_cb(data.sh, step_cb, &data);

  setup_console();

  tty_printf(data.tty, "%s\n", "Use 'quit' or 'exit' to quit.");
  tty_rx(data.tty, '\n');
  tty_set_enter_cb(data.tty, enter_cb, &data);

  while (!terminated) {
    int c = console_getchar();
    if (c > 0 && c <= 255) {
      if (c == '\r') {
        c = '\n';
      }
      tty_rx(data.tty, c);
    }
  }
  restore_console();
  return 0;
}



