#include "copyright.h"
#include "messagebuffer.h"

MessageBuffer::MessageBuffer()
{
  message = new List<char*>;
}

MessageBuffer::~MessageBuffer()
{}
int MessageBuffer::getBufferID() {
        return this->bufferID;
}
void MessageBuffer::setBufferID(int bufferID){
        this->bufferID = bufferID;
}

int MessageBuffer::getSender(){
        return this->sender;
}
void MessageBuffer::setSender(int sender){
        this->sender = sender;
}

int MessageBuffer::getReceiver(){
        return this->receiver;
}
void MessageBuffer::setReceiver(int receiver){
        this->receiver = receiver;
}

void MessageBuffer::initialiseMessage(){
    message = new List<char*>;
}
char* MessageBuffer::getMessage(){
     if (!message->IsEmpty()) {
       return message->RemoveFront();
   }  else {
       return NULL;
   }
}

void MessageBuffer::setMessage(char* msg){
     message->Append(msg);
}

char* MessageBuffer::getAnswer(){
        return  this->answer;
}

void MessageBuffer::setAnswer(char* answer){
        this->answer = answer;
}

bool MessageBuffer::getIsEmpty(){
        return  this->isEmpty;
}
void MessageBuffer::setIsEmpty(bool isEmpty){
         this->isEmpty = isEmpty;
}



