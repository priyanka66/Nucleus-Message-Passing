#include "syscall.h"

int
main()
{
    WaitMessage("B");
    SendAnswer("Process A got the message and sending back the answer to B");
//    WaitMessage("B");
    Exit(0);
}

