/*----------------------------------------------------------------------------*/
/**
* @pkg loop_buff
*/
/**
* Loop buffer module.
*
* Loop buffer for use as FIFO for input/output.<br>
* (C) DDA team, Kiev, Ukraine 2012.<br>
* started 28.11.2012  9:41:01<br>
* @pkgdoc loop_buff
* @author oleg
* @version 0.01 
*/
/*----------------------------------------------------------------------------*/
#ifndef LOOP_BUFF_H_1354088461
#define LOOP_BUFF_H_1354088461
/*----------------------------------------------------------------------------*/
typedef struct
{
  char *buffer;
  unsigned size;
  unsigned head;
  unsigned tail;
} LOOP_BUFFER;
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/**Loop buffer init*/
void lb_init(LOOP_BUFFER *b, char *buffer, unsigned buffer_size);
/**Return data size in buffer*/
unsigned lb_size(LOOP_BUFFER *b);
/**Return free size in buffer*/
unsigned lb_free(LOOP_BUFFER *b);
/**Get data pointer at index*/
char *lb_at(LOOP_BUFFER *b, unsigned index);
/**Put byte to buffer. Return 0 if buffer full*/
int lb_push(LOOP_BUFFER *b, char c);
/**Put size count bytes to buffer. Return 0 if buffer full*/
int lb_push_buffer(LOOP_BUFFER *b, const void *buffer, unsigned size);
/**Get byte from buffer. Return 0 if buffer empty*/
int lb_pop(LOOP_BUFFER *b, char *c);
/**Get size count bytes from buffer. Buffer may be NULL. Return 0 if buffer empty*/
int lb_pop_buffer(LOOP_BUFFER *b, void *buffer, unsigned size);

/**Clear buffer*/
void lb_clear(LOOP_BUFFER *b);


#ifdef __cplusplus
} //extern "C"
#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#endif /*LOOP_BUFF_H_1354088461*/

