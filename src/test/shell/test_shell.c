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
const char* ts_keyword(void *arg, unsigned index)
{
  (void) arg;

  if(index < sizeof (shell_commands) / sizeof (shell_commands[0])) {
    return shell_commands[index];
  }
  return NULL;
}
/*----------------------------------------------------------------------------*/
static int _read(void *arg, int argc, char **argv);
/*----------------------------------------------------------------------------*/
int ts_exec(void *arg, int argc, char **argv)
{
  if(!strcmp(argv[0], "read")) {
    return _read(arg, argc, argv);
  }
  return SHELL_ERR_COMMAND_NOT_FOUND;
}
/*----------------------------------------------------------------------------*/
static int _read(void *arg, int argc, char **argv)
{
  TEST_SHELL_DATA *data;

  data = (TEST_SHELL_DATA *) arg;
  (void) argc;
  (void) argv;
  return SHELL_OK;
}
/*----------------------------------------------------------------------------*/
