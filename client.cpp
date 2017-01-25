#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <poll.h>

#include "packetBuilder.h"
#include "tcpHandler.h"

#define BUFSIZE 1032

using namespace std;

vector<Packet> win;
ofstream outfile ("received.data", ofstream::out | ofstream::trunc);
bool retransmit = false;
clock_t startTime;



struct pollfd pfd;
int ret;


int main(int argc, char **argv) {
  int sockfd, portno, n;
  socklen_t serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname;
  int fin = 0;
  bool syn_not_sent = true;

  /* check command line arguments */
  if (argc != 3) {
    fprintf(stderr,"usage: %s SERVER-HOST-OR-IP PORT-NUMBER\n", argv[0]);
    exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  /* socket: create the socket */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    perror("ERROR opening socket");

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host as %s\n", hostname);
    exit(0);
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
	(char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);
  char* buf;
  //Packet endConn = Packet(NULL, 0, , 0, client_win, true, false, true); //Fin
  Packet startConn = Packet(NULL, 0, 0, 0, client_win, false, true, false); //Syn
  buf = startConn.getBuiltPacket();
  int l = startConn.getLength()+8;
  serverlen = sizeof(serveraddr);
  //Send Syn packet
  n = sendto(sockfd, buf, l, 0, (const struct sockaddr*) &serveraddr, serverlen);
  if (n < 0)
    perror("ERROR in sendto");

  while (1) {
    buf = new char[BUFSIZE]; // ACK buffer
    bzero(buf, BUFSIZE);
    
  top:
    
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    ret = poll(&pfd, 1, 500);
    switch (ret) {
    case -1:
      perror("Error with poll\n");
      break;
    case 0:
      //Timeout
      if (fin) {
	to_count++;
	if (to_count == 2) {
	  close(sockfd); //Close connection
	  //printf("Closed connection\n");
	  outfile.close();
	  exit(10);
	}
	goto top;
	
      } else if (syn_not_sent) {
	//printf("Have to retransmit syn\n");
	startConn = Packet(NULL, 0, 0, 0, client_win, false, true, false);
	buf = startConn.getBuiltPacket();
	l = startConn.getLength()+8;
	n = sendto(sockfd, buf, l, 0, (const struct sockaddr*) &serveraddr, serverlen);
	cout << "Sending packet 0 Retransmission SYN" << endl;
	if (n < 0)
	  perror("Error in sendto for syn retrans\n");
      
      	goto top;
      } else
	//printf("timeout. sending normal packet\n");
	; //do nothing. Server will take care of it
      break;
    default:
      // Received packet
      syn_not_sent = false;
      n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)  &serveraddr, (unsigned int*) &serverlen);
      if (n < 0)
	perror("Error in recvfrom");
    }

    Packet temp = Packet(n, buf);
    Packet clientMSG = clientReceived(buf, n, outfile); //Format the packet
    cout << "Receiving packet " << temp.getSeqNum() << endl;

    //printf("clientMSG.getLength(): %d\n", clientMSG.getLength());
    /*
    if (fin==1 && temp.getAck()) {
      close(sockfd); //Close connection
      printf("Closed connection\n");
      //cout << "///////RECEIVED PKT FIN///////" << endl;
      //temp.debugPacket();
      //cout << "///////Sent PKT That isn't really sent at all///////" << endl;
      //clientMSG.setAck(true);
      //clientMSG.debugPacket();
      //cout << "///////END PKT STUFF" << endl;
      outfile.close();
      exit(10);
      }*/

    /*
    if (temp.getFin()) { //Close the connection
      printf("Connection closed prev");
      fflush(stdout);
      fin = 1;
      final_ack = clientMSG.getSeqNum() + 1;
      final_seq = clientMSG.getAckNum();
      }*/
    //cout << "before " << fin_in_buffer << endl;
    if (fin_in_buffer && (exp_seq == window[0].getSeqNum())) {
	//If only fin is left in buffer
	//printf("Connection closed from buffer fin");
	fflush(stdout);
	fin = 1;
	final_ack = window[0].getSeqNum() + 1;
	final_seq = window[0].getAckNum();
	Packet endConn = Packet(NULL, 0, final_seq, final_ack, client_win, true, false, true);
        buf = endConn.getBuiltPacket();
        l = endConn.getLength()+8;
        n = sendto(sockfd, buf, l, 0, (const struct sockaddr*) &serveraddr, serverlen);
	cout << "Sending packet " << final_ack << " FIN" << endl;
        if (n < 0)
	  perror("Error in sendto for fin retrans\n");
    } else if (temp.getFin() && !fin_in_buffer) {
      //If only fin is left in buffer
      //printf("Connection closed from normal fin");
      fflush(stdout);
      fin = 1;
      final_ack = temp.getSeqNum() + 1;
      final_seq = temp.getAckNum();
      Packet endConn = Packet(NULL, 0, final_seq, final_ack, client_win, true, false, true);
      buf = endConn.getBuiltPacket();
      l = endConn.getLength()+8;
      n = sendto(sockfd, buf, l, 0, (const struct sockaddr*) &serveraddr, serverlen);
      cout <<"Sending packet " << final_ack << " FIN" << endl;
      if (n < 0)
	perror("Error in sendto for fin retrans\n");
    }

    //debug stuff
    //cout << "///////RECEIVED PKT///////" << endl;
    //temp.debugPacket();
    //cout << "///////Sent PKT///////" << endl;
    //clientMSG.debugPacket();
    //cout << "///////END PKT STUFF" << endl;
     n = sendto(sockfd, clientMSG.getBuiltPacket(), clientMSG.getLength()+8, 0, (const struct sockaddr*) &serveraddr, serverlen);
    if (n < 0)
      perror("ERROR in sendto here");

    //If recieved fin, send fin ack back
    /*
    if (fin) {
      printf("Sending fin\n");
      Packet endConn = clientMSG;
      buf = endConn.getBuiltPacket();
      int l = endConn.getLength()+8;
      //Send Fin packet
      n = sendto(sockfd, buf, l, 0, (const struct sockaddr*) &serveraddr, serverlen);
      if (n < 0)
	perror("Error sending fin using sendto");
	}*/
    }
}
