/*----------------------------------------------------------------------------*/
/**
* @pkg c_tty
*/
/**
* TTY (teletypewriter) - Character User Interface for microcontroller (telnet/minicom backend).
*
* c_tty supports basic line editing, understanding the keyS:
* - backspace to delete characters,
* - Del to delete characters,
* - left and right arrow to move the insertion point,
* -  Ctrl-C to ditch the entire line,
* - Up and Down arrow for select line from history buffer.<br>
* - Tab  to tab Completion
* (C) T&T team, Kiev, Ukraine 2020.<br>
* started 04.12.2020 11:33:36<br>
* @pkgdoc c_tty
* @author oleg
* @version 0.01
*/
/*----------------------------------------------------------------------------*/
#include "c_tty.h"
#include "loop_buff.h"
#include "nprintf.h"
/*----------------------------------------------------------------------------*/
#define UTF8_SUPPORT
/*----------------------------------------------------------------------------*/
#undef STDLIB
#if !defined(NOSTDLIB)
  #if defined (UNIX) || defined (LINUX) || defined (WIDOWS) || defined (WIN32)
    #define STDLIB
  #endif //defined (UNIX) || defined (LINUX) || defined (WIDOWS) || defined (WIN32)
#endif //NOSTDLIB
/*----------------------------------------------------------------------------*/
#if defined (WIN_CONSOLE) || defined (UNIX_CONSOLE)
  #define STDLIB
#endif //CONSOLE
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
#ifdef WIN_CONSOLE
#include <windows.h>
#include <io.h>
#endif
/*----------------------------------------------------------------------------*/
#ifdef UNIX_CONSOLE
#include <unistd.h>
#include <termios.h>
#endif
/*----------------------------------------------------------------------------*/
static int tty_writer(void *data, const char *buffer, int size);
/*----------------------------------------------------------------------------*/
#define ESC_CURSOR_RIGHT    "\33[1C"
#define ESC_CURSOR_LEFT     "\33[1D"
#define ESC_ERASE_LINE      "\33[2K"

#define CTRL_C 3  // ^C
#define BS 8      // backspace
#define DEL 127   // delete

#define ESCCHAR         27
#define ESCCHAR_UP      'A'
#define ESCCHAR_DOWN    'B'
#define ESCCHAR_RIGHT   'C'
#define ESCCHAR_LEFT    'D'
#define ESCCHAR_HOME    'H'
#define ESCCHAR_END     'F'
#define ESCCHAR_DEL     126
#define ESCCHAR_CTRLLEFT    'd'
#define ESCCHAR_CTRLRIGHT   'c'
typedef enum  {
    IN_ESCAPE = 0x01,
    IN_BRACKET_ESCAPE = 0x02,
    IN_NUMERIC_ESCAPE = 0x04,
} TTY_FLAGS;
/*----------------------------------------------------------------------------*/
typedef enum {

  MOVE_NEXT = 1,
  MOVE_PRIOR = -1,
  MOVE_FIRST = -2,
  MOVE_LAST = 2

} TTY_MOVE;
/*----------------------------------------------------------------------------*/
struct TTY_TAG {
  struct {
    TTY_PRINT_CB cb;
    void *arg;
  } print_cb;

  struct {
    TTY_ENTER_CB cb;
    void *arg;
  } enter_cb;

  struct {
    TTY_OVERFLOW_CB cb;
    void *arg;
  } overflow_cb;

  struct {
    TTY_PROMPT_CB cb;
    void *arg;
  } prompt_cb;

  struct {
    TTY_CTRL_C_CB cb;
    void *arg;
  } ctrl_c_cb;


  struct {
    TTY_TAB_CB cb;
    TTY_KW_CB kw_cb;
    void *arg;
    const char **kwords;
    unsigned kw_size;
  } tab_cb;


  struct {
    LOOP_BUFFER lb;
    unsigned pos;
  } histore;

  struct {
    char *buffer;
    int length;
    int pos;
    int size;
  } input;

  struct {
    char *buffer;
    int length;
    int size;
  } prompt;

  unsigned flags;

  struct {
    int password : 1;
    int echo : 1;
    int histore : 1;
    int delete_later : 1;
    int kill_prompt : 1;
    int reserved : 28;
  } mode;

  struct TTY_TAG *child;
  struct TTY_TAG *parent;

};
/*----------------------------------------------------------------------------*/
#ifdef UTF8_SUPPORT
/**Is 1-rst multibyte sequence*/
#define IS_MB1(c) ( (((c) & 0x80) == 0) || (((c) & 0xe0) == 0xc0) || (((c) & 0xf0) == 0xe0) || (((c) & 0xf8)==0xf0) )
#else
#define IS_MB1(c) ( (c) | 1 ) //Always TRUE
#endif

#if defined(UTF8_SUPPORT) && defined(WIN_CONSOLE)
/**
 * @brief mb_size - get size of UTF-8 multibyte char
 * @param c - first byte in char
 * @return size 1...4
 */
