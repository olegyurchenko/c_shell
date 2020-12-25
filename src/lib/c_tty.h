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
* (C) Oleg Yurchenko, Kiev, Ukraine 2020.<br>
* started 04.12.2020 11:33:36<br>
* @pkgdoc c_tty
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef C_TTY_H_1607074416
#define C_TTY_H_1607074416
/*----------------------------------------------------------------------------*/
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define TTY_DEFAULT_INPUT_SIZE 256
#define TTY_DEFAULT_HISTORE_SIZE 512
#define TTY_DEFAULT_PROMPT_SIZE 80

/**
 * @brief TTY - TTY handle
 */
typedef struct TTY_TAG TTY;
typedef int (*TTY_PRINT_CB)(void *arg, int c);
typedef int (*TTY_ENTER_CB)(void *arg, const char *text, unsigned size);
typedef int (*TTY_OVERFLOW_CB)(void *arg, const char *text, unsigned size);
typedef int (*TTY_PROMPT_CB)(void *arg, char *buffer, unsigned buffer_size);
typedef int (*TTY_CTRL_C_CB)(void *arg, const char *text, unsigned size);
typedef int (*TTY_TAB_CB)(void *arg, const char *text, unsigned size);
typedef const char* (*TTY_KW_CB)(void *arg, unsigned index);

/**
 * @brief tty_alloc - memory allocate and init TTY handle
 * @param input_buffer_size - size of input buffer. -1 - default size
 * @param histore_size - size of histore buffer size. -1 - default size
 * @param prompt_size - size of prompt buffer size. -1 - default size
 * @return pointer to handle. NULL if memory allocate error
 */
TTY* tty_alloc(int input_buffer_size, /*=-1*/ int histore_size /*=-1*/, int prompt_size /*=-1*/);
/**
 * @brief tty_free - memory free
 * @param tty - handle
 */
void tty_free(TTY* tty);
/**
 * @brief tty_delete_later - mark to delete (use inside callback where not allowed call tty_free)
 * @param tty - handle
 */
void tty_delete_later(TTY* tty);
/**
 * @brief tty_set_print_cb - set printing callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_print_cb(TTY* tty, TTY_PRINT_CB cb, void *cb_arg);
/**
 * @brief tty_set_enter_cb - set enter text callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_enter_cb(TTY* tty, TTY_ENTER_CB cb, void *cb_arg);
/**
 * @brief tty_set_overflow_cb - set buffer overflow callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_overflow_cb(TTY* tty, TTY_OVERFLOW_CB cb, void *cb_arg);
/**
 * @brief tty_set_prompt_cb - set get prompt callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_prompt_cb(TTY* tty, TTY_PROMPT_CB cb, void *cb_arg);
/**
 * @brief tty_set_prompt - set simple prompt text
 * @param tty - TTY handle
 * @param text - text of prompt ala "#"
 */
void tty_set_prompt(TTY* tty, const char *text);
/**
 * @brief tty_set_ctrlc_c_cb - set CTRL+C callback
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_ctrlc_c_cb(TTY* tty, TTY_CTRL_C_CB cb, void *cb_arg);
/**
 * @brief tty_set_tab_cb - set TAB callback (for Tab Completion)
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_tab_cb(TTY* tty, TTY_TAB_CB cb, void *cb_arg);
/**
 * @brief tty_set_keywords - set keywords for internal Tab Completion.
 * @param tty - TTY handle
 * @param kwords - point to keywords area
 * @param size - sizeo of keyword area
 */
void tty_set_keywords(TTY* tty, const char **kwords, unsigned size);
/**
 * @brief tty_set_kwords_cb - set kwords callback for (for Tab Completion)
 * @param tty - TTY handle
 * @param cb - callback
 * @param cb_arg - arbitrary argument to pass as callback argument
 */
void tty_set_kwords_cb(TTY* tty, TTY_KW_CB cb, void *cb_arg);
/**
 * @brief tty_rx - handle user interface input char
 * @param tty - TTY handle
 * @param c - input char
 */
void tty_rx(TTY* tty, int c);
/**
 * @brief tty_putc - put char to user
 * @param tty - TTY handle
 * @param c - char to print
 * @return number of output char or -1 if error
 */
int tty_putc(TTY* tty, int c);
/**
 * @brief tty_puts - put string to user
 * @param tty - TTY handle
 * @param text - string to print
 * @return number of output char or -1 if error
 */
