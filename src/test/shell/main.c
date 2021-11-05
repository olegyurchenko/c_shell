#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_shell.h"

#define CTRL_C 3  // ^C
#define CTRL_C_EXIT -200


static int terminated = 0;


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
  int r;
  TEST_SHELL_DATA *ts;

  (void) size;
  (void) arg;

  ts = (TEST_SHELL_DATA *)arg;

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
  TEST_SHELL_DATA *data;

  data = (TEST_SHELL_DATA *)arg;
  if(shell_stack_size(data->sh)) {
    snprintf(buffer, buffer_size, "> ");
  }
  else {
    snprintf(buffer, buffer_size, "shell$ ");
  }
  return 0;
}


int ctrl_c_cb(void *arg, const char *text, unsigned size)
{
  TEST_SHELL_DATA *ts;

  (void) text;
  ts = (TEST_SHELL_DATA *)arg;
  if(!size) {
    shell_printf(ts->sh, "<reset>\n");
    shell_reset(ts->sh);
  }
  return 0;
}

int step_cb(void *arg, int argc, char **argv)
{
  TEST_SHELL_DATA *ts;
  int c;

  (void) argc;
  (void) argv;

  ts = (TEST_SHELL_DATA *) arg;
  (void) ts;

  if(console_kbhit()) {
    c = console_getchar();
    if(c == CTRL_C)
    {
      return CTRL_C_EXIT;
    }
  }

  return SHELL_OK;
}

static int exec_file(void *arg, int argc, char **argv)
{
  int i;
  int ret = SHELL_OK;
  TEST_SHELL_DATA *data;

  data = (TEST_SHELL_DATA *) arg;

  if(argc < 1) {
    return SHELL_ERR_INVALID_ARG;
  }



    FILE *f;

    int line = 0, debug = 0;
    char buffer[1024];

    f = fopen(argv[i], "r");
    if(f == NULL)
    {
      shell_printf(data->sh, "Error open file '%s'\n", argv[i]);
      return SHELL_ERR_INVALID_ARG;
    }

    for(i = 0; i < argc - 1; i++) {
      snprintf(buffer, sizeof(buffer), "$%d", i);
      shell_set_var(data->sh, buffer, argv[i + 1]);
    }

    ret = 0;
    while (!feof(f)) {
      line ++;
      if(NULL == fgets(buffer, sizeof(buffer), f)) {
        ret = 0;
        break;
      }
      if(SHELL_OK == shell_get_int_var(data->sh, SHELL_DEBUG_VAR_NAME, &debug) && debug) {
        shell_printf(data->sh, "%s:%d:%s\n", argv[i], line, buffer);
      }
      ret = shell_rx(data->sh, buffer);
      if(ret < 0) {
        shell_printf(data->sh, "%s:%d:%s\n", argv[i], line, shell_err_string(data->sh, ret));
        break;
      }
    }

    fclose(f);
    return ret;
}

int shell_exec(void *arg, int argc, char **argv)
{

  if(argc >= 1) {
    if(!strcmp(argv[0], ".")) {
      return exec_file(arg, argc, argv);
    }
  }


  return ts_exec(arg, argc, argv);
}

int main(int argc, char ** argv)
{
  TEST_SHELL_DATA data;



  data.tty = tty_alloc(TTY_DEFAULT_INPUT_SIZE, TTY_DEFAULT_HISTORE_SIZE, TTY_DEFAULT_PROMPT_SIZE);
  tty_set_echo_mode(data.tty, 1);
  tty_set_prompt_cb(data.tty, prompt_cb, &data);
  tty_set_print_cb(data.tty, print_cb, data.tty);
  tty_set_ctrlc_c_cb(data.tty, ctrl_c_cb, &data);
  tty_set_kwords_cb(data.tty, ts_keyword, &data);


  data.sh = shell_alloc();
  shell_set_print_cb(data.sh, print_cb, data.tty);
  shell_set_exec_cb(data.sh, shell_exec, &data);
  shell_set_step_cb(data.sh, step_cb, &data);

  ts_init(&data);

  if(argc > 1) {
    return exec_file(&data, argc - 1, &argv[1]);
  }

  if (!console_isatty()) {
    fprintf(stderr, "%s must run on a tty\n", argv[0]);
    return 1;
  }
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



