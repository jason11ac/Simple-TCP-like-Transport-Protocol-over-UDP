
#include "packetBuilder.h"

using namespace std;

/*
Pair stuff
*/

Pair::Pair(char* buf, int len){
  data = new char[len];
  memcpy(data, buf, len);
  length = len;
}

int Pair::getLength(){
  return length;
}

char* Pair::getData(){
  return data;
}

/*
 *Packet Contructors
 */

/*
 *This is meant for building the packet from a buffer received from a file. The
 * reason we don't just build the whole thing here is because we often will need
 *to change things, so the builtPacket variable will not be known at this time.
 */

Packet::Packet(char* buffer, int len){
  seqNum = 0;
  ackNum = 0;
  window = 1024;
  length = len;
  ack = false;
  syn = false;
  fin = false;
  sent = false;
  if(length > 0){
    data = new char[length];
    for(int i = 0; i < length; i++){
      data[i] = buffer[i];
    }
  }
  else{
    data = NULL;
  }
  builtPacket = NULL;
}

/*
 *This is meant for building a packet from the packet it receives from the
 *network. It can fill in all fields immediately.
 */
Packet::Packet(int len, char* built){
  window = 1024;
  length = len - 8;
  if(len > 0){
    builtPacket = new char[len];
    for(int i = 0; i < len; i++){
      builtPacket[i] = built[i];
    }
  }
  memcpy(&seqNum, builtPacket, 2);
  memcpy(&ackNum, builtPacket+2, 2);
  memcpy(&window, builtPacket+4, 2);
  unsigned short flags = 0;
  memcpy(&flags, builtPacket+6, 2);
  ack = ((flags&4)==4)?true:false;
  syn = ((flags&2)==2)?true:false;
  fin = ((flags&1)==1)?true:false;
  sent = false;
  if(length > 0){
    data = new char[length];
    memcpy(data, builtPacket+8, length);
  }
  else{
    data = NULL;
  }
}

Packet::Packet(char* buffer, int len, unsigned short sequenceNumber, unsigned short ackNumber, unsigned short wNum, bool a, bool s, bool f){
  seqNum = sequenceNumber % 30720;
  ackNum = ackNumber % 30720;
  window = wNum;
  length = len;
  ack = a;
  syn = s;
  fin = f;
  sent = false;
  sentAt = 0;
  if(length > 0){
    data = new char[length];
    for(int i = 0; i < length; i++){
      data[i] = buffer[i];
    }
  }
  else{
    data = NULL;
  }
  
  char* packetArr = new char[(length + 8)];
  memcpy(packetArr, &seqNum, 2);
  memcpy(packetArr+2, &ackNum, 2);
  memcpy(packetArr+4, &window, 2);
  unsigned short flags = 0;
  if(ack){
    flags = flags|4;
  }
  if(syn){
    flags = flags|2;
  }
  if(fin){
    flags = flags|1;
  }
  memcpy(packetArr+6, &flags, 2);
  if(length > 0)
    memcpy(packetArr+8, data, length);
  builtPacket = packetArr;
}

/*
 *Packet Destructor


Packet::~Packet(){
  if(data != nullptr){
    cout << "Data " << endl;
    printf("Pointer: %p\n", data);
    delete [] data;
  }
  if(builtPacket != nullptr){
    cout << "Packet" << endl;
    delete [] builtPacket;
  }
}
*/

/*
 *Packet getters for relevant variables
 */

unsigned short Packet::getSeqNum(){
  return seqNum;
}

unsigned short Packet::getAckNum(){
  return ackNum;
}

char* Packet::getData(){
  return data;
}

char* Packet::getBuiltPacket(){
  return builtPacket;
}

int Packet::getLength(){
  return length;
}

bool Packet::getAck(){
  return ack;
}

bool Packet::getSyn(){
  return syn;
}

bool Packet::getFin(){
  return fin;
}

clock_t Packet::getSentAt(){
  return sentAt;
}