static int mb_size(int c)
{
  if((c & 0x80) == 0)
    return 1;

  if((c & 0xe0) == 0xc0)
    return 2;

  if((c & 0xf0) == 0xe0)
    return 3;

  if((c & 0xf8) == 0xf0)
    return 4;

  return 1;
}
#endif
/*----------------------------------------------------------------------------*/
static const char delimiters[] = "[]|;{}+*-=><!$&()\"\'`";
/*----------------------------------------------------------------------------*/
static int tty_print(TTY* tty, int c)
{
  int result = 0;
  if(tty != NULL && tty->print_cb.cb != NULL) {
#if defined(UTF8_SUPPORT) && defined(WIN_CONSOLE)
    //Convert UTF-8 character sequency to ANSI char
    static char ubuf[4];
    static int upos = 0, usize = 0;

    if(upos < usize) {
      //Next byte from sequency
      ubuf[upos ++] = (char) c;
      if(upos >= usize) {
        wchar_t buf[2];
        //Convert to UNICODE
        usize = MultiByteToWideChar(CP_UTF8, 0, ubuf, usize, buf, sizeof(buf)/sizeof(buf[0]));
        if(usize) {
          //Convert to ANSI
          usize = WideCharToMultiByte(CP_OEMCP, 0, buf, usize, ubuf, sizeof(ubuf), NULL, NULL);
        }

        //Send to console
        for(upos = 0; upos < usize; upos++) {
          result = tty->print_cb.cb(tty->print_cb.arg, ubuf[upos]);
          if(result < 0) {
            break;
          }
        }
        upos = usize = 0;
        return result;
      }
    } else {
      usize = mb_size(c);
      if(usize > 1 && usize < (int) sizeof(ubuf)) {
        //Start mulibyte sequency
        ubuf[0] = (char) c;
        upos = 1;
        return 1;
      }
      usize = 0;
    }


#endif //defined(UTF8_SUPPORT) && defined(WIN_CONSOLE)
    result = tty->print_cb.cb(tty->print_cb.arg, c);
  }
  return result;
}
/*----------------------------------------------------------------------------*/
static int tty_writer(void *data, const char *buffer, int size)
{
  TTY* tty;
  int i, result = 0, ret;

  tty = (TTY *) data;
  for(i = 0; i < size; i++) {
    if((ret = tty_print(tty, *buffer)) < 0) {
      return -1;
    }
    result += ret;
    buffer ++;
  }
  return result;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_pop_histore - remove oldest text form histore
 * @param tty
 */
static void tty_pop_histore(TTY* tty)
{
  int ret = 1;
  char c = 1;
  while (c && ret) {
    ret = lb_pop(&tty->histore.lb, &c);
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_push_histore - Add string to histore
 * @param tty - handle
 * @param txt - text
 * @param size - text size
 */
static void tty_push_histore(TTY* tty, const char *txt, unsigned size)
{
  int i, j;
  if(!size || size + 1 >= tty->histore.lb.size)
    return;


  //Check if last is equal
  if(lb_size(&tty->histore.lb) > 2) {
    i = lb_size(&tty->histore.lb) - 2;
    j = size - 1;
    while(i >= 0 && j >= 0) {
      if(*lb_at(&tty->histore.lb, i) != txt[j])
        break;
      i --;
      if(i < 0) {
        i = lb_size(&tty->histore.lb) - 1;
      }
      j --;
    }

    if(j < 0) {
      if(!*lb_at(&tty->histore.lb, i))
        return; //Equal
    }
  }

  while(lb_free(&tty->histore.lb) < size + 1) {
    tty_pop_histore(tty);
  }

  tty->histore.pos = lb_size(&tty->histore.lb);
  lb_push_buffer(&tty->histore.lb, txt, size);
  lb_push(&tty->histore.lb, 0);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief histore_substitution - copy histore by histore position to input buffer
 * @param tty - handle
 */
static void histore_substitution(TTY *tty)
{
  int i, pos;

  if(lb_size(&tty->histore.lb) < 2 || tty->histore.lb.size < 2)
    return;

  pos = tty->histore.pos;
  for(i = 0; i < tty->input.size - 1; i++) {
    tty->input.buffer[i] = *lb_at(&tty->histore.lb, pos);
    if(!tty->input.buffer[i])
      break;
    pos ++;
  }

  tty->input.length = tty->input.pos = i;
  tty_restore(tty);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief histore_select - nove histore position
 * @param tty - handle
 * @param move - direction (home, end, next, prior)
 */
static void histore_select(TTY *tty, TTY_MOVE move)
{
  int i, pos;
  unsigned size;

  size = lb_size(&tty->histore.lb);

  if(size < 2 || tty->histore.lb.size < 2)
    return;

  pos = tty->histore.pos;

  if(move == MOVE_FIRST) {
    pos = 0;
  }

  if(move == MOVE_LAST) {
    pos = size - 1;
    move = MOVE_PRIOR;
  }

  if(move == MOVE_NEXT) {
    for(i = 0; i < (int)size - 1; i++) {
      if(!tty->input.buffer[i])
        break;
      pos ++;
    }

    pos ++;
    if(pos >= (int) size)
      pos = 0;
  }

  if(move  == MOVE_PRIOR) {
    pos --;
    if(pos < 0)
      pos = size - 1;
    while(1) {
      pos --;
      if(pos < 0)
        pos = size - 1;
      if(!*lb_at(&tty->histore.lb, pos))
        break;
    }
    pos ++;
    if(pos >= (int) size)
      pos = 0;

  }

  tty->histore.pos = pos;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_alloc - memory allocate and init TTY handle
 * @param input_buffer_size - size of input buffer. -1 - default size
 * @param histore_size - size of histore buffer size. -1 - default size
 * @param prompt_size - size of prompt buffer size. -1 - default size
 * @return pointer to handle. NULL if memory allocate error
 */
TTY* tty_alloc(int input_buffer_size, /*=-1*/ int histore_size /*=-1*/, int prompt_size /*=-1*/)
{
  TTY* tty;
  unsigned size;
  char *data;

  if(input_buffer_size < 0)
    input_buffer_size = TTY_DEFAULT_INPUT_SIZE;
  if(histore_size < 0)
    histore_size = TTY_DEFAULT_HISTORE_SIZE;
  if(prompt_size < 0)
    prompt_size = TTY_DEFAULT_PROMPT_SIZE;

  size = sizeof(TTY) + input_buffer_size + histore_size + prompt_size;
  data = (char *) MALLOC(size);
  if(data == NULL)
    return NULL;
  memset(data, 0, size);
  tty = (TTY *) data;
  tty->input.buffer = &data[sizeof(TTY)];
  tty->input.size = (int) input_buffer_size;
  lb_init(&tty->histore.lb, &data[sizeof(TTY) + input_buffer_size], histore_size);
  tty->prompt.buffer = &data[sizeof(TTY) + input_buffer_size + histore_size];
  tty->prompt.size = (int) prompt_size;
  tty->mode.histore = tty->histore.lb.size > 4 ? 1 : 0;

  return tty;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_free - memory free
 * @param tty - handle
 */
void tty_free(TTY* tty)
{
    if(tty->child != NULL) {
      tty_free(tty->child);
    }
    if(tty->parent != NULL && tty->parent->child == tty) {
      tty->parent->child = NULL;
      tty_restore(tty->parent);
    }

    FREE(tty);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_delete_later - mark to delete (use inside callback where not allowed call tty_free)
 * @param tty - handle
 */
void tty_delete_later(TTY* tty)
{
  tty->mode.delete_later = 1;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_print_cb - set printing callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_print_cb(TTY* tty, TTY_PRINT_CB cb, void *cb_arg)
{
  tty->print_cb.cb = cb;
  tty->print_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_enter_cb - set enter text callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_enter_cb(TTY* tty, TTY_ENTER_CB cb, void *cb_arg)
{
  tty->enter_cb.cb = cb;
  tty->enter_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_overflow_cb - set buffer overflow callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_overflow_cb(TTY* tty, TTY_OVERFLOW_CB cb, void *cb_arg)
{
  tty->overflow_cb.cb = cb;
  tty->overflow_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_prompt_cb - set get prompt callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_prompt_cb(TTY* tty, TTY_PROMPT_CB cb, void *cb_arg)
{
  tty->prompt_cb.cb = cb;
  tty->prompt_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_prompt - set simple prompt text
 * @param tty - TTY handle
 * @param text - text of prompt ala "#"
 */
void tty_set_prompt(TTY* tty, const char *text)
{
  if(tty->prompt.size < 2)
    return;
  strncpy(tty->prompt.buffer, text, tty->prompt.size - 1);
  tty->prompt.buffer[tty->prompt.size - 1] = 0;
  tty->prompt.length = (int) strlen(tty->prompt.buffer);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_ctrlc_c_cb - set CTRL+C callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_ctrlc_c_cb(TTY* tty, TTY_CTRL_C_CB cb, void *cb_arg)
{
  tty->ctrl_c_cb.cb = cb;
  tty->ctrl_c_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_tab_cb - set TAB callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_tab_cb(TTY* tty, TTY_TAB_CB cb, void *cb_arg)
{
  tty->tab_cb.cb = cb;
  tty->tab_cb.arg = cb_arg;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief intern_tab_cb - internal Tab Completion handle for the constatnt keywords massive
 * @param arg - arbitrary argument, (TTY pointer)
 * @param text - entered test
 * @param size - sizeo of entered test
 * @return - ignored
 */
static int intern_tab_cb(void *arg, const char *text, unsigned size)
{
  unsigned i, j, kw_matched = 0, char_mathced=0xffffffff;
  const char *cmd = NULL, *kw;
  TTY *tty;

  tty = (TTY *)arg;

  if(tty->tab_cb.kw_cb == NULL) {
    return 0;
  }


  if(size >= 1) {
    i = size - 1;
    while (i) {
      if( isblank(text[i]) || strchr(delimiters, text[i]) != NULL) {
        text = text + i + 1;
        size -= i + 1;
        break;
      }
      i --;
    }
  }

  if(size >= 1) {
    for(i = 0; NULL != (kw = tty->tab_cb.kw_cb(tty->tab_cb.arg, i)); i ++) {
      if(strstr(kw, text) == kw) {
        if(cmd != NULL) {
          j = 0;
          while(cmd[j] && cmd[j] == kw[j]) {
            j ++;
          }
          if(char_mathced > j) {
            char_mathced = j;
          }
        }
        cmd = kw;
        kw_matched ++;
      }
    }
  }

  if(kw_matched == 1) {
    i = size;
    size = strlen(cmd);
    for(; i < size; i++) {
      tty_ins(tty, cmd[i]);
    }
  }

  if(kw_matched > 1) {
    tty_putc(tty, '\n');
    for(i = 0; NULL != (kw = tty->tab_cb.kw_cb(tty->tab_cb.arg, i)); i ++) {
      if(strstr(kw, text) == kw) {
        tty_printf(tty, "%s\n", kw);
      }
    }

    for(i = size; i < char_mathced; i++) {
      tty_ins(tty, cmd[i]);
    }

    tty_restore(tty);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_tab_completion - run tab completion with known keywords
 * @param tty - TTY handle
 * @param text - not complete text
 * @param size - text size
 * @param keywords - keywords
 * @param kw_size - keywords list size
 * @return -1 if error
 */
int tty_tab_completion(TTY *tty, const char *text, unsigned size, const char **keywords, unsigned kw_size)
{
  unsigned i, j, kw_matched = 0, char_mathced=0xffffffff;
  const char *cmd = NULL, *kw;

  if(keywords == NULL || !kw_size) {
    return 0;
  }


  if(size >= 1) {
    i = size - 1;
    while (i) {
      if(isblank(text[i]) || strchr(delimiters, text[i]) != NULL) {
        text = text + i + 1;
        size -= i + 1;
        break;
      }
      i --;
    }
  }

  if(size >= 1) {
    for(i = 0; i < kw_size &&  NULL != (kw = keywords[i]); i ++) {
      if(strstr(kw, text) == kw) {
        if(cmd != NULL) {
          j = 0;
          while(cmd[j] && cmd[j] == kw[j]) {
            j ++;
          }
          if(char_mathced > j) {
            char_mathced = j;
          }
        }
        cmd = kw;
        kw_matched ++;
      }
    }
  }

  if(kw_matched == 1) {
    i = size;
    size = strlen(cmd);
    for(; i < size; i++) {
      tty_ins(tty, cmd[i]);
    }
  }

  if(kw_matched > 1) {
    tty_putc(tty, '\n');
    for(i = 0; i < kw_size &&  NULL != (kw = keywords[i]); i ++) {
      if(strstr(kw, text) == kw) {
        tty_printf(tty, "%s\n", kw);
      }
    }

    for(i = size; i < char_mathced; i++) {
      tty_ins(tty, cmd[i]);
    }

    tty_restore(tty);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
static const char *intern_kword_cb(void *arg, unsigned index)
{
  TTY *tty;

  tty = (TTY *)arg;
  if(tty->tab_cb.kwords == 0 || tty->tab_cb.kw_size <= index) {
    return NULL;
  }
  return tty->tab_cb.kwords[index];
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_keywords - set keywords for internal Tab Completion.
 * @param tty - TTY handle
 * @param kwords - point to keywords area
 * @param size - sizeo of keyword area
 */
void tty_set_keywords(TTY* tty, const char **kwords, unsigned size)
{
  tty->tab_cb.kwords = kwords;
  tty->tab_cb.kw_size = size;

  if(tty->tab_cb.kwords != NULL
     && tty->tab_cb.kw_size
     && tty->tab_cb.cb == NULL) {
    //Set internal callback
    tty_set_tab_cb(tty, intern_tab_cb, tty);
    tty_set_kwords_cb(tty, intern_kword_cb, tty);
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_kwords_cb - set kwords callback for (for Tab Completion)
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_kwords_cb(TTY* tty, TTY_KW_CB cb, void *cb_arg)
{
  tty->tab_cb.kw_cb = cb;
  tty->tab_cb.arg = cb_arg;

  if(tty->tab_cb.kw_cb != NULL && tty->tab_cb.cb == NULL) {
    tty_set_tab_cb(tty, intern_tab_cb, tty);
  }
}
/**
 * @brief tty_input_length - get TTY user input string length
 * @param tty - TTY handle
 * @return - input string length
 */
int tty_input_length(TTY *tty)
{
  return tty->input.length;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_input_position - get cursor position
 * @param tty - TTY handle
 * @return - cursor position
 */
int tty_input_position(TTY *tty)
{
  return tty->input.pos;
}
/*----------------------------------------------------------------------------*/
static void tty_print_prompt(TTY* tty)
{
  if(tty->prompt.size >= 2 && tty->prompt_cb.cb != NULL) {
    tty->prompt_cb.cb(tty->prompt_cb.arg, tty->prompt.buffer, tty->prompt.size - 1);
    tty->prompt.buffer[tty->prompt.size - 1] = 0;
    tty->prompt.length = (int) strlen(tty->prompt.buffer);
  }
  tty_writer(tty, tty->prompt.buffer, tty->prompt.length);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_input_buffer - get TTY input buffer
 * @param tty - TTY handle
 * @return pointer to buffer
 */
const char *tty_input_buffer(TTY *tty)
{
  return tty->input.buffer;
}
/*----------------------------------------------------------------------------*/
#ifdef WIN_CONSOLE
void move_cursor_x(int x)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if(GetConsoleScreenBufferInfo(hConsole, &info)) {
    info.dwCursorPosition.X += x;
    SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
  }
}
#endif

/**
 * Move only the terminal cursor. This does not move the insertion point.
 */
static void term_cursor_move(TTY *tty, int n)
{
#ifdef WIN_CONSOLE
  (void) tty;
  move_cursor_x(n);
#else
  for ( ; n > 0; --n) {
    tty_puts(tty, ESC_CURSOR_RIGHT);
  }

  for ( ; n < 0; ++n) {
    tty_puts(tty, ESC_CURSOR_LEFT);
  }
#endif
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_cursor_move - Move the tty cursor. This moves the insertion point.
 * @param tty - TTY handle
 * @param n - position delta
 */
void tty_cursor_move(TTY *tty, int n)
{
  int pos;
  if (tty->input.pos + n < 0) {
    n = -tty->input.pos;
  }
  else
    if (tty->input.pos + n > (int) tty->input.length) {
      n = tty->input.length - tty->input.pos;
    }

  pos = tty->input.pos + n;
  if(n > 0) {
    while (pos < tty->input.length && !IS_MB1(tty->input.buffer[pos])) {
      pos ++;
    }
  }

  if(n < 0) {
    while (pos > 0 && !IS_MB1(tty->input.buffer[pos])) {
      pos --;
    }
  }

  term_cursor_move(tty, n);
  tty->input.pos = pos;
}
/*----------------------------------------------------------------------------*/
/**
 * Move the tty cursor backwards or forwards one word. This applies history
 * substitution, moves the terminal cursor, and moves the insertion point.
 *
 * @param dir - move forward if +1 or negative if -1. All other numbers produce
 *              undefined behavior.
 */
static void word_move(TTY *tty, int dir)
{
  int pos = tty->input.pos;

  if (dir == 0) {
    return;
  }
  else
    if (dir < 0) {
      for (; pos > 0 && tty->input.buffer[pos - 1] == ' '; --pos);
      for (; pos > 0 && tty->input.buffer[pos - 1] != ' '; --pos);
    } else {
      const int length = tty->input.length;
      for (; pos < length && tty->input.buffer[pos] != ' '; ++pos);
      for (; pos < length && tty->input.buffer[pos] == ' '; ++pos);
    }

  term_cursor_move(tty, pos - tty->input.pos);
  tty->input.pos = pos;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_restore - restore user screen
 * @param tty - handle
 */

#ifdef WIN_CONSOLE
void erase_line(TTY *tty)
{
  SHORT x;
  DWORD w;
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE hConsole;

  (void) tty;

  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  if(GetConsoleScreenBufferInfo(hConsole, &info)) {

    info.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
    for(x = 0; x < info.dwSize.X - 1; x++) {
      WriteConsole(hConsole, " ", 1, &w, 0);
    }

    info.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
  }
}
#endif

void tty_restore(TTY *tty)
{
  int move = 0, pos;
#ifdef WIN_CONSOLE
  erase_line(tty);
#else
  tty_puts(tty, ESC_ERASE_LINE "\r"); // Clear line
#endif
  tty_print_prompt(tty);

  if(tty_is_echo_mode(tty)) {
    if(tty_is_password_mode(tty)) {
      pos = 0;
      while(pos < tty->input.length) {
        tty_putc(tty, '*');
        pos ++;
        while (pos < tty->input.length && !IS_MB1(tty->input.buffer[pos])) {
          pos ++;
        }
      }

    } else {
      tty->input.buffer[tty->input.length] = 0;
      tty_puts(tty, tty->input.buffer);
    }

    pos = tty->input.pos;
    while(pos < tty->input.length) {
      pos ++;
      while (pos < tty->input.length && !IS_MB1(tty->input.buffer[pos])) {
        pos ++;
      }
      move --;
    }

    term_cursor_move(tty,  move);
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_ins - insert char char after input position
 * @param tty - handle
 * @param c - char
 */
void tty_ins(TTY *tty, char c)
{
  int move;

  if(tty->input.length >= tty->input.size)
    return;

  move = (tty->input.pos != tty->input.length);

  memmove(&tty->input.buffer[tty->input.pos + 1],
      &tty->input.buffer[tty->input.pos],
      tty->input.length - tty->input.pos);

  if (c) {
    tty->input.buffer[tty->input.pos] = c;
  }

  tty->input.length ++;
  tty->input.pos ++;

  if (move) {
    tty_restore(tty);
  } else {
    if(tty_is_echo_mode(tty)) {
      if(tty_is_password_mode(tty)) {
        tty_putc(tty, '*');
      } else {
        tty_putc(tty, c);
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_bs - delete char before input position
 * @param tty - handle
 */
void tty_bs(TTY *tty)
{
  int move, pos;
  if(tty->input.pos < 1)
    return;

  move = (tty->input.pos != tty->input.length);

  pos = tty->input.pos - 1;

  while(pos > 0 && !IS_MB1(tty->input.buffer[pos])) {
    pos --;
  }

  memmove(&tty->input.buffer[pos],
          &tty->input.buffer[tty->input.pos],
      tty->input.length - pos);


    tty->input.length -= tty->input.pos - pos;
    tty->input.pos = pos;

    if (move) {
        tty_restore(tty);
    } else {
        tty_puts(tty, "\b \b");
    }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_del - delete char on input position
 * @param tty - handle
 */
void tty_del(TTY *tty)
{
  int pos;
  if(tty->input.pos >= tty->input.length)
    return;

  pos = tty->input.pos + 1;
  while(pos < tty->input.length && !IS_MB1(tty->input.buffer[pos])) {
    pos ++;
  }

  memmove(&tty->input.buffer[tty->input.pos],  &tty->input.buffer[pos],
      tty->input.length - pos);


  tty->input.length -= pos - tty->input.pos;

  tty_restore(tty);
}
/*----------------------------------------------------------------------------*/
static void tty_handle_char(TTY* tty, int c)
{
  if (tty->input.length < tty->input.size - 1) {
    tty_ins(tty, c);
  }
  else {
    tty->input.length = tty->input.size;

    tty->input.buffer[tty->input.size - 1] = 0;
  }
}
/*----------------------------------------------------------------------------*/
static void tty_enter(TTY* tty)
{
  if (tty->input.length >= tty->input.size) {
    if(tty->overflow_cb.cb != NULL) {
      tty->overflow_cb.cb(tty->overflow_cb.arg, tty->input.buffer, tty->input.length);
    }
    tty->input.length = tty->input.pos = 0;
    tty_print_prompt(tty);
    return;
  }
  else {
    tty->input.buffer[tty->input.length] = 0;
  }

  tty_putc(tty, '\n');
  if(tty->input.length && !tty_is_password_mode(tty) && tty_is_histore_mode(tty))
    tty_push_histore(tty, tty->input.buffer, tty->input.length);

  if(tty_is_histore_mode(tty)) {
    histore_select(tty, MOVE_LAST);
  }

  if(tty->enter_cb.cb != NULL) {
    tty->enter_cb.cb(tty->enter_cb.arg, tty->input.buffer, tty->input.length);
  }

  tty->input.length = tty->input.pos = 0;

  if(tty->mode.kill_prompt) {
    tty->mode.kill_prompt = 0;
  } else {
    tty_print_prompt(tty);
  }
}
/*----------------------------------------------------------------------------*/
static void tty_handle_ctrl(TTY* tty, int c)
{
  switch (c) {
    case ESCCHAR: // escape
      tty->flags |= IN_ESCAPE;
      break;
    case CTRL_C:  // ^C
      if(tty->ctrl_c_cb.cb != NULL) {
        if(tty->input.length < tty->input.size)
          tty->input.buffer[tty->input.length] = 0;
        tty->ctrl_c_cb.cb(tty->ctrl_c_cb.arg, tty->input.buffer, tty->input.length);
      }
      tty_puts(tty, "^C\n");
      tty_print_prompt(tty);
      tty->input.length = tty->input.pos = 0;
      if(tty_is_histore_mode(tty)) {
        histore_select(tty, MOVE_LAST);
      }
      break;
    case '\n':
      tty_enter(tty);
      break;
    case '\t':
      if(tty->tab_cb.cb != NULL) {
        if(tty->input.length < tty->input.size)
          tty->input.buffer[tty->input.length] = 0;
        tty->tab_cb.cb(tty->tab_cb.arg, tty->input.buffer, tty->input.length);
      }
      break;
    case BS:    // backspace
    case DEL:   // delete
      if (tty->input.length > 0
          && tty->input.length < tty->input.size
          && tty->input.pos > 0) {
        tty_bs(tty);
      }
      break;
    default:
      break;
  }
}
/*----------------------------------------------------------------------------*/
static void tty_handle_esc(TTY* tty, int esc)
{
  int cmove = 0, wmove = 0;

  if (esc >= '0' && esc <= '9') {
    tty->flags
        |= IN_ESCAPE | IN_BRACKET_ESCAPE | IN_NUMERIC_ESCAPE;
    return;
  }

  if (tty->flags & (IN_NUMERIC_ESCAPE | IN_BRACKET_ESCAPE)) {
    tty->flags
        &= ~(IN_BRACKET_ESCAPE | IN_NUMERIC_ESCAPE | IN_ESCAPE);
  }

  switch (esc) {
    case ESCCHAR_UP:
      if(tty_is_histore_mode(tty)) {
        histore_substitution(tty);
        histore_select(tty, MOVE_PRIOR);
      }
      break;
    case ESCCHAR_DOWN:
      if(tty_is_histore_mode(tty)) {
        histore_substitution(tty);
        histore_select(tty, MOVE_NEXT);
      }
      break;
    case ESCCHAR_DEL:
      if (tty->input.length > 0
          && tty->input.length < tty->input.size
          && tty->input.pos < tty->input.length) {
         tty_del(tty);
       }
      break;
    case ESCCHAR_LEFT:
      cmove = -1;
      break;
    case ESCCHAR_RIGHT:
      cmove = 1;
      break;
    case ESCCHAR_HOME:
      cmove = -tty->input.pos;
      break;
    case ESCCHAR_END:
      cmove = tty->input.length - tty->input.pos;
      break;
    case ESCCHAR_CTRLLEFT:
      wmove = -1;
      break;
    case ESCCHAR_CTRLRIGHT:
      wmove = 1;
      break;
  }

  if(cmove) { // micro-optimization, yo!
    tty_cursor_move(tty, cmove);
  }

  if(wmove) {
    word_move(tty, wmove);
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_rx - handle user interface input char
 * @param tty - TTY handle
 * @param c - input char
 */
void tty_rx(TTY* tty, int c)
{
  if(tty->child != NULL) {
    tty_rx(tty->child, c);
    return;
  }

  if (tty->flags & (IN_BRACKET_ESCAPE | IN_NUMERIC_ESCAPE)) {
      tty_handle_esc(tty, c);
  } else if (tty->flags & IN_ESCAPE) {
      if (c == '[' || c == 'O') {
          tty->flags |= IN_BRACKET_ESCAPE;
      } else {
          tty->flags &= ~(IN_ESCAPE | IN_BRACKET_ESCAPE);
      }
  } else {
      // Verify the c is valid non-extended ASCII (and thus also valid
      // UTF-8, for Rust), regardless of whether this platform's isprint()
      // accepts things above 0x7f.
      //TODO: HANdle UTF-8
      if (c >= 0x20
    #ifdef UTF8_SUPPORT
          && ( (unsigned char) c < 0x7f || (unsigned char) c >= 0x80 )
    #else
          && (unsigned char) c < 0x7f
    #endif //UTF8_SUPPORT
          ) {
          tty_handle_char(tty, c);
      } else {
          tty_handle_ctrl(tty, c);
      }
  }

  if(tty->mode.delete_later) {
    //Free
    tty_free(tty);
  }
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_putc - put char to user
 * @param tty - TTY handle
 * @param c - char to print
 * @return number of output char or -1 if error
 */
int tty_putc(TTY* tty, int c)
{
  return tty_writer(tty, (char *) &c, 1);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_puts - put string to user
 * @param tty - TTY handle
 * @param text - string to print
 * @return number of output char or -1 if error
 */
int tty_puts(TTY* tty, const char *text)
{
  return tty_writer(tty, text, (int) strlen(text));
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_printf - print text to user
 * @param tty - TTY handle
 * @param format - printf format string
 * @return - number of output char or -1 if error
 */
int tty_printf(TTY* tty, const char *format, ...)
{
  int r;
  va_list ap;
  va_start(ap, format);
  r = tty_vprintf(tty, format, ap);
  va_end(ap);
  return r;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_vprintf - print text to user
 * @param tty - TTY handle
 * @param format - printf format string
 * @param ap - args
 * @return number of output char or -1 if error
 */
int tty_vprintf(TTY* tty, const char *format, va_list ap)
{
  return vnprintf(tty_writer, tty, format, ap);
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_is_echo_mode - get echo mode
 * @param tty - TTY handle
 * @return 1/0 - echo mode ON/OFF
 */
int tty_is_echo_mode(TTY* tty)
{
  return tty->mode.echo ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_echo_mode - set echo mode
 * @param tty - TTY handle
 * @param echo - 1/0 - echo mode ON/OFF
 */
void tty_set_echo_mode(TTY* tty, int echo)
{
  tty->mode.echo = echo ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_is_password_mode - get password mode
 * @param tty - TTY handle
 * @return - 1/0 - password mode ON/OFF
 */
int tty_is_password_mode(TTY* tty)
{
  return tty->mode.password ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_password_mode - set password mode
 * @param tty - TTY handle
 * @param mode
 * @return - 1/0 - password mode ON/OFF
 */
int tty_set_password_mode(TTY* tty, int mode)
{
  tty->mode.password = mode ? 1 : 0;
  return tty->mode.password ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_is_histore_mode - get histore mode
 * @param tty - TTY handle
 * @return  - 1/0 - histore mode ON/OFF
 */
int tty_is_histore_mode(TTY* tty)
{
  return tty->mode.histore ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_hitore_mode - set histore mode
 * @param tty - TTY handle
 * @param mode 1/0 - ON/OFF
 * @return - 1/0 - histore mode ON/OFF
 */
int tty_set_histore_mode(TTY* tty, int mode)
{
  tty->mode.histore = mode ? 1 : 0;
  return tty->mode.histore ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
/**
 * @brief tty_set_interceptor - include interceptor to event handler (tty_rx)
 * @param tty - source tty
 * @param child - tty interceptor
 */
void tty_set_interceptor(TTY *tty, TTY *interceptor)
{
  tty->child = interceptor;
  tty->mode.kill_prompt = 1;
  interceptor->parent = tty;
  interceptor->print_cb = tty->print_cb;
}
/*----------------------------------------------------------------------------*/
#ifdef WIN_CONSOLE
/**Native windows console functions wrappers*/

int console_kbhit(void)
{
  DWORD cc = 0;
  HANDLE h = GetStdHandle( STD_INPUT_HANDLE );

  if (h == NULL) {
    return 0; // console not found
  }

  if(GetNumberOfConsoleInputEvents(h, &cc)) {
    return cc != 0;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
int getconchar( KEY_EVENT_RECORD *krec )
{
  DWORD cc;
  INPUT_RECORD irec;
  HANDLE h = GetStdHandle( STD_INPUT_HANDLE );

  if (h == NULL)
  {
    return 0; // console not found
  }
/*
  GetNumberOfConsoleInputEvents(h, &cc);
  if(!cc) {
    return 0;
  }
*/

  ReadConsoleInput( h, &irec, 1, &cc );
  if( irec.EventType == KEY_EVENT
      &&  irec.Event.KeyEvent.bKeyDown
      )//&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
  {
    *krec = irec.Event.KeyEvent;
    return 1;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
int console_getchar(void)
{
  static int keys[4];
  static int queue = 0;
  static int queue_size = 0;
  int c = 0;

  if(queue < queue_size) {
    c = keys[queue];
    queue ++;
    if(queue >= queue_size) {
      queue = queue_size = 0;
    }
  }

  else {
    KEY_EVENT_RECORD key;
    if(getconchar( &key )) {
      //fprintf(stderr, "key: %x code: %x\n", (unsigned) key.uChar.AsciiChar, (unsigned) key.wVirtualKeyCode);
      if(key.uChar.AsciiChar) {
#ifdef UTF8_SUPPORT
        //Convert UNICODE key char to UTF-8 sequency
        int i, s;
        char chars[8];
        s = WideCharToMultiByte(
              CP_UTF8, //CP_ACP, // code page
              0, // performance and mapping flags
              &key.uChar.UnicodeChar, // wide-character string
              1, // number of chars in string
              chars, // buffer for new string (must use vector here!)
              sizeof(chars), // size of buffer
              NULL, // default for unmappable chars
              NULL // set when default char used
              );

        if(s && s < (int) sizeof(keys)) {
          c = 0xff & (unsigned) chars[0];
          for(i = 1; i < s; i++) {
            keys[i - 1] = 0xff & (unsigned) chars[i];
          }
          queue_size = i - 1;
          queue = 0;
        }
#else
        c = key.uChar.AsciiChar;
#endif //

      } else {
        //Convert VIRTUAL keycodes to ESC sequency
        switch (key.wVirtualKeyCode) {
          case VK_LEFT:
            c = ESCCHAR;
            keys[queue_size ++] = '[';
            keys[queue_size ++] = ESCCHAR_LEFT;
            break;
          case VK_RIGHT:
            c = ESCCHAR;
            keys[queue_size ++] = '[';
            keys[queue_size ++] = ESCCHAR_RIGHT;
            break;
          case VK_UP:
            c = ESCCHAR;
            keys[queue_size ++] = '[';
            keys[queue_size ++] = ESCCHAR_UP;
            break;
          case VK_DOWN:
            c = ESCCHAR;
            keys[queue_size ++] = '[';
            keys[queue_size ++] = ESCCHAR_DOWN;
            break;
          case VK_DELETE:
            c = ESCCHAR;
            keys[queue_size ++] = '[';
            keys[queue_size ++] = ESCCHAR_DEL;
            break;
          default:
            break;
        }
      }
    }
  }

  return c;
}
/*----------------------------------------------------------------------------*/
static DWORD root_mode = 0;
/*----------------------------------------------------------------------------*/
void setup_console()
{
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hStdin, &mode);
  root_mode = mode;

  mode &= ~ENABLE_ECHO_INPUT;
  mode &= ~ENABLE_PROCESSED_INPUT;
  mode &= ~ENABLE_LINE_INPUT;

  //fprintf(stderr, "Console mode %08x\n", mode);
  SetConsoleMode(hStdin, mode);
}
/*----------------------------------------------------------------------------*/
void restore_console(void)
{
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  SetConsoleMode(hStdin, root_mode);
}
/*----------------------------------------------------------------------------*/
#endif //WIN_CONSOLE

#ifdef UNIX_CONSOLE

int console_getchar(void)
{
  return getchar();
}
/*----------------------------------------------------------------------------*/
int console_kbhit(void)
{
  struct timeval tv = { 0L, 0L };
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  return select(1, &fds, NULL, NULL, &tv);
}
/*----------------------------------------------------------------------------*/
static struct termios saved_term;
/*----------------------------------------------------------------------------*/
void setup_console(void)
{
  struct termios term;
  if (tcgetattr(STDIN_FILENO, &saved_term) < 0) {
    perror("tcgetattr");
  }

  term = saved_term;

  term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  term.c_oflag &= ~(OPOST);
  term.c_cflag |= CS8;
  term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  term.c_cc[VMIN] = 0;    term.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
    perror("tcsetattr");
  }
}
/*----------------------------------------------------------------------------*/
void restore_console(void)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term);
}
/*----------------------------------------------------------------------------*/
#endif

/*----------------------------------------------------------------------------*/
#if defined(WIN_CONSOLE) || defined(UNIX_CONSOLE)
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

int console_isatty(void)
{
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}
#endif
/*----------------------------------------------------------------------------*/
