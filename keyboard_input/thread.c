/* thread.c --- 
 * Filename: thread.c
 * Description: 
 * Author: balaji
 * Email:  sjbalajimdu@gmail.com
 * Organization:  IIT Madras
 * Created: Sat Mar 16 21:24:40 2013 (+0530)
 * Last-Updated: Sun Mar 17 08:09:17 2013 (+0530)
 *           By: balaji
 *     Update #: 17
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}


void *start_server(void *ptr);

main()
{
  pthread_t thread1, thread2;
  char *message1 = "5000";
  char *message2 = "5001";
  int  iret1, iret2;
  int iter1;

  /* Create independent threads each of which will execute function */

  iret1 = pthread_create( &thread1, NULL, start_server, (void*) message1);
  iret2 = pthread_create( &thread2, NULL, start_server, (void*) message2);
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */

  pthread_join( thread1, NULL);
  pthread_join( thread2, NULL); 

  printf("Thread 1 returns: %d\n",iret1);
  printf("Thread 2 returns: %d\n",iret2);
  exit(0);
}

void *start_server( void *ptr )
{
  char *message;
  message = (char *) ptr;
  printf("%s \n", message);

  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  portno = atoi(message);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);

#define DEBUG
#ifdef DEBUG
    if (n < 0)
      error("ERROR in recvfrom");
    printf("Received %s\n",buf);
#endif  
  }
}
