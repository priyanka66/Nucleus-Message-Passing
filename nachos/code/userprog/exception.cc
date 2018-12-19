// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "addrspace.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

map <int, int> childExists;

void handlePageFault(int vpn) {
	int pageNum = kernel->physPage->FindAndSet();
	char* buff= new char[PageSize];
	bzero(buff,PageSize);
	if(pageNum == -1){
		int evictPage = rand() % NumPhysPages;
		TranslationEntry* evictedEntry = kernel->threadPPN[evictPage]->space->getByPPN(evictPage);
		evictedEntry->physicalPage = -1;
		evictedEntry->valid = FALSE;

		int start = evictPage*PageSize;       
		int end = start + PageSize;
		int j = 0;
		// copy from memory to buffer
		for(int i = start; i < end; i++){     
			buff[j++] =  kernel->machine->mainMemory[i];
		}

		// copy from buffer to swapFile
		kernel->swapFile->WriteAt(buff, PageSize, evictedEntry->swapLocation*PageSize);



		bzero(buff, PageSize); 
		TranslationEntry* entry = kernel->currentThread->space->getPagetable(vpn);         

		// copy from swapFile to buffer
		kernel->swapFile->ReadAt(buff, PageSize, (entry->swapLocation * PageSize));

		start = pageNum * PageSize;
		end = start + PageSize;
		j = 0;


		// copy from buffer to main memory
		for(int i = start; i < end; i++){
			kernel->machine->mainMemory[i] = buff[j++];
		}


		entry->physicalPage = pageNum;                                             
		entry->valid = TRUE;                                                       
		kernel->threadPPN.insert(pair<int, Thread*>(pageNum, kernel->currentThread)); 
	}
	else{
		TranslationEntry* entry = kernel->currentThread->space->getPagetable(vpn);
		bzero(buff, PageSize);

		// copy from swap file to buffer 
		kernel->getSwapFile()->ReadAt(buff, PageSize, (entry->swapLocation * PageSize));

		int start = pageNum * PageSize;
		int end = start + PageSize;
		int j = 0;

		// copy from buffer to main memory
		for(int i = start; i < end; i++){  
			kernel->machine->mainMemory[i] = buff[j++];
		} 

		entry->physicalPage = pageNum;
		entry->valid = TRUE;
		kernel->threadPPN.insert(pair<int, Thread*>(pageNum, kernel->currentThread)); 

	}

	delete kernel->swapFile;
}

