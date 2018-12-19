#include "syscall.h"

int
main()
{
    WaitMessage("C");
    SendAnswer("Process A sending back the answer to C");
    Exit(0);
}

