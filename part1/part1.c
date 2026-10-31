
#include <unistd.h>

int main()
{
    getpid();
    sync();
    getuid();
    dup(0);
    alarm(0);
}
