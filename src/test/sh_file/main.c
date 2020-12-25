#include <stdio.h>
#include <string.h>
#include "c_sh.h"
/*----------------------------------------------------------------------------*/
static int exec_file(int argc, char **argv);
/*----------------------------------------------------------------------------*/
int print_cb(void *arg, int c)
{
  (void) arg;
  if (c == '\n') {
    fputc('\r', stdout);
  }
  fputc(c, stdout);
  return 1;
}
/*----------------------------------------------------------------------------*/
int shell_exec(void *arg, int argc, char **argv)
{
  (void) arg;

  if(argc >= 1) {
    if(!strcmp(argv[0], ".")) {
      return exec_file(argc, argv);
    }
  }

  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
  return exec_file(argc, argv);
}
/*----------------------------------------------------------------------------*/
static int exec_file(int argc, char **argv)
{
  int i;
  int ret;

  for(i = 1; i < argc; i++)
  {
    FILE *f;
    C_SHELL *sh;
    int line = 0, debug = 0;
    char buffer[1024];

    f = fopen(argv[i], "r");
    if(f == NULL)
    {
      fprintf(stderr, "Error open file '%s'\n", argv[i]);
      return 2;
    }

    sh = shell_alloc();
    shell_set_print_cb(sh, print_cb, sh);
    shell_set_exec_cb(sh, shell_exec, sh);
    //shell_set_int_var(sh, LEX_DEBUG_VAR_NAME, 1);
    //shell_set_int_var(sh, PARS_DEBUG_VAR_NAME, 1);
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
        fprintf(stderr, "%s:%d:%s\n", argv[i], line, shell_err_string(sh, ret));
        break;
      }
    }

    if(ret >= 0) {
      if(shell_stack_size(sh)) {
        ret = SHELL_STACK_ERROR;
      }
    }

    shell_free(sh);
    fclose(f);
    if(ret < 0)
      break;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
