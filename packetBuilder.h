
#ifndef PACKETBUILDER_H
#define PACKETBUILDER_H
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <string.h>
using namespace std;
const int SEQMAX = 30720;

class Pair{
public:
  Pair(char* buf, int len);
  char* getData();
  int getLength();
private:
  char* data;
  int length;
};

class Packet{
public:
  Packet(char* buffer, int len);
  Packet(int len, char* built);
  Packet(char* buffer, int len, unsigned short seq, unsigned short ack, unsigned short wNum, bool a,
    bool s, bool f);
//  ~Packet();
  void cleanupPacket();
  void buildPacketArr();
  void buildPacketFromBuf(char* buf, int length);
  Packet decodePkt(char* encodedPkt);
  void debugPacket();
  //Getters
  unsigned short getSeqNum();
  unsigned short getAckNum();
  char* getData();
  char* getBuiltPacket();
  int getLength();
  bool getAck();
  bool getSyn();
  bool getFin();
  clock_t getSentAt();
  bool getSent();
  //Setters
  void setAck(bool val);
  void setSyn(bool val);
  void setFin(bool val);
  void setSeqNum(unsigned short seq);
  void setAckNum(unsigned short ack);
  void setSent();//sets the sent time to right now.
  void setNotSent();

private:
  char* data;
  unsigned short seqNum;
  unsigned short ackNum;
  unsigned short window;
  bool ack, syn, fin;
  int length; //length of the data ONLY, not header
  clock_t sentAt; //time it was sent at. used for retransmission, -1 if not sent.
  bool sent;
  char* builtPacket;
};

vector<Pair> readFile(char* filename);
vector<Packet> connectionSetup();
void debugVector(vector<Packet> vec);

#endif
