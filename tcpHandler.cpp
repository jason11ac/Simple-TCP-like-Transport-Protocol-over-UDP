#include "tcpHandler.h"

/*
Packet(char* buffer, int len, unsigned short seq, unsigned short ack, bool a,
  bool s, bool f);

 ACKS, SYNS, FINS
 serverMsg= message sent from SERVER, received by CLIENT.
 clientMsg = message sent from CLIENT, received by SERVER.
 1.) If we get (clientMsg: ackNum=don't care, seqNum=b, ack=0, syn=1, fin=0,
 length=0, data=nullptr) then send back (serverMsg: ackNum=b+1, seqNum=0,
 ack=1, syn=1, fin=0, length=0, data=nullptr)
 2.) If we get a regular message (clientMsg: ackNum = a, seqNum=b, ack=1,
 syn=0, fin=0, length=0, data=nullptr) then send (serverMsg: ackNum=b+c,
 seqNum=a, ack=1, syn=0, fin=0, length=c, data=something).
 TODO: implement the acks so that we only send an ack if everything up until
 that point has been received.
 3.) If there's no more data, and we just sent the packet (serverMsg: ackNum=a,
 seqNum=b, ack=1, syn=0, fin=0, length=c, data=something)
 then close the connection by sending (serverMsg: ackNum=a, seqNum=b+c, ack=1, syn=0, fin=1, length=0, data=nullptr).
 Note: If we get a FinAck from the client ack=1, syn=0, fin=1, ackNum=a, seqNum=b,
 then send ACK with ack=1, syn=0, fin=0, ack=b+1, seqNum=a) close connection.


void fillDataVec(char* filename){
  pairData = readFile(filename);
  idx = 0;
  cout << pairData.size();
}*/

