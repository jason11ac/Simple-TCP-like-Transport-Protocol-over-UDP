#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "tcpHandler.h"

#define BUFSIZE 1032

using namespace std;

//global variables needed for a connection
unsigned short expAck;
vector<Packet> windowSrv;

int main(int argc, char **argv) {
  int sockfd; // socket descriptor
  int portno; // port to listen on
  int clientlen; // byte size of client's address
  struct sockaddr_in serveraddr; // server's address
  struct sockaddr_in clientaddr; // client's address
  struct hostent *hostp; // client host info
  char *hostaddrp; // dotted decimal host addr string
  int optval; // flag value for setsockopt
  int n; // message byte size


  int numPacketsSent = 0;


  // check command line arguments
  if (argc != 3) {
    fprintf(stderr, "usage: %s PORT-NUMBER FILE-NAME\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  // socket: create the parent socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    perror("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));


  // build the server's Internet address
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);


  // bind: associate the parent socket with a port
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    perror("ERROR on binding");

  //fillDataVec(argv[2]);
  // main loop: wait for a datagram, then echo it
  clientlen = sizeof(clientaddr);

  clock_t startTime;
  initStuff(argv[2]);
  while (1) {
    char* buf = new char[BUFSIZE]; // message buffer
    //printf("In here\n");
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, (unsigned int*)  &clientlen);
    //printf("Past the recv\n");
    if (n < 0)
      perror("ERROR in recvfrom");

    Packet clientMsg = Packet(n, buf);
    //DEBUG STUFF
    //cout << endl << endl << endl << "**************************************" << endl;
    //cout << "////////RECEIVED PACKET//////\n";
    //clientMsg.debugPacket();
    //cout << endl << "BEFORE THE THING" << endl;
    //debugVector(windowSrv);
    //END DEBUG STUFF
    windowSrv = serverReceived(windowSrv, buf, n); //creates a window of packets

    //MORE DEBUG STUFF
    //cout << endl << "AFTER THE THING" << endl;
    //debugVector(windowSrv);
    //END DEBUG STUFF

    // gethostbyaddr: determine who sent the datagram
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      perror("ERROR on inet_ntoa\n");
    //  printf("server received datagram from %s (%s)\n",
    // hostp->h_name, hostaddrp);
    //  printf("server received %lu/%d bytes: %s\n", strlen(buf), n, buf);

    //+8 because of header, send a bunch of packets at once
    for(int i = 0; i < windowSrv.size(); i++){
      double timeout = (double)(clock() - windowSrv[i].getSentAt())/(double)CLOCKS_PER_SEC;
      if((windowSrv[i].getSent()==false) || (timeout >= .5)){ //change this.
        n = sendto(sockfd, windowSrv[i].getBuiltPacket(), windowSrv[i].getLength() + 8, 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          perror("ERROR in sendto");
        else{
	  cout << "Sending packet " << windowSrv[i].getSeqNum() << " " << cwnd << " " << ssthreash;
          if(windowSrv[i].getSentAt() != 0 && windowSrv[i].getSent() == false){
            cout << " Retransmission";
          }
          if(windowSrv[i].getSyn()){
            cout << " SYN";
          }
          if(windowSrv[i].getFin()){
            cout << " FIN";
          }
          cout << endl;
          windowSrv[i].setSent();
   
       //cout << endl << "PACKET SENT:" <<endl;
	  //          windowSrv[i].debugPacket();
          //window[i].cleanupPacket();
        }
        if(timeout >= .5){
          timeoutCA();
        }
      }
    }
    //  printf("Past for\n");
    if(clientMsg.getFin()){
      double t0 = clock();
      while ((double) (clock()-t0)/CLOCKS_PER_SEC < 1) {}
      close(sockfd);
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      if (sockfd < 0)
        perror("ERROR opening socket");

      /* setsockopt: Handy debugging trick that lets
       * us rerun the server immediately after we kill it;
       * otherwise we have to wait about 20 secs.
       * Eliminates "ERROR on binding: Address already in use" error.
       */
      optval = 1;
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		 (const void *)&optval , sizeof(int));


      // build the server's Internet address
      bzero((char *) &serveraddr, sizeof(serveraddr));
      serveraddr.sin_family = AF_INET;
      serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
      serveraddr.sin_port = htons((unsigned short)portno);


      // bind: associate the parent socket with a port
      if (bind(sockfd, (struct sockaddr *) &serveraddr,
	       sizeof(serveraddr)) < 0)
        perror("ERROR on binding");
      initStuff(argv[2]);
      windowSrv.clear();
      //printf("Done in server 1234\n");
    }
  }
}
