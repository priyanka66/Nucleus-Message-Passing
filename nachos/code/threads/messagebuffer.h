#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H
#include "list.h"

class MessageBuffer {
    private:
        int bufferID;
        int sender;
        int receiver;
        List<char*> *message;
        char* answer;
        bool isEmpty;
 
    public:
	MessageBuffer();
        ~MessageBuffer();
        int getBufferID();
        void setBufferID(int bufferID);
        
        int getSender();
        void setSender(int sender);
   
        int getReceiver();
        void setReceiver(int receiver);      
        
        void initialiseMessage();  
        char* getMessage();
        void setMessage(char* msg);
   
        char* getAnswer();
        void setAnswer(char* answer);
  
        bool getIsEmpty();
        void setIsEmpty(bool isEmpty);
 
};


#endif // MESSAGEBUFFER_H
