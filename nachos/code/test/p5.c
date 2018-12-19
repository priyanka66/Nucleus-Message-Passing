#include "syscall.h"

int
main()
{
    SendMessage("A", "Message 1 for A");
    SendMessage("A", "Message 2 for A");
    WaitAnswer();
    SendMessage("B", "Message 1 for B");
    SendMessage("B", "Message 2 for B");
    WaitAnswer();
    Exit(0);
}

