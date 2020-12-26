/*----------------------------------------------------------------------------*/
/**
* Demo shell functions.
*/
/*----------------------------------------------------------------------------*/
#include "shell.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
extern const SHELL_FUNCTION  _date, _help, _passwd, _quit, _useradd, _userdel, _userlist;
static const SHELL_FUNCTION *shell_functions[] = {
  &_help
//, &_date
//, &_passwd
, &_quit
//, &_useradd
//, &_userdel
//, &_userlist
};
/*----------------------------------------------------------------------------*/
static const char *shell_commands[] = {
    "exit"
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
  , "set"
};

#define PASSWORD_ECHO_MODE 0
static int help_cb(SHELL_DATA *sh, int argc, char **argv);
static int exit_cb(SHELL_DATA *sh, int argc, char **argv);

const SHELL_FUNCTION _help = {
  help_cb,
  "help",
  "This help",
  "Used: help [command]\n"
  "Get help for command, or list all commands (if command not enter)"
};

const SHELL_FUNCTION _quit = {
  exit_cb,
  "quit",
  "Exit from shell",
  "Used: quit (without parameters) to exit"
};

/*----------------------------------------------------------------------------*/
static const char *kword_cb(void *arg, unsigned index)
{
  (void) arg;
  if(index < sizeof (shell_commands) / sizeof (shell_commands[0]))
    return shell_commands[index];
  index -= sizeof (shell_commands) / sizeof (shell_commands[0]);
  if(index < sizeof(shell_functions)/sizeof(shell_functions[0]))
    return shell_functions[index]->name;
  return NULL;
}
/*----------------------------------------------------------------------------*/
static void set_login_mode(SHELL_DATA *data)
{
  data->mode = LOGIN_MODE;
  tty_set_echo_mode(data->tty, 1);
  tty_set_password_mode(data->tty, 0);
  tty_set_histore_mode(data->tty, 0);
  tty_set_kwords_cb(data->tty, NULL, 0);
}
/*----------------------------------------------------------------------------*/
static void set_password_mode(SHELL_DATA *data)
{
  data->mode = PASSWORD_MODE;
  tty_set_echo_mode(data->tty, PASSWORD_ECHO_MODE);
  tty_set_password_mode(data->tty, 1);
  tty_set_histore_mode(data->tty, 0);
  tty_set_kwords_cb(data->tty, NULL, 0);
}
/*----------------------------------------------------------------------------*/
static void set_shell_mode(SHELL_DATA *data)
{
  data->mode = SHELL_MODE;
  tty_set_echo_mode(data->tty, 1);
  tty_set_password_mode(data->tty, 0);
  tty_set_histore_mode(data->tty, 1);
  tty_set_kwords_cb(data->tty, kword_cb, data);
  tty_printf(data->tty, "%s\n", "Use 'help' to get help.");
}
/*----------------------------------------------------------------------------*/
static int ctrl_c_cb(void *arg, const char *text, unsigned size)
{
  SHELL_DATA *data;

  (void) text;
  data = (SHELL_DATA *)arg;

  if(data->mode == LOGIN_MODE) {
      data->terminated = 1;
  }

  if(data->mode == PASSWORD_MODE) {
      data->terminated = 1;
    set_login_mode(data);
  }


  if(!size) {
    shell_printf(data->sh, "<reset>\n");
    shell_reset(data->sh);
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int enter_cb(void *arg, const char *text, unsigned size)
{
  int r;
  SHELL_DATA *data;

  data = (SHELL_DATA *) arg;

  if(data->mode == LOGIN_MODE) {
    if(size && size < sizeof(data->user)) {
      memcpy(data->user, text, size);
      data->user[size] = 0;
#if 0
      if(is_password_set(data->shell.user)) {
        set_password_mode(data);
      } else {
        set_shell_mode(data);
      }
#else
      set_shell_mode(data);
#endif
      return 1;
    }
    return 0;
  }

  if(data->mode == PASSWORD_MODE) {
    //Check user & password
    if(1/*FM_OK == shell_login(data->shell.user, text)*/) {
      //Password Ok
      set_shell_mode(data);
      return 1;

    } else {
      tty_printf(data->tty, "Login incorrect\n");
      set_login_mode(data);
      return -1;
    }
  }


  r = shell_rx(data->sh, text);
  if(r < 0) {
      shell_printf(data->sh, ">> %s <<\n", shell_err_string(data->sh, r));

    if(shell_stack_size(data->sh)
       || r == SHELL_ERR_MALLOC)
      shell_reset(data->sh);
  }

  if(r <= SHELL_EXIT) {
    data->terminated = 1;
  }

  return (int) size;
}
/*----------------------------------------------------------------------------*/
static int prompt_cb(void *arg, char *buffer, unsigned buffer_size) {
  SHELL_DATA *data;

  data = (SHELL_DATA *)arg;

  if(data->mode == LOGIN_MODE) {
    return snprintf(buffer, buffer_size, "Login: ");
  }

  if(data->mode == PASSWORD_MODE) {
    return snprintf(buffer, buffer_size, "Enter %s password: ", data->user);
  }

  if(shell_stack_size(data->sh)) {
    return snprintf(buffer, buffer_size, "> ");
  }
  else {
    return snprintf(buffer, buffer_size, "shell$ ");
  }
}
/*----------------------------------------------------------------------------*/
static int exit_cb(SHELL_DATA *data, int argc, char **argv)
{
  (void) data;
  (void) argc;
  (void) argv;

  return SHELL_EXIT;
}
/*----------------------------------------------------------------------------*/
static int help_cb(SHELL_DATA *sh, int argc, char **argv)
{
  unsigned i;
  const char *cmd = NULL;

  if(strcmp(argv[0], "help")) { //Not help command but command with --helo option
    cmd = argv[0];
  }

  for(i = 1; cmd == NULL && i < (unsigned) argc; i++) {
    if(argv[i][0] != '-') {
      cmd = argv[i];
    }
  }

  if(cmd == NULL) {
    shell_printf(sh->sh, "Known commands:\n");
    for(i = 0; i < sizeof(shell_functions)/sizeof(shell_functions[0]); i++) {
      shell_printf(sh->sh, "%s - %s\n", shell_functions[i]->name, shell_functions[i]->description);
    }

    shell_printf(sh->sh, "For get help for command - use help <command>\n");
  } else {
    for(i = 0; i < sizeof(shell_functions)/sizeof(shell_functions[0]); i++) {
      if(!strcmp(cmd, shell_functions[i]->name)) {
        break;
      }
    }

    if(i >= sizeof(shell_functions)/sizeof(shell_functions[0])) {
      shell_printf(sh->sh, "Command '%s' not found\n", cmd);
    } else {
      shell_printf(sh->sh, "%s - %s\n", shell_functions[i]->name, shell_functions[i]->description);
      shell_printf(sh->sh, "%s\n", shell_functions[i]->help);
    }

  }
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
static int exec_cb(void *arg, int argc, char **argv)
{
  SHELL_DATA *data;
  int r = SHELL_ERR_COMMAND_NOT_FOUND;
  unsigned i;

  data = (SHELL_DATA *) arg;

  (void) arg;
  for(i = 0; i < sizeof(shell_functions)/sizeof(shell_functions[0]); i++) {
    if(!strcmp(argv[0], shell_functions[i]->name)) {
      //Check -h or --help option
      if(argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        r = help_cb(data, argc, argv);
      } else {
        r = shell_functions[i]->cb(data, argc, argv);
      }
      break;
    }
  }

  return r;
}
/*----------------------------------------------------------------------------*/
static int shell_user_count(void) {return 0;} //TODO
/*----------------------------------------------------------------------------*/
int shell_init(SHELL_DATA *data)
{
  if(shell_user_count()) {
    set_login_mode(data);
  } else {
    set_shell_mode(data);
  }
  tty_set_prompt_cb(data->tty, prompt_cb, data);
  tty_set_ctrlc_c_cb(data->tty, ctrl_c_cb, data);

  shell_set_exec_cb(data->sh, exec_cb, data);


  tty_rx(data->tty, '\n');
  tty_set_enter_cb(data->tty, enter_cb, data);
  return 0;
}
/*----------------------------------------------------------------------------*/
void shell_destroy(SHELL_DATA *data)
{
  (void) data;
}
/*----------------------------------------------------------------------------*/
