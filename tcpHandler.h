#ifndef TCPHANDLER_H
#define TCPHANDLER_H
  #include <vector>
  #include <errno.h>
  #include <stdio.h>
  #include <string>
  #include <stdlib.h>
  #include <fstream>
  #include <ctime>
  #include <math.h>
  #include "packetBuilder.h"

  const int WINDOWCAP = 15360; //set to bytes at some point along with the rest of the stuff.
  static ifstream sendFile;
  static char* fileName;
  static vector<Packet> notAcked;
  static unsigned long idx = -1;
  static int windowSize = 1024;
  static int lastSeq = -1;
  static bool isDone = false;
  static bool gotAllData = false;
  static unsigned short lastAck;

  static int ssthreash = 15360;
  static int cwnd = 1024;
  static bool isSS = true;
  static bool timedOut = false;

  static int duplicateCount = -1;

  //Expected Seq number for duplicate ACKs
  static int exp_seq = 0;
  static int cum_seq = 0;

  //Client window
  static vector<Packet> window;
  static bool outOfOrder = false;
  static vector<int> array1;
  const int client_win = 15360;
  static bool syn_ack = false;
  static bool fin = false;
  static unsigned int final_seq;
  static unsigned int final_ack;
  static unsigned int last_seq;
  static int to_count = 0;
  static unsigned int fin_seq;

  //Implement this
  static int fin_in_buffer = 0;

  using namespace std;

vector<Packet> sortVec(vector<Packet> v); 

  void fillDataVec(char* filename);
  vector<Packet> serverReceived(vector<Packet> prevQueue, char* buf, int length);
  Packet clientReceived(char* buf, int length, ostream& fd);
  Packet startClient();
  void addToUnacked(Packet p);
  vector<Packet> createQueue(vector<Packet> prevQueue, int size); //set size to 1 when syn or fin.
  vector<Packet> removeAckedPkt(vector<Packet> prevQueue, Packet p);
  void initStuff(char* file);
  void timeoutCA();
#endif
