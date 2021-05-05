/*----------------------------------------------------------------------------*/
/**
* @pkg c_sh_int
*/
/**
* Simple shell internal structures and types.
*
* (C) Oleg Yurchenko, Kiev, Ukraine 2021.<br>
* started 05.05.2021  8:30:27<br>
* @pkgdoc c_sh_int
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef C_SH_INT_H_1620192627
#define C_SH_INT_H_1620192627
#include "c_sh.h"
#include "c_cache.h"
/*----------------------------------------------------------------------------*/
#define VAR_NAME_LENGTH 16
#define MAX_LEXER_SIZE 256
#define DEFAULT_CACHE_SIZE 64*1024
#define BUFFER_SIZE 1024
/*----------------------------------------------------------------------------*/
typedef enum {
  LEX_ASSIGN //'='
  , LEX_NOT //!
  , LEX_AMP //&
  , LEX_VERBAR //|
  , LEX_AND //&&
  , LEX_OR //||
  , LEX_EQ  // ==
  , LEX_NE  // !=
  , LEX_GT  //>
  , LEX_GE  //>=
  , LEX_LT  //<
  , LEX_LE  //<=
  , LEX_LSQB //[
  , LEX_LDSQB //[[
  , LEX_RSQB //]
  , LEX_RDSQB //]
  , LEX_LPAREN //(
  , LEX_RPAREN //)
  , LEX_STR //string
  , LEX_COMMENT //#comm
} LEX_CODES;
/*----------------------------------------------------------------------------*/
typedef enum {
  OP_UNKNOWN = 0,
  OP_IF,
  OP_FOR,
  OP_WHILE,
  OP_UNTIL
} OP_CODE;
/*----------------------------------------------------------------------------*/
struct C_SHELL_VAR_TAG {
  struct C_SHELL_VAR_TAG *next;
  unsigned hash;
  char name[VAR_NAME_LENGTH];
  char value[1];
};
typedef struct C_SHELL_VAR_TAG C_SHEL_VAR;
/*----------------------------------------------------------------------------*/
typedef struct {
  const char *start;
  const char *end;
  int type;
  int reserved; //Alignmemnt
} LEX_ELEM;
/*----------------------------------------------------------------------------*/
struct C_SHELL_PARSER_TAG {
  LEX_ELEM lex[MAX_LEXER_SIZE];
  char *argv[MAX_LEXER_SIZE];
  int argc;
  int arg0; //Start argument (may be > 0)
  //int reserved; //Alignmemnt
};
typedef struct C_SHELL_PARSER_TAG C_SHELL_PARSER;
/*----------------------------------------------------------------------------*/
struct C_SHELL_LINE_CACHE_TAG {
  int op_id;
  struct C_SHELL_LINE_CACHE_TAG *next;
  char text[1];
};
typedef struct C_SHELL_LINE_CACHE_TAG C_SHELL_LINE_CACHE;
/*----------------------------------------------------------------------------*/
struct C_SHELL_CONTEXT_TAG {
  struct C_SHELL_CONTEXT_TAG *child;
  struct C_SHELL_CONTEXT_TAG *parent;
  struct {
    C_SHELL_LINE_CACHE *first;
    C_SHELL_LINE_CACHE *last;
    C_SHELL_LINE_CACHE *root;
  } cache;
  int op_id;
  OP_CODE op_code;
  int for_index;
  int condition:1;
  int should_store_cache:1;
  int is_loop:1;
  int is_continue:1;
};
typedef struct C_SHELL_CONTEXT_TAG C_SHELL_CONTEXT;
/*----------------------------------------------------------------------------*/
struct C_SHELL_INTERN_STREAM_TAG {
  int fd;
  unsigned size;
  unsigned position;
  unsigned allocated;
  unsigned hash;
  char name[VAR_NAME_LENGTH];
  char *buffer;
  struct C_SHELL_INTERN_STREAM_TAG *next;
};
typedef struct C_SHELL_INTERN_STREAM_TAG C_SHELL_INTERN_STREAM;
/*----------------------------------------------------------------------------*/
struct C_SHELL_TAG {
  struct {
    SHELL_PRINT_CB cb;
    void *arg;
  } print_cb;

  struct  {
    SHELL_EXEC_CB cb;
    void *arg;
  } exec_cb;

  struct  {
    SHELL_STEP_CB cb;
    void *arg;
  } step_cb;

  struct {
    SHELL_STREAM_HANDLER *ext_handler;
    int f[3]; /*File h f[0] - stdin, f[1] - stdout, f[2] - stderr*/
  } stream;

  C_CACHE *cache;
  C_SHELL_CONTEXT context;
  C_SHELL_PARSER *parser;
  C_SHEL_VAR *vars;
  C_SHELL_INTERN_STREAM *intern_stream;

  int op_id;
  int cache_stack;
  int cache_mode:1;
  int terminate:1;
};
/*----------------------------------------------------------------------------*/
#define TRUE_CONDITION 0
#define NOT_TRUE_CONDITION 1
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

int sh_exec(C_SHELL *sh, int argc, char **argv);
unsigned sh_hash(const char *str);


void sh_set_condition(C_SHELL *sh, int c);
void sh_set_true_condition(C_SHELL *sh);
void sh_set_false_condition(C_SHELL *sh);
int sh_get_condition(C_SHELL *sh);
int sh_is_true_condition(C_SHELL *sh);

int sh_push_context(C_SHELL *sh);
int sh_pop_context(C_SHELL *sh);

C_SHELL_CONTEXT *sh_root_context(C_SHELL *sh);
C_SHELL_CONTEXT *sh_current_context(C_SHELL *sh);


void sh_set_opcode(C_SHELL *sh, OP_CODE c);
OP_CODE sh_opcode(C_SHELL *sh);

int sh_play_cache(C_SHELL *sh);
int sh_remove_cache(C_SHELL *sh);
void sh_set_cache_mode(C_SHELL *sh, int mode);
int sh_is_cache_mode(C_SHELL *sh);

C_SHEL_VAR *sh_find_var(C_SHELL *sh, unsigned hash, C_SHEL_VAR **prior /*=NULL*/);
int sh_set_var(C_SHELL *sh, const char *name, const char *value);
const char *sh_get_var(C_SHELL *sh, const char *name);

int sh_is_debug_mode(C_SHELL *sh);
int sh_is_lex_debug_mode(C_SHELL *sh);
int sh_is_pars_debug_mode(C_SHELL *sh);
int sh_is_cache_debug_mode(C_SHELL *sh);
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_SH_INT_H_1620192627*/

