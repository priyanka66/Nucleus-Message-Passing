#include "syscall.h"

int
main()
{
  
    SendMessage("A", "Message 1");
    SendMessage("A", "Message 2");
    SendMessage("A", "Message 3");
    SendMessage("A", "Message 4");
    SendMessage("A", "Message 5");
    WaitAnswer("B");
    Exit(0);
}

