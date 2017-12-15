#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <exception>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

const int PORTNUM = 15000;

const size_t MAX_REQUESTS = 5;
const size_t MAX_BUF = 80;

const char TERMINATE_MESSAGE[] = "terminate";
const char PLAY_MESSAGE[] = "play";
const int FIELD_SIZE_M = 6,
          FIELD_SIZE_N = 4,
	  FIELD_SIZE   = FIELD_SIZE_M*FIELD_SIZE_N,
	  WIN_SIZE     = 3;

enum MESSAGES
{
     MSG_CONNECT       = 17,
     MSG_PLAYER_1      = 19,
     MSG_PLAYER_2      = 21,
     MSG_CONNECTED     = 23,
     MSG_BEGIN_REQUEST = 25,
     MSG_READY         = 26,
     MSG_TABLE         = 27,
     MSG_ACCEPT        = 28,
     MSG_DECLINE       = 29,
     MSG_WIN           = 30,
     MSG_LOSE          = 31,
     MSG_ABORT         = 32,
     MSG_NEW_PORT      = 33,
     MSG_DRAW          = 34
};	

enum GAME_RESULTS
{
    INVALID_POSITION      = 54,
    OK                    = 56,
    P1_WINS               = 57,
    P2_WINS               = 58,
    ERROR_OCCURRED        = 59,
    DRAW                  = 60
};

class Game
{
    char* field;
    const int m, n, win_size;

    public:
    Game (int m_, int n_, int win_size_);
    ~Game ();
    int Move (int pos, char player);
    char* GetField ();
};

class Server
{
    int ppid; // родительский pid
    int nport; // номер порта
    sockaddr_in serv_addr; // ip сервера
    // массив адресов клиентов
    sockaddr_in clnt_addr[2];
// для получения IP-адреса по имени хоста
    hostent *hp;

    char buf[MAX_BUF], hname[MAX_BUF];
// когда подключились два клиента
// запускаем дочерний процесс для
// обслуживания игры
// далее сервер продолжает слушать
   void new_game(int socket);
public:
//создает порт чтобы слушать подключения
// заполняет информацию serv_addr
   Server(int port_num);
// запускает бесконечный цикл приема запросов
   void start();
// останавливает сервер
// для остановки запускается еще один процесс 
// сервера с ключом -stop
// и посылает ему сообщение terminate 
// сервер может получить сообщение terminate 
// также от клиента
   void stop();   
};

class Client
{
    sockaddr_in serv_addr; // ip сервера
// для получения IP-адреса по имени хоста
    hostent *hp;
    int nport;// номер порта
    std::string addr;
public:
// запускает сервер и подключается к хосту
// с адресом addr
   Client(std::string addr_, int port_num);
// сообщает серверу о выходе из игры
// закрывает соединение
   ~Client();
// предоставляет возможность делать ходы
// получает и обрабатывает сообщения от 
// сервера
   void play();
};