bool Packet::getSent(){
  return sent;
}

/*
 *Packet setters for relevant variables
 */

void Packet::setAck(bool val){
  ack = val;
}

void Packet::setSyn(bool val){
  ack = val;
}

void Packet::setFin(bool val){
  fin = val;
}

void Packet::setSeqNum(unsigned short seq){
  seqNum = seq %30720;
}

void Packet::setAckNum(unsigned short ack){
  ackNum = ack %30720;
}

void Packet::setNotSent(){
  sent = false;
}

void Packet::setSent(){
  sentAt = clock();
  sent = true;
}

/*
 *Create the builtPacket variable containing the packet to send over the
 *network. When sending over the actual network, take that and then add 8
 *to the length to ensure that you include the whole header with it.
 */

void Packet::buildPacketArr(){
  //section that takes care of the header
  char* packetArr = new char[(length + 8)];
  memcpy(packetArr, &seqNum, 2);
  memcpy(packetArr+2, &ackNum, 2);
  memcpy(packetArr+4, &window, 2);
  unsigned short flags = 0;
  if(ack){
    flags = flags|4;
  }
  if(syn){
    flags = flags|2;
  }
  if(fin){
    flags = flags|1;
  }
  memcpy(packetArr+6, &flags, 2);
  memcpy(packetArr+8, data, length);
  builtPacket = packetArr;
}

/*
 *Takes a buffer containing a builtPacket with header and stuff and the length
 *of the data only from the packet and fills in the rest of the fields in the
 *rest of the packet.

Packet Packet::buildPacketFromBuf(char* buf, int length){
  memcpy(&seqNum, builtPacket, 2);
  memcpy(&ackNum, builtPacket+2, 2);
  memcpy(&window, builtPacket+4, 2);
  unsigned short flags = 0;
  memcpy(&flags, builtPacket+6, 2);
  ack = ((flags&4)==4)?true:false;
  syn = ((flags&2)==2)?true:false;
  fin = ((flags&1)==1)?true:false;
  data = new char[length];
  memcpy(data, builtPacket+8, length);
}
*/

void Packet::cleanupPacket(){
  //delete [] data;
  if(builtPacket != NULL){
    delete [] builtPacket;
    builtPacket = NULL;
  }

}

void Packet::debugPacket(){
  cout << "Sequence Number: " << seqNum << endl;
  cout << "Ack Number: " << ackNum << endl;
  cout << "Window size: " << window << endl;
  if(ack == true){
    cout << "ACK IS SET" << endl;
  }
  cout << "Ack: " << (ack?"T":"F") << endl;
  cout << "Syn: " << (syn?"T":"F") << endl;
  cout << "Fin: " << (fin?"T":"F") << endl;
  cout << "Time: " << sentAt << endl;
  cout << "Sent: " << sent << endl;
  cout << "Length: " << length << endl;
  cout << "Data:" << endl;
  for(int i = 0; i < length; i++){
    cout << data[i];
  }
  cout << endl;
}

void debugVector(vector<Packet> vec){
  for(int i = 0; i < vec.size(); i++){
    cout << "Packet " << i << endl;
    vec[i].debugPacket();
  }
}

/*
 *This function takes a cstring name of the file we want to read and creates
 *a vector containing a list of packets containing data we want to send. The
 *connection setup/teardown are not handled here. It just literally makes a
 *bunch of packets filled with data and nothing else it set except the size
 *and the length of the packet.

vector<Pair> readFile(char* filename){
  char data[1024];
  ifstream sendFile;
  char* fileName = filename;
  vector<Pair> pktVec;
  int count = 0;
  sendFile.open(fileName, ios::in);
  while(true){
    sendFile.read(data, 1024);
    int length;
    if((length = sendFile.gcount())>0){
      Pair temp = Pair(data, length);
      pktVec.push_back(temp);
      count++;
      cout << data;
      cout << length << endl;
    }
    else{
      break;
    }
  }
  return pktVec;
}*/