//NEED to set the window size now. Figure that out.
vector<Packet> serverReceived(vector<Packet> prevQueue, char* buf, int length){
  Packet recPkt = Packet(length, buf);
  bool isDup = false;
  bool foundData = false;
  cout << "Receiving packet " << recPkt.getAckNum() << endl;

  //FAST RETRANSMIT PART. checks for 3 dups, then resend.
  if(recPkt.getAck() && !recPkt.getSyn() && !recPkt.getFin()){ //when we start, no chance we'll have to retransmit since we haven't sent anything yet. When we stop, maybe. Not sure.
    if(lastAck == recPkt.getAckNum() && duplicateCount != -1){
      duplicateCount++;
      if(duplicateCount == 3){
	//fast retransmit
	for(int i = 0; i < prevQueue.size(); i++){
	  if(prevQueue[i].getSeqNum() == recPkt.getAckNum()){
	    prevQueue[i].setNotSent(); //will stay in the queue and get resent.
	  }
	}

	ssthreash = max(ssthreash/2, 1024); //3 duplicate acks.
	cwnd = ssthreash;
	isSS = false;
	isDup = true;
	duplicateCount = 1;
      }
    }
    else{
      lastAck = recPkt.getAckNum();
      duplicateCount = 1; //got one ack that is the same as itself so...
    }
  }
  //cout << "DUPLICATE COUNT " << duplicateCount << endl;
  //this deletes the packet in the queue that just got acked. Not completely right yet.
  for(int i = 0; i < prevQueue.size(); i++){
    unsigned short s = (prevQueue[i].getLength()+prevQueue[i].getSeqNum()) % 30720;
    //    if(((recPkt.getAckNum() >= s) && prevQueue[i].getSent()) || prevQueue[i].getSyn() || prevQueue[i].getFin()){
    if(((recPkt.getAckNum() >= s) && prevQueue[i].getSent() && !prevQueue[i].getFin()) || (prevQueue[i].getSyn() && recPkt.getAckNum() == prevQueue[i].getSeqNum()+1) || (prevQueue[i].getFin() && (recPkt.getAckNum() == prevQueue[i].getSeqNum() + 1))){
      prevQueue[i].cleanupPacket();
      prevQueue.erase(prevQueue.begin() + i);
    }
  }
  vector<Packet> ret = prevQueue;
  
  if(gotAllData == false){
  //Case 1
  if(recPkt.getSyn() && !recPkt.getAck() && !recPkt.getFin() && !gotAllData){
    cwnd = 1024; //set window to 1 for now so you can synchronize.
    ret.push_back(Packet(NULL, 0, 0, recPkt.getSeqNum()+1, cwnd, true, true, false));
  }
  //Case 2
  else if(recPkt.getAck() && !recPkt.getSyn() && !recPkt.getFin() && !gotAllData){
    //printf("Came to case 2****************************************************\n");
    unsigned short startSeq;
    if(lastSeq == -1){
      startSeq = recPkt.getAckNum();
    }
    else{
      startSeq = lastSeq;
    }
    ////////////////////////////////////////////////////////
    ///////congestion avoidance + slow start stuff//////////
    ////////////////////////////////////////////////////////
    if(!isDup){
      if(cwnd < ssthreash){
        cwnd += 1024;
        cwnd = min(cwnd, WINDOWCAP);
	isDup = false;
	//cout << "SS CONGESTION WINDOW: " << " " << cwnd << " " << ssthreash << endl;
      }
      else{
        double x = cwnd;
        cwnd += (1024*1024)/cwnd; //will add 1 for ever cwnd windows
        isSS = false; //not slow start, could only set this once, but I'm lazy.
        cwnd = min(cwnd, WINDOWCAP);
	//cout <<"CA CONGESTION WINDOW: " << " "<< cwnd<< " " << ssthreash << endl;
      }
    }
    //windowSize = cwnd; //make this less terrible later.
    //cout <<"CONGESTION WINDOW: " << " " << cwnd << " " << ssthreash << endl;
    ////////////////////////////////////////////////////////
    /////////////////////end ca+ss stuff.///////////////////
    ////////////////////////////////////////////////////////
    int bytes = 0;
    for(int i = 0; i < ret.size(); i++){
      bytes += ret[i].getLength();
    }
    
    while(ret.size() < cwnd/1024){
      //add something that reads packet gradually here.
      char* buf = new char[1024];
      unsigned short length;
      sendFile.read(buf, 1024);
      if((length = sendFile.gcount())>0){
        Packet temp = Packet(buf, length, startSeq, recPkt.getSeqNum() + recPkt.getLength(), cwnd, true, false, false);
        ret.push_back(temp);
        startSeq = (startSeq + length) % 30720;
        bytes+=length;
      }
      else{
        gotAllData = true;
        sendFile.close();
	ret.push_back(Packet(NULL, 0, startSeq, recPkt.getSeqNum() + recPkt.getLength(), cwnd, true, false, true));
	//        printf("GOT ALL DATA\n");
        break;
      }
    }
    lastSeq = startSeq;
    
  }
  //Case 3 note part
  //else
  /*
  if(!recPkt.getSyn() && recPkt.getAck() && recPkt.getFin() && gotAllData){
    cwnd = 1024;
    Packet xy = Packet(NULL, 0, recPkt.getAckNum(), (recPkt.getSeqNum()+ 1), cwnd, true, false, false);
    ret.push_back(xy);
    }*/
  /*else if(isDone == false && gotAllData){
    printf("FIN FROM SRV\n");
    cwnd = 1024;
    //ret.clear();
    isDone = true;
    ret.push_back(Packet(NULL, 0, lastSeq, recPkt.getSeqNum() + recPkt.getLength(), cwnd, true, false, true));
    }*/
  /*else if(isDone == true && gotAllData){
    printf("WAIT FOR SHIT\n");
    ret.clear();
    }*/
  
  
  timedOut = false;
  }
  if(!recPkt.getSyn() && recPkt.getAck() && recPkt.getFin() && gotAllData){
    cwnd = 1024;
    Packet xy = Packet(NULL, 0, recPkt.getAckNum(), (recPkt.getSeqNum()+ 1), cwnd, true, false, false);
    ret.push_back(xy);
  }
  recPkt.cleanupPacket();
  
return ret;
}

/*
 1.) Start connection: choose (clientMsg: seqNum=b, ackNum=don't care, ack=0, syn=1, fin=0,
  length=0, data=nullptr) send that to the server. Then we'll get SYN-ACK, so we
  get (serverMsg: ackNum=a+1=a', seqNum=b, ack=1, syn=1, fin=0, length=0,
  data=nullptr), then send (clientMsg: ackNum=b+1, seq=a', ack=1, syn=0, fin=0, length=0, data=nullptr)
 2.) Data being received: Received data from server (serverMsg: ackNum=a, seqNum=b, ack=1, syn=0,
  fin=0, length=c), send (clientMsg: ackNum=b+c, seqNum=a, ack=1, syn=0, fin=0, length=0, data=nullptr)
  TODO: implement the acks so that we only send an ack if everything up until
  that point has been received.
 3.) End connection: Received data from server (serverMsg: ackNum=a, seqNum=b, ack=1, syn=0,
  fin=1, length=0, data=nullptr), send back (clientMsg: ackNum=b+1, seqNum=a, ack=1, syn=0, fin=1)
 4.)Received ack for the fin we just sent: close connection.
 */

