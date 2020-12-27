/*----------------------------------------------------------------------------*/
/**
* Demo shell functions.
*/
/*----------------------------------------------------------------------------*/
#include "shell.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
#ifndef MALLOC
#define MALLOC malloc
#endif
#ifndef FREE
#define FREE free
#endif
/*----------------------------------------------------------------------------*/
typedef struct {
  char user[MAX_USER_LENGTH];
  unsigned password;
} USER_ACCOUNT;

#define MAX_USER_COUNT 16
#define MAX_PASSWORD_LENGTH 32

static USER_ACCOUNT users[MAX_USER_COUNT];
static int user_count = 0;
/*----------------------------------------------------------------------------*/
extern const SHELL_FUNCTION  _date, _help, _passwd, _quit, _useradd, _userdel, _userlist;
static const SHELL_FUNCTION *shell_functions[] = {
  &_help
//, &_date
, &_passwd
, &_quit
, &_useradd
, &_userdel
, &_userlist
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
static int passwd_cb(SHELL_DATA *sh, int argc, char **argv);
static int useradd_cb(SHELL_DATA *sh, int argc, char **argv);
static int userdel_cb(SHELL_DATA *sh, int argc, char **argv);
static int userlist_cb(SHELL_DATA *sh, int argc, char **argv);
static int shell_login(const char *user, const char *password);
static int is_password_set(const char *user);

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

const SHELL_FUNCTION _passwd = {
  passwd_cb,
  "passwd",
  "Set user password",
  "Used: passwd [options] [LOGIN]\n"
  "Options:\n"
  "-d or --delete - delete the password for the named account\n"
  "If LOGIN not enter - use current user login"
};


const SHELL_FUNCTION _useradd = {
  useradd_cb,
  "useradd",
  "Add new user account",
  "Used: useradd <name> - where name - new user name"
};

const SHELL_FUNCTION _userdel = {
  userdel_cb,
  "userdel",
  "Delete user account",
  "Used: userdel <name> - where name - user name to remove"
};

const SHELL_FUNCTION _userlist = {
  userlist_cb,
  "userlist",
  "Print user list",
  "Used: userlist without args"
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
      if(is_password_set(data->user)) {
        set_password_mode(data);
      } else {
        set_shell_mode(data);
      }
      return 1;
    }
    return 0;
  }

  if(data->mode == PASSWORD_MODE) {
    //Check user & password
    if(!shell_login(data->user, text)) {
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
static unsigned pass_hash(const char *password)
{
  //Very secure cryptographic hash
  //Do not use in production !
  unsigned h = 5381;
  int c;

  while ((c = *password++) != 0)
      h = ((h << 5) + h) + c; /* hash * 33 + c */

  return h;
}
/*----------------------------------------------------------------------------*/
static int shell_login(const char *user, const char *password)
{
  int i;
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      //Check password
      if(users[i].password == pass_hash(password)) {
        return 0;
      }
    }
  }

  return -1;
}
/*----------------------------------------------------------------------------*/
static int is_password_set(const char *user)
{
  int i;
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      //Check password
      return users[i].password != 0;
    }
  }

  return 1;
}
/*----------------------------------------------------------------------------*/
static const char *prompts[] = {
  " Enter NEW PASSWORD ",
  "Retype NEW PASSWORD ",
};
/*----------------------------------------------------------------------------*/
typedef struct {
  TTY *tty;
  int attempt;
  char user[MAX_USER_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
} PASSWD_ENTER_DATA;
/*----------------------------------------------------------------------------*/
static int passwd_enter_cb(void *arg, const char *text, unsigned size)
{
  PASSWD_ENTER_DATA *data;
  int i;

  data = (PASSWD_ENTER_DATA *)arg;

  tty_set_prompt(data->tty, prompts[1]);
  data->attempt ++;
  if(data->attempt > 1) {
    if(size != strlen(data->password) || memcmp(data->password, text, size)) {
      tty_printf(data->tty, "Password does not match\n");
    } else {
      //Set password
      for(i = 0; i < user_count; i++) {
        if(!strcmp(users[i].user, data->user)) {
          users[i].password = pass_hash(data->password);
          break;
        }
      }
    }
    //Return parent tty
    tty_delete_later(data->tty);

    memset(data, 0, sizeof(PASSWD_ENTER_DATA)); //clear pass
    FREE(data);
  } else {
    if(size < sizeof(data->password)) {
      memcpy(data->password, text, size);
      data->password[size] = 0;
    }
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
extern int passwd_ctrc_cb(void *arg, const char *text, unsigned size)
{
  PASSWD_ENTER_DATA *data;

  (void) text;
  (void) size;

  data = (PASSWD_ENTER_DATA *)arg;

  //Return parent tty
  tty_delete_later(data->tty);

  memset(data, 0, sizeof(PASSWD_ENTER_DATA)); //clear pass
  FREE(data);

  return 0;
}
/*----------------------------------------------------------------------------*/
static int rm_pass(SHELL_DATA *sh, const char *user)
{
  int i;
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      //Set password
      users[i].password = 0;
      break;
    }
  }

  if(i >= user_count) {
    shell_printf(sh->sh, "User %s not found\n", user);
    return 1;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
static int passwd_cb(SHELL_DATA *sh, int argc, char **argv)
{
  PASSWD_ENTER_DATA *data;
  int i, f, del = 0;
  const char *user;

  user = sh->user;
  for(i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--delete")) {
      del = 1;
      continue;
    }
    user = argv[i];
  }




  if(argc > 1) {
    if(!*user || strlen(argv[1]) > MAX_USER_LENGTH) {
      shell_printf(sh->sh, "Name length out of range\n");
      return 1;
    }
  }

  if(del) {
    return rm_pass(sh, user);
  }

  data = (PASSWD_ENTER_DATA *) MALLOC(sizeof(PASSWD_ENTER_DATA));
  if(data == NULL) {
    return SHELL_ERR_MALLOC;
  }

  memset(data, 0, sizeof(PASSWD_ENTER_DATA));
  strcpy(data->user, user);

  //Find user
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      break;
    }
  }

  if(i >= user_count) {
    shell_printf(sh->sh, "User %s not found\n", user);
    FREE(data);
    return 1;
  }

  data->tty = tty_alloc(MAX_PASSWORD_LENGTH, 0, 24);
  if(data->tty == NULL) {
    FREE(data);
    return SHELL_ERR_MALLOC;
  }


  tty_set_interceptor(sh->tty, data->tty);
  tty_set_prompt(data->tty, prompts[0]);
  tty_set_password_mode(data->tty, 1);
  tty_set_echo_mode(data->tty, 1);
  tty_rx(data->tty, '\n');
  tty_set_enter_cb(data->tty, passwd_enter_cb, data);
  tty_set_ctrlc_c_cb(data->tty, passwd_ctrc_cb, data);
  return 0;
}
/*----------------------------------------------------------------------------*/
static int user_add(SHELL_DATA *sh, const char *user)
{
  int i;
  unsigned size;

  size = strlen(user);

  if(!size || size >= MAX_USER_LENGTH) {
    shell_printf(sh->sh, "Name length out of range\n");
    return 1;
  }

  if(user_count >= MAX_USER_COUNT - 1) {
    shell_printf(sh->sh, "The number of users has exceeded the limit\n");
    return 1;
  }

  //Find user
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      break;
    }
  }

  if(i < user_count) {
    shell_printf(sh->sh, "User %s already exist\n", user);
    return 1;
  }

  memcpy(users[user_count].user, user, size);
  users[user_count].user[size] = 0;
  users[user_count].password = 0;
  user_count ++;

  return 0;
}
/*----------------------------------------------------------------------------*/
static int useradd_cb(SHELL_DATA *sh, int argc, char **argv)
{
  int result = 0, i;
  if(argc < 2) {
    shell_printf(sh->sh, "Invalid arg\n");
    return 1;
  }
  for(i = 1; i < argc; i++) {
    result = user_add(sh, argv[i]);
    if(result) {
      break;
    }
  }
  return result;
}
/*----------------------------------------------------------------------------*/
static int user_del(SHELL_DATA *sh, const char *user)
{
  int i, j;
  for(i = 0; i < user_count; i++) {
    if(!strcmp(users[i].user, user)) {
      break;
    }
  }

  if(i >= user_count) {
    shell_printf(sh->sh, "User %s not found\n", user);
    return 1;
  }

  for(j = j; j < user_count - 1; j++) {
    users[j] = users[j + 1];
  }
  user_count --;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int userdel_cb(SHELL_DATA *sh, int argc, char **argv)
{
  int result = 0, i;
  if(argc < 2) {
    shell_printf(sh->sh, "Invalid arg\n");
    return 1;
  }

  for(i = 1; i < argc; i++) {
    result = user_del(sh, argv[i]);
    if(result) {
      break;
    }
  }
  return result;
}
/*----------------------------------------------------------------------------*/
static int userlist_cb(SHELL_DATA *sh, int argc, char **argv)
{
  (void) argc;
  (void) argv;

  int i, j;
  for(i = 0; i < user_count; i++) {
    shell_printf(sh->sh, "%s\n", users[i].user);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
int shell_init(SHELL_DATA *data)
{
  if(user_count) {
    set_login_mode(data);
  } else {
    set_shell_mode(data);
    shell_printf(data->sh,
                 "User accounts are not programmed\n"
                 "Use 'useradd' & 'passwd' for create accounts\n"
                 );
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
