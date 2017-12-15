#include "SocketLib.h"

int main ()
{
    Client cl ("127.0.0.1", PORTNUM);
    cl.play ();
}