Packet clientReceived(char* buf, int length, ostream& fd){
  Packet recPkt = Packet(length, buf);

   //Received regular packet with data
   if (recPkt.getAck() && !recPkt.getSyn() && !recPkt.getFin()) {
     //printf("Came here this time\n");
     if (recPkt.getSeqNum() == exp_seq) {
       //printf("Correct Seq Number\n");
       //printf("Exp_seq: %d\n", exp_seq);
       //printf("recPkt.getSeqNum: %d\n", recPkt.getSeqNum());
       //printf("recPkt.getSeqNum(): %d\n", recPkt.getSeqNum());
       //printf("recPkt.getAckNum(); %d\n", recPkt.getAckNum());

       //If this is true, in order packet has been received and we can add to file from buffer
       unsigned long int win_len = window.size();
       
       fd.write (recPkt.getData(), recPkt.getLength());

       if (win_len) {

	 //Sort vector here
	 window = sortVec(window);

	 //printf("Came to outOfOrder\n");
	 int length = window.size();
	 //printf("window.size() from length: %d\n", length);
	 exp_seq += recPkt.getLength();
	 exp_seq = exp_seq % 30720;
	 for (int i = 0; i < length; i++) {
	   //printf("%d\n", i);
	   if ((exp_seq) == window[i].getSeqNum()) {
	     if (window[i].getFin()) {
	       //Received fin from buffer
	       Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), exp_seq+1, client_win, true, false, true);
	       cout <<"Sending packet " << exp_seq+1 << endl;
	       return sendPkt;
	     }
	     //printf("Inputed correctly from buffer\n");
	     fd.write (window[i].getData(), window[i].getLength());
	     array1.push_back(i);
	     exp_seq += (window[i].getLength());
	     exp_seq = exp_seq % 30720;
	   }
	 }
 
	 //Delete packets
	 int length1 = array1.size();
	 //printf("array1.size(): %d\n", length1);
	 for (int j = 0; j < array1.size(); j++) {
	   window.erase(window.begin() + array1[j]);
	   //printf("Deleted\n");
	 }
	 array1.clear();
	 //Cumulative acking system
	 //printf("ack value: %d\n", exp_seq);
	 Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), exp_seq, client_win,  true, false, false) \
	   ;
	 //outOfOrder = false;
	 //Packet(NULL, 0, sendPkt.getSeqNum(), sendPkt.getAckNum(), client_win, sendPkt.getAck(), sendPkt.getSyn(), sendPkt.getFin()); //For retransmission
	 last_seq = sendPkt.getSeqNum();
	 cout <<"Sending packet " << exp_seq << endl;
	 return sendPkt;
       } else {
       
	 //fd.write (recPkt.getData(), recPkt.getLength());
	 unsigned short tempAckNum = recPkt.getSeqNum() + recPkt.getLength();
	 //printf("ack value: %hu\n", tempAckNum);
	 Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), tempAckNum, client_win, true, false, false);
	 exp_seq += recPkt.getLength();
	 exp_seq = exp_seq % 30720;
	 //last = new Packet(NULL, 0, sendPkt.getSeqNum(), sendPkt.getAckNum(), client_win, sendPkt.getAck(), sendPkt.getSyn(), sendPkt.getFin()); //For retransmission
	 last_seq = sendPkt.getSeqNum();
	 cout <<"Sending packet " << tempAckNum << endl;
	 return sendPkt;
       }
     } else { //Out of order packet, send duplicate ACK

       //printf("Incorrect Seq Number\n");
       //printf("Exp_seq: %d\n", exp_seq);
       //printf("recPkt.getSeqNum: %d\n", recPkt.getSeqNum());
       //printf("recPkt.getAckNum(): %d\n", recPkt.getAckNum());
       //unsigned short tempAckNum = recPkt.getSeqNum() + recPkt.getLength(); //Don't increment
       //printf("ack value: %d\n", exp_seq);
       Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), exp_seq, client_win, true, false, false);

       //Add to out-of-order vector
       int byte_size = 0;
       int length2 = window.size();
       //printf("window.size(): %d\n", length2);
       for (int b = 0; b < window.size(); b++) {
	 byte_size += window[b].getLength();
	 //printf("Came in here");
       }
       if (byte_size >= 15360) {
 	; //do nothing
 	//cout << "Too many bytes in client vector\n" << endl;
       } else if (exp_seq < recPkt.getSeqNum()) {
	 //printf("Put in buffer\n");
	 window.push_back(recPkt);
       } else {
	 ; //do nothing
	 //printf("Already have packet\n");
       }
       
       //last = new Packet(NULL, 0, sendPkt.getSeqNum(), sendPkt.getAckNum(), client_win, sendPkt.getAck(), sendPkt.getSyn(), sendPkt.getFin()); //For retransmission
       last_seq = sendPkt.getSeqNum();
       cout << "Sending packet " << exp_seq << endl;
       return sendPkt;
     }
   }
   //Started connection. Received a Syn-Ack
   else if (recPkt.getAck() && recPkt.getSyn() && !recPkt.getFin()) {
     fin_seq = recPkt.getSeqNum();
     if (!syn_ack) {
       exp_seq++;
       exp_seq = exp_seq % 30720;
       syn_ack = true;
     }
     unsigned short tempAckNum = recPkt.getSeqNum() + 1;
     //printf("ack value: %hu\n", tempAckNum);
     Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), tempAckNum, client_win, true, false, false);
     //last = new Packet(NULL, 0, sendPkt.getSeqNum(), sendPkt.getAckNum(), client_win, sendPkt.getAck(), \
     //		   sendPkt.getSyn(), sendPkt.getFin()); //For retransmission
     last_seq = sendPkt.getSeqNum();
     cout <<"Sending packet " << tempAckNum << endl;
     return sendPkt;
   }
   //Ending connection. Received a Fin
   else if (recPkt.getAck() && !recPkt.getSyn() && recPkt.getFin()) {
     //printf("received a fin now\n");
     fin_seq = recPkt.getSeqNum();
     if (exp_seq == recPkt.getSeqNum()) { //Send fin ack
       //printf("Sending fin ack");
       unsigned short tempAckNum = recPkt.getSeqNum() + 1;
       Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), tempAckNum, client_win, true, false, true);
       cout <<"Sending packet " << tempAckNum << " FIN" << endl;
       return sendPkt;
     } else {
       fin_in_buffer = 1;
       //printf("fin seq num: %d\n", recPkt.getSeqNum());
       //printf("Put fin in buffer\n");
       window.push_back(recPkt);
       //cout << "right after " << fin_in_buffer << endl;
       Packet sendPkt = Packet(NULL, 0, recPkt.getAckNum(), exp_seq, client_win, true, false, false);
       cout <<"Sending packet " << exp_seq << endl;
       return sendPkt;
     }
   }
   //Should pass one of the four cases above
   else if (!recPkt.getAck() && recPkt.getSyn() && !recPkt.getFin()) {
     //Retransmit syn to start connection
     Packet sendPkt = Packet(NULL, 0, 0, 0, client_win, false, true, false);
     cout << "Sending packet " << 0 << " Retransmission" << " SYN" << endl;
     return sendPkt;
   } 
   else { //Retransmit
     Packet sendPkt = Packet(NULL, 0, last_seq, exp_seq, client_win, 1, 0, 0); //For retransmission
     //cout << "Timeout occured" << endl;
     //printf("Retransmission of last packet\n");
     //cout << sendPkt.getAckNum() << endl;
     //cout << sendPkt.getSeqNum() << endl;
     //cout << "ack: " << sendPkt.getAck() << endl;
     //cout << "syn: " << sendPkt.getSyn() << endl;
     //cout << "fin: " << sendPkt.getFin() << endl;
     cout << "Sending packet " << exp_seq << " Retransmission" << endl;
     return sendPkt; //dup ack after timeout
   }
}