int tty_puts(TTY* tty, const char *text);
/**
 * @brief tty_printf - print text to user
 * @param tty - TTY handle
 * @param format - printf format string
 * @return - number of output char or -1 if error
 */
int tty_printf(TTY* tty, const char *format, ...);
/**
 * @brief tty_vprintf - print text to user
 * @param tty - TTY handle
 * @param format - printf format string
 * @param ap - args
 * @return number of output char or -1 if error
 */
int tty_vprintf(TTY* tty, const char *format, va_list ap);
/**
 * @brief tty_is_echo_mode - get echo mode
 * @param tty - TTY handle
 * @return 1/0 - echo mode ON/OFF
 */
int tty_is_echo_mode(TTY* tty);
/**
 * @brief tty_set_echo_mode - set echo mode
 * @param tty - TTY handle
 * @param mode - 1/0 - echo mode ON/OFF
 */
void tty_set_echo_mode(TTY* tty, int mode);
/**
 * @brief tty_is_password_mode - get password mode
 * @param tty - TTY handle
 * @return - 1/0 - password mode ON/OFF
 */
int tty_is_password_mode(TTY* tty);
/**
 * @brief tty_set_password_mode - set password mode
 * @param tty - TTY handle
 * @param mode 1/0 - ON/OFF
 * @return - 1/0 - password mode ON/OFF
 */
int tty_set_password_mode(TTY* tty, int mode);
/**
 * @brief tty_is_histore_mode - get histore mode
 * @param tty - TTY handle
 * @return  - 1/0 - histore mode ON/OFF
 */
int tty_is_histore_mode(TTY* tty);
/**
 * @brief tty_set_hitore_mode - set histore mode
 * @param tty - TTY handle
 * @param mode 1/0 - ON/OFF
 * @return - 1/0 - histore mode ON/OFF
 */
int tty_set_histore_mode(TTY* tty, int mode);
/**
 * @brief tty_del - delete char on input position
 * @param tty - handle
 */
void tty_del(TTY *tty);
/**
 * @brief tty_bs - delete char before input position
 * @param tty - handle
 */
void tty_bs(TTY *tty);
/**
 * @brief tty_ins - insert char char after input position
 * @param tty - handle
 * @param c - char
 */
void tty_ins(TTY *tty, char c);
/**
 * @brief tty_cursor_move - Move the tty cursor. This moves the insertion point.
 * @param tty - TTY handle
 * @param n - position delta
 */
void tty_cursor_move(TTY *tty, int n);
/**
 * @brief tty_input_length - get TTY user input string length
 * @param tty - TTY handle
 * @return - input string length
 */
int tty_input_length(TTY *tty);
/**
 * @brief tty_input_position - get cursor position
 * @param tty - TTY handle
 * @return - cursor position
 */
int tty_input_position(TTY *tty);
/**
 * @brief tty_input_buffer - get TTY input buffer
 * @param tty - TTY handle
 * @return pointer to buffer
 */
const char *tty_input_buffer(TTY *tty);
/**
 * @brief tty_restore - restore user screen
 * @param tty - handle
 */
void tty_restore(TTY *tty);

//void tty_gets(TTY *tty, char *dst, unsigned size, int password, int no_echo);
/**
 * @brief tty_set_interceptor - include interceptor to event handler (tty_rx)
 * @param tty - source tty
 * @param child - tty interceptor
 */
void tty_set_interceptor(TTY *tty, TTY *interceptor);

#if defined(WIN_CONSOLE) || defined(UNIX_CONSOLE)
/**Native windows & unix console functions wrappers*/
/**
 * @brief console_isatty - test whether a STDIN, STDOUT refers to a terminal
 * @return returns 1 if STDIN, STDOUT referring to a terminal; otherwise 0 is returned
 */
int console_isatty(void);

/**
 * @brief console_getchar read console input char
 * @return != 0 - ASCII code or ESC sequency
 */
int console_getchar(void);
/**
 * @brief console_kbhit - check if keyboard queue is not empty
 * @return != 0 if keyboard queue is not empty
 */
int console_kbhit(void);
/**
 * @brief setup_console - set up console to TTY mode (no echo, ...)
 */
void setup_console(void);
/**
 * @brief restore_console - restore console to initial mode
 */
void restore_console(void);
#endif //WIN_CONSOLE



#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*C_TTY_H_1607074416*/

