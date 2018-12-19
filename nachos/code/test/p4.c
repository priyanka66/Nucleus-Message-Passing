#include "syscall.h"

int
main()
{
    WaitMessage("C");
    SendAnswer("Process B  sending back the answer to C");
    Exit(0);
}