//Function to start the TCP connection using UDP
Packet startClient() {
  Packet sendPkt = Packet(NULL, 0, 0, 0, client_win, false, true, false);
  cout <<"Sending packet " << 0 << " SYN"<< endl;
  return sendPkt;
}


//resets the window in the server so we can send data again.
void initStuff(char* file){
  fileName = file;
  //printf("***************** %s\n", file);
  //idx = 0;
  sendFile.close();
  sendFile.open(file, ios::in);

  notAcked.clear();
  windowSize = 1024;
  lastSeq = -1;
  isDone = false;

  cwnd = 1024;
  isSS = true;
  timedOut = false;
  duplicateCount = -1;
  gotAllData = false;

  ssthreash = 15360;
  cwnd = 1024;
  static bool isSS = true;
  static bool timedOut = false;
}

//used for timeouts
void timeoutCA(){
  ssthreash = max(1024, ssthreash/2);
  cwnd = 1024;
  isSS = false;
  timedOut = true;
}

vector<Packet> sortVec(vector<Packet> v) {
  vector<Packet> ret;
  while(v.size() > 0) {
    unsigned short minSeqNumIdx = 0;
    for (int i = 0; i < v.size(); i++) {
      if (v[i].getSeqNum() < v[minSeqNumIdx].getSeqNum()) {
	minSeqNumIdx = i;
      }
    }
    ret.push_back(v[minSeqNumIdx]);
    v.erase(v.begin()+minSeqNumIdx);
  }
  return ret;
}
