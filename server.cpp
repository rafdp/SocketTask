#include "SocketLib.h"

int main(int argc, char **argv)
{
    Server serv (PORTNUM);
    serv.start ();
}