map <int, Thread*> childParent;
void AdvancePC();
void Exit_POS(int child){
	if (childExists[child] != 1){
		cout<<"\nInvalid child ID -- Aborting the program"<<endl;
		exit(0);
	}
	(void) kernel->interrupt->SetLevel(IntOff);
	Thread* parent = childParent[child];
	if (parent != NULL ) {
		cout << "Exiting  Child Id " << child << endl;
		kernel->scheduler->ReadyToRun(parent);
		(void) kernel->interrupt->SetLevel(IntOff);
	}
	return;
}
void ForkTest1(int id)
{
	printf("ForkTest1 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest1 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}
void ForkTest2(int id)
{
	printf("ForkTest2 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest2 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}

void ForkTest3(int id)
{
	printf("ForkTest3 is called, its PID is %d\n", id);
	for (int i = 0; i < 3; i++)
	{
		printf("ForkTest3 is in loop %d\n", i);
		for (int j = 0; j < 100; j++) 
			kernel->interrupt->OneTick();
	}
	Exit_POS(id);
}


void AdvancePC(){
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);  
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
}

char* ReadData(int addr) {
	char* buff = new char[MemorySize];
	int size = 0;
	buff[MemorySize - 1] = '\0'; 
	bool ret;  
	do{
		ret = kernel->machine->ReadMem(addr,sizeof(char), (int*)(buff+size));
		if(!ret) {

			kernel->machine->ReadMem(addr,sizeof(char), (int*)(buff+size));

		}
		addr+=sizeof(char);
		size++;
	} while( (size < (MemorySize - 1) && buff[size-1] != '\0'));

	size--;

	return buff;
}

	void
ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which) {
		case PageFaultException: {
						 int virtualAddr = kernel->machine->ReadRegister(39);
						 int vpn = virtualAddr / PageSize;
						 handlePageFault(vpn);
						 return;
					 }
					 break;
		case SyscallException:
					 switch(type) {
						 case SC_Halt:
							 DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

							 SysHalt();

							 ASSERTNOTREACHED();
							 break;
						 case SC_Fork_POS:
							 {  
								 (void) kernel->interrupt->SetLevel(IntOff); 
								 int value  = kernel->machine->ReadRegister(4);
								 cout << "In fork  ";
								 int childPID;
								 if(value == 1)
								 {   
									 Thread *child = new Thread("Child Thread - 1");
									 child->Fork((VoidFunctionPtr)ForkTest1, (void *)child->getPID());
									 childPID = child->getPID();
									 cout <<"Created Process with PID " <<  childPID;
									 kernel->machine->WriteRegister(2, (int)childPID);  
								 }
								 else if(value == 2)
								 {
									 Thread *child = new Thread("Child Thread - 2");
									 child->Fork((VoidFunctionPtr)ForkTest2, (void *)child->getPID());
									 childPID = child->getPID();
									 cout <<"Created Process with PID " <<  childPID;
									 kernel->machine->WriteRegister(2, (int)childPID);
								 }
								 else if(value == 3)
								 {
									 Thread *child = new Thread("Child Thread - 3");
									 child->Fork((VoidFunctionPtr)ForkTest3, (void *)child->getPID());
									 childPID = child->getPID();
									 cout <<"Created Process with PID " <<  childPID;
									 kernel->machine->WriteRegister(2, (int)childPID);
								 }
								 childExists.insert(pair<int, int>(childPID,1));
								 AdvancePC();
								 (void) kernel->interrupt->SetLevel(IntOn);
								 return;
							 }
							 break;
						 case SC_Wait_POS:
							 {
								 (void) kernel->interrupt->SetLevel(IntOff);	
								 int child =  kernel->machine->ReadRegister(4);
								 if (childExists[child] != 1){
									 cout<<"\nInvalid child Id - Aborting program"<<endl;
									 exit(0);
								 } 
								 (void) kernel->interrupt->SetLevel(IntOff);
								 childParent.insert(pair <int, Thread*> (child, kernel->currentThread));
								 kernel->currentThread->Sleep(FALSE);
								 AdvancePC();
								 (void) kernel->interrupt->SetLevel(IntOn);
								 return;
							 }
							 return;
							 break;

						 case SC_Exit:
							 {
								 (void) kernel->interrupt->SetLevel(IntOff);	
								 cout<<" Process "<<kernel->currentThread->getName()<<" is finishing"<<endl;
								 if (kernel->currentThread->isMessageQueueEmpty()) {
									 MessageBuffer *ansBuffer = kernel->currentThread->getMessage();
									 if(ansBuffer != NULL){
										 kernel->processIDMap.erase(kernel->currentThread->getPID());
										 ansBuffer->setMessage("Sending dummy message as i am finishing");
										 Thread *sender = kernel->processIDMap[ansBuffer->getSender()];
										 kernel->ansBuffMap.insert(pair<int, int>(sender->getPID(), ansBuffer->getBufferID()));
										 if (sender->getWaitAns()){
											 sender->setWaitAns(FALSE);
											 kernel->scheduler->ReadyToRun(sender);
										 } 
									 }else{
										 (void) kernel->interrupt->SetLevel(IntOff);
										 int freeBuf =  kernel->msgBuffer->FindAndSet();
										 char *tmpName = kernel->currentThread->getName();
										 int receiverID = kernel->waitingProMap[tmpName[0]];
										 Thread *sendingProcess = kernel->processIDMap[receiverID];
										 if(sendingProcess != NULL){
											 ansBuffer = &kernel->messagebuffer[freeBuf]; 
											 ansBuffer->setSender(kernel->currentThread->getPID());
											 ansBuffer->setReceiver(receiverID);
											 ansBuffer->setMessage("Sending dummy message as i am finishing");
											 ansBuffer->setIsEmpty(FALSE);
											 kernel->messagebuffer[freeBuf].setIsEmpty(FALSE);
											 kernel->processIDMap[receiverID]->addMessage(ansBuffer);
											 if (sendingProcess->getWaitMsg()) {
												 kernel->scheduler->ReadyToRun(sendingProcess);
												 sendingProcess->setWaitMsg(FALSE);
											 }   
											 kernel->machine->WriteRegister(2, freeBuf);
										 }   
									 }
								 } else {
									 MessageBuffer *ansBuffer;
									 while ((ansBuffer = kernel->currentThread->removeMessage()) != NULL ) {
										 ansBuffer->setAnswer("Sending dummy answer as i am finishing");
										 Thread *sender = kernel->processIDMap[ansBuffer->getSender()];
										 kernel->ansBuffMap.insert(pair<int, int>(sender->getPID(), ansBuffer->getBufferID()));
										 if (sender->getWaitAns()){
											 sender->setWaitAns(FALSE);
											 kernel->scheduler->ReadyToRun(sender);
										 }        
									 }
								 }
								 kernel->currentThread->Finish();
								 (void) kernel->interrupt->SetLevel(IntOn);	
								 return;
							 }
							 break; 
						 case SC_Write:
							 {
								 (void) kernel->interrupt->SetLevel(IntOff);	
								 int startAddr = 0;
								 int totalLength = 0;
								 startAddr =  kernel->machine->ReadRegister(4);
								 totalLength =  kernel->machine->ReadRegister(5);
								 int currLength = 0;
								 int str;
								 do  {
									 (void) kernel->interrupt->SetLevel(IntOff);
									 bool ret = kernel->machine->ReadMem(startAddr, sizeof(char), (&str));
									 if (ret) {
										 startAddr += sizeof(char); 
										 printf("%c", (char)str);
										 currLength++; 
									 }
								 }while((currLength <(totalLength)) && (str!=0));
								 printf("\n");
								 currLength--;
								 (void) kernel->interrupt->SetLevel(IntOff);
								 (void) kernel->interrupt->SetLevel(IntOn);	
								 AdvancePC();
								 return;
							 }
							 break; 
						 case SC_Send_Message:
							 {
								 int receiverAddr = kernel->machine->ReadRegister(4);
								 int msgAddr =  kernel->machine->ReadRegister(5);
								 char* message = ReadData(msgAddr);
								 int freeBuf;

								 char* receiverName = ReadData(receiverAddr);
								 char temp = *receiverName;
								 int receiverID = kernel->processNameToID[temp];
								 MessageBuffer *msgBuff;
								 int tempBuff = -1;
								 bool flag = false;
								 if(kernel->currentThread->numMsgs < kernel->maxMsgs){
									 if (kernel->currentThread->sendBuffMap.size() > 0) {
										 tempBuff = kernel->currentThread->sendBuffMap[receiverID];
									 }
									 if (tempBuff == -1) {
										 freeBuf =  kernel->msgBuffer->FindAndSet();
										 if(freeBuf == -1){
											 kernel->machine->WriteRegister(2, -1);
											 cout << "\nNo free buffers to send message\n";
											 AdvancePC();

										 } else {
											 kernel->currentThread->sendBuffMap.insert(pair<int, int>(receiverID, freeBuf));              
											 flag=true;
										 }
									 } else { 
										 freeBuf = tempBuff;
										 flag=true;
									 } 

									 if (flag == true){
										 (void) kernel->interrupt->SetLevel(IntOff);
										 Thread *sendingProcess = kernel->processIDMap[receiverID];
										 if(sendingProcess != NULL){
											 msgBuff = &kernel->messagebuffer[freeBuf]; 
											 msgBuff->setSender(kernel->currentThread->getPID());
											 msgBuff->setReceiver(receiverID);
											 msgBuff->setMessage(message);
											 msgBuff->setIsEmpty(FALSE);
											 kernel->messagebuffer[freeBuf].setIsEmpty(FALSE);
											 kernel->processIDMap[receiverID]->addMessage(msgBuff);
											 if (sendingProcess->getWaitMsg()) {
												 kernel->scheduler->ReadyToRun(sendingProcess);
												 sendingProcess->setWaitMsg(FALSE);
											 }

											 cout << "\nMessage written into buffer : " << freeBuf << endl; 
											 cout << "Process " << kernel->currentThread->getName()<< " is sending a message to Process " << receiverName<< " ===>"; 
											 cout <<message<<endl<<endl;
											 kernel->machine->WriteRegister(2, freeBuf);
										 }
									 }
									 (void) kernel->interrupt->SetLevel(IntOn);
									 kernel->currentThread->numMsgs++;
								 }else{
									 cout<<"\n==========================================================================="<<endl;
									 cout<< "Reached max number of messages for the process "<<kernel->currentThread->getName()<<endl;
									 cout<<"==========================================================================="<<endl;
								 }
								 AdvancePC();
								 return;
							 }
							 break; 
						 case SC_Wait_Message:
							 {
								 cout << "Process "<<  kernel->currentThread->getName() << " is waiting for a message from ";
								 Thread *waitingProcess = kernel->currentThread;
								 int receiverAddr = kernel->machine->ReadRegister(4);
								 char* receiverName = ReadData(receiverAddr);
								 char temp = *receiverName;
								 cout<<temp<<endl;
								 kernel->waitingProMap.insert(pair<char,int>(receiverName[0],waitingProcess->getPID()));
								 MessageBuffer *msgBuffer = waitingProcess->getMessage();
								 char *recMsg = NULL;
								 if ((msgBuffer) == NULL){ 
									 waitingProcess->setWaitMsg(TRUE);
									 (void) kernel->interrupt->SetLevel(IntOff);
									 waitingProcess->Sleep(FALSE);
									 (void) kernel->interrupt->SetLevel(IntOn);

								 } else  {
									 cout<<"The content of the message received is -- "<<endl;
									 recMsg = msgBuffer->getMessage();
									 while(recMsg != NULL){
										 cout << recMsg <<endl;
										 recMsg = msgBuffer->getMessage();
									 }
									 AdvancePC();
								 }
								 return; 
							 }
							 break;
						 case SC_Send_Answer: {
									      (void) kernel->interrupt->SetLevel(IntOff);
									      cout << "Process "<<  kernel->currentThread->getName() << " is sending an answer to "; 
									      int ansAddr =  kernel->machine->ReadRegister(4);
									      char* answer = ReadData(ansAddr);
									      char *senderName;
									      MessageBuffer  *ansBuffer = (kernel->currentThread->removeMessage());
									      if (ansBuffer == NULL) {
										      break;
									      } else  {
										      ansBuffer->setAnswer(answer);
										      Thread *sender = kernel->processIDMap[ansBuffer->getSender()];
										      if(sender != NULL){
											      senderName = sender->getName();
											      cout << sender->getName()<<endl;
											      MessageBuffer *msgBuffer = ansBuffer;
											      kernel->ansBuffMap.insert(pair<int, int>(sender->getPID(), ansBuffer->getBufferID())); 
											      kernel->waitingProMap.erase(senderName[0]);
											      if (sender->getWaitAns()){
												      sender->setWaitAns(FALSE);
												      kernel->scheduler->ReadyToRun(sender);
											      }
										      } 
									      }
									      (void) kernel->interrupt->SetLevel(IntOn);
									      AdvancePC();
									      return;
								      }
								      break;
						 case SC_Wait_Answer: { 
									      cout<<"Process "<<  kernel->currentThread->getName() <<" is waiting for an answer\n";

									      int receiverID = kernel->currentThread->getPID();
									      int senderAddr = kernel->machine->ReadRegister(4);
									      char* senderName = ReadData(senderAddr);
									      MessageBuffer *msgBuffer;
									      int bufferID;
									      if(kernel->ansBuffMap.size() > 0){
										      bufferID = kernel->ansBuffMap[receiverID];
										      kernel->ansBuffMap.erase(receiverID);
										      msgBuffer = &kernel->messagebuffer[bufferID];
									      }else{
										      msgBuffer = NULL;
									      }
									      if (msgBuffer == NULL) {
										      kernel->currentThread->setWaitAns(TRUE);
										      (void) kernel->interrupt->SetLevel(IntOff);
										      kernel->currentThread->Sleep(FALSE);
										      (void) kernel->interrupt->SetLevel(IntOn);
									      } else {
										      cout << "Process "<<  kernel->currentThread->getName() << " received answer and the content is --- " << 
											      cout << msgBuffer->getAnswer() << endl;
										      msgBuffer->setIsEmpty(TRUE);
										      kernel->currentThread->setWaitAns(FALSE);
										      kernel->msgBuffer->Clear(bufferID);
										      AdvancePC();
									      }
									      return;
								      }
								      break;
						 case SC_Add:
								      DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

								      /* Process SysAdd Systemcall*/
								      int result;
								      result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
										      /* int op2 */(int)kernel->machine->ReadRegister(5));

								      DEBUG(dbgSys, "Add returning with " << result << "\n");
								      /* Prepare Result */
								      kernel->machine->WriteRegister(2, (int)result);

								      /* Modify return point */
								      {
									      /* set previous programm counter (debugging only)*/
									      kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

									      /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
									      kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

									      /* set next programm counter for brach execution */
									      kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
								      }

								      return;

								      ASSERTNOTREACHED();

								      break;

						 default:
								      cerr << "Unexpected system call " << type << "\n";
								      break;
					 }
					 break;
		default:
					 cerr << "Unexpected user mode exception" << (int)which << "\n";
					 break;
	}
	ASSERTNOTREACHED();
}
