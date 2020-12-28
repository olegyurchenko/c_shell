#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
typedef int socklen_t;

static void bzero(void *s, size_t  n ) {
  memset(s, 0, n);
}
#endif


#ifdef UNIX
  #include <unistd.h>
  #include <netinet/in.h>

  typedef int SOCKET;
  #define closesocket(sd) close(sd)
#endif


#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define PORT 4423
#define MAX_CONNECTIONS 16

SHELL_DATA connections[MAX_CONNECTIONS];
int connection_count = 0;

void on_new_connection(SHELL_DATA *);
void on_rx(SHELL_DATA *);
void on_close(SHELL_DATA *);


static void sock_init(void)
{
#ifdef WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;

  wVersionRequested = MAKEWORD( 2, 0 );

  err = WSAStartup(wVersionRequested, &wsaData);
#endif /*WIN32*/
}
/*----------------------------------------------------------------------------*/
int main()
{
  int listenfd, connfd, nready, maxfdp1;
  fd_set rset, eset;
  socklen_t len;
  struct sockaddr_in cliaddr, servaddr;

  sock_init();
  /* create listening TCP socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // binding server addr structure to listenfd
  bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if(listen(listenfd, 10) < 0) {
    fprintf(stderr, "listen error\n");
    return 1;
  }

  fprintf(stdout, "Connection opened on port %d\n", PORT);


  for (;;) {
    int i, j;

    // clear the descriptor set
    FD_ZERO(&rset);
    // clear the descriptor set
    FD_ZERO(&eset);

    // set listenfd in readset
    FD_SET(listenfd, &rset);
    FD_SET(listenfd, &eset);

    maxfdp1 = listenfd;
    for(i = 0; i < connection_count; i++) {
      maxfdp1 = max(maxfdp1, connections[i].fd);
      FD_SET(connections[i].fd, &rset);
      FD_SET(connections[i].fd, &eset);
    }
    maxfdp1 += 1;

    // select the ready descriptor
    nready = select(maxfdp1, &rset, NULL, &eset, NULL);
    if(nready <= 0) {
      fprintf(stderr, "select error\n");
      return 1;
    }

    for(i = 0; i < connection_count; i++) {
      if (FD_ISSET(connections[i].fd, &rset)) {
        //Handle RX event
        on_rx(&connections[i]);
      }

      if(FD_ISSET(connections[i].fd, &eset) || connections[i].terminated) {
        //Handle ERROR or EXIT event
        on_close(&connections[i]);
        closesocket(connections[i].fd);
        fprintf(stdout, "Connection %d closed\n", i);

        //Remove connection
        for(j = i; j < connection_count - 1; j++) {
          connections[j] = connections[j + 1];
        }
        connection_count --;
        i --;
      }
    }

    // if tcp socket is readable then handle
    // it by accepting the connection
    if (FD_ISSET(listenfd, &rset)) {
      len = sizeof(cliaddr);
      connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
      if(connfd > 0) {
        //Hew connection
        if(connection_count < MAX_CONNECTIONS - 1) {
          memset(&connections[connection_count], 0, sizeof (connections[0]));
          connections[connection_count].fd = connfd;
          fprintf(stdout, "New connection %d established\n", connection_count);
          on_new_connection(&connections[connection_count]);
          connection_count ++;

        } else {
          closesocket(connfd);
        }
      }
    }
  }
}
/*----------------------------------------------------------------------------*/
static int print_cb(void *arg, int c)
{
  SHELL_DATA *conn;
  char buffer;

  conn = (SHELL_DATA *) arg;
  if (c == '\n') {
    buffer = '\r';
    if(send(conn->fd, &buffer, 1, 0) < 1) {
      conn->terminated = 1;
      return -1;
    }
  }

  buffer = (char) c;
  if(send(conn->fd, &buffer, 1, 0) <= 0) {
    conn->terminated  = 1;
    return -1;
  }
  return 1;
}
/*----------------------------------------------------------------------------*/
static int step_cb(void *arg, int argc, char **argv)
{
  SHELL_DATA *data;
  int r = SHELL_OK;

  (void) argc;
  (void) argv;

  data = (SHELL_DATA *) arg;

/*Wathdog
 * For such a wretched server, I had to do a watchdog
 * so that none of the users hung up the system in an endless loop
 * such a
 * while thue; do done
*/

  data->watchdog ++;
  if(data->watchdog > 1000) {
    shell_printf(data->sh, "Wachdog triggered\n");
    fprintf(stderr, "Wachdog triggered\n");
    r = SHELL_STACK_ERROR;
  }

  return r;
}
/*----------------------------------------------------------------------------*/
void on_new_connection(SHELL_DATA *data)
{
  char telnet_settings[] = {
    0xFF, 0xFD, 0x01,
    0xFF, 0xFD, 0x1F,
    0xFF, 0xFB, 0x01,
    0xFF, 0xFB, 0x03,

    0x1B, 0x5B, 0x31, 0x48,
    0x1B, 0x5B, 0x32, 0x4A
  };

  data->tty = tty_alloc(TTY_DEFAULT_INPUT_SIZE, TTY_DEFAULT_HISTORE_SIZE, TTY_DEFAULT_PROMPT_SIZE);
  //TODO: check for NULL
  tty_set_print_cb(data->tty, print_cb, data);

  data->sh = shell_alloc();
  //TODO: check for NULL
  shell_set_print_cb(data->sh, print_cb, data);
  shell_set_step_cb(data->sh, step_cb, data);

  //Write telnet client settings
  send(data->fd, telnet_settings, sizeof (telnet_settings), 0);
  //For read telnet client answer
  data->telnet_settings = 1;



  if(shell_init(data)) {
    //Error
    data->terminated  = 1;
  }
}
/*----------------------------------------------------------------------------*/
void on_rx(SHELL_DATA *data)
{
  char buffer[1024];
  int i, len, c;

  len = recv(data->fd, buffer, sizeof(buffer), 0);
  if(len <= 0) {
    data->terminated = 1;
    return;
  }

  if(data->telnet_settings) {
    data->telnet_settings = 0;
    return;
  }

  data->watchdog = 0; //watchdog reset

  for(i = 0; i < len; i++) {
    c = buffer[i];
    if(c == '\n') {
      continue; //win telnet ?
    }

    if(c == '\r') {
      c = '\n';
    }
    tty_rx(data->tty, c);
  }
}
/*----------------------------------------------------------------------------*/
void on_close(SHELL_DATA *data)
{
  shell_destroy(data);
  if(data->tty != NULL) {
    tty_free(data->tty);
  }
  if(data->sh != NULL) {
    shell_free(data->sh);
  }
}
/*----------------------------------------------------------------------------*/
