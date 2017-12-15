
#include "SocketLib.h"

Game::Game (int m_, int n_, int win_size_) :
    field    (NULL),
    m        (m_),
    n        (n_),
    win_size (win_size_)
{
    try {field = new char [m * n];}
    catch (std::bad_alloc)
    {
        perror ("failed to allocate memory for game");
	exit (1);
    }
    bzero (field, m*n*sizeof (char));
}	

Game::~Game ()
{
    if (field) delete [] field;
    field = NULL;
}
#define LOOP_CONTENT \
{if (field[currentM*n + currentN] == prev && prev) consecutive++; \
else  \
if (field[currentM*n + currentN]) consecutive = 1; \
else \
if (!field[currentM*n + currentN]) consecutive = 0; \
prev = field[currentM*n + currentN]; \
if (consecutive == WIN_SIZE) \
{ \
    if (field[currentM*n + currentN] == MSG_PLAYER_1) return P1_WINS; \
    if (field[currentM*n + currentN] == MSG_PLAYER_2) return P2_WINS; \
    return ERROR_OCCURRED; \
}}

int Game::Move (int pos, char player)
{
    if (pos < 0 || pos >= m*n || field [pos]) return INVALID_POSITION;

    field[pos] = player;

    int newN = pos % n;
    int newM = pos / n;

    // horizontal
    int firstN = newN - WIN_SIZE + 1;
    int firstM = newM;
    if (firstN < 0) firstN = 0;
    char prev = field[firstM*n + firstN];
    int consecutive = prev ? 1 : 0;
    int currentN = firstN + 1;
    int currentM = newM;
    for (currentN; currentN <= newN + WIN_SIZE - 1 && currentN < n; currentN++)
        LOOP_CONTENT
    // vertical
    firstM = newM - WIN_SIZE + 1;
    firstN = newN;
    if (firstM < 0) firstM = 0;
    prev = field[firstM * m + firstN];
    consecutive = prev ? 1 : 0;
    currentM = firstM + 1;
    currentN = newN;
    for (currentM; currentM <= newM + WIN_SIZE - 1 && currentM < m; currentM++)
        LOOP_CONTENT

    // top left
    firstN = newN - WIN_SIZE + 1;
    firstM = newM - WIN_SIZE + 1;
    if (firstN < 0) firstN = 0;
    if (firstM < 0) firstM = 0;
    prev = field[firstM*m + firstN];
    consecutive = prev ? 1 : 0;
    currentN = firstN + 1;
    currentM = firstM + 1;
    for (currentN; currentN <= newN + WIN_SIZE - 1 &&
		   currentM <= newM + WIN_SIZE - 1 && 
		   currentN < n && 
		   currentM < m; (currentN++, currentM++))
        LOOP_CONTENT

    // top right
    firstN = newN - WIN_SIZE + 1;
    firstM = newM + WIN_SIZE - 1;
    if (firstN < 0) firstN = 0;
    if (firstM >= m) firstM = m-1;
    prev = field[firstM*m + firstN];
    consecutive = prev ? 1 : 0;
    currentN = firstN + 1;
    currentM = firstM + 1;
    for (currentN; currentN <= newN + WIN_SIZE - 1 &&
		   currentM >= newM - WIN_SIZE + 1 && 
		   currentN < n && 
		   currentM >= 0; (currentN++, currentM--))
        LOOP_CONTENT


    int zeroes = 0;
    for (int i = 0; i < m*n; i++) if (!field[i]) zeroes++;
    if (!zeroes) return DRAW;
    return OK;
}
#undef LOOP_CONTENT

char* Game::GetField ()
{
    return field;
}

Server::Server (int port_num) : 
    ppid      (getpid ()),
    nport     (port_num),
    serv_addr (),
    clnt_addr (),
    hp        (NULL)
{
    nport = htons (static_cast<u_short> (nport));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = nport;
}
#define RECV(i, n) \
nbytes = recv (client_communication_socket[i], data, n, 0); \
if (nbytes < 0) \
{ perror ("recv failed"); exit (1); }

#define SEND(i, n) \
nbytes = send (client_communication_socket[i], data, n, 0); \
if (nbytes == -1) \
{ perror ("send failed"); exit (1); }

#define CHECK_SEQUENCE(x) if (data[0] != x) {perror ("Client-server protocol mismatch"); exit (1);}

#define MOVE_LOOP(x, player) \
result = INVALID_POSITION; \
while (result == INVALID_POSITION) \
{  \
    RECV (x, 2) \
    CHECK_SEQUENCE (player) \
    result = tictac.Move (data[1], player); \
    if (result == INVALID_POSITION) \
        data[0] = MSG_DECLINE; \
    else  \
    if (result == OK) data[0] = MSG_ACCEPT; \
    else if (result == P1_WINS) \
    { \
        data[0] = MSG_WIN; \
        SEND (0, 1) \
	data[0] = MSG_LOSE; \
        SEND (1, 1) \
	break;	 \
    } \
    else if (result == P2_WINS) \
    { \
        data[0] = MSG_WIN; \
        SEND (1, 1) \
	data[0] = MSG_LOSE; \
        SEND (0, 1) \
	break;	 \
    } \
    else if (result == DRAW) \
    { \
        data[0] = MSG_DRAW; \
        SEND (0, 1) \
	data[0] = MSG_DRAW; \
        SEND (1, 2) \
	break;	 \
    } \
    else if (result == ERROR_OCCURRED) \
    { \
        perror ("Game error occurred"); \
	exit (1); \
    } \
    SEND (x, 1) \
} 


void Server::new_game (int socket)
{
    char data[MAX_BUF] = {};
    int nbytes = 0; 
    int client_communication_socket[2] = {};
    if (listen (socket, MAX_REQUESTS) == -1)
    {
        perror ("new_game listen failed");
	exit (1);
    }

    int newSock = 0;
    socklen_t addrlen = sizeof (clnt_addr[0]);
    for (int i = 0; i < 2; i ++)
    {
        if ((newSock = accept (socket, (sockaddr*) (clnt_addr), &addrlen)) == -1)
        {
            perror ("accept failed");
	    exit (1);
	}
        client_communication_socket[i] = newSock;
    }
    close (socket);
    RECV (0, 2)
    CHECK_SEQUENCE (MSG_CONNECTED)
    RECV (1, 2)
    CHECK_SEQUENCE (MSG_CONNECTED)
    if (data[1] != MSG_PLAYER_2)
    std::swap (client_communication_socket[0], client_communication_socket[1]);

    data[0] = MSG_BEGIN_REQUEST;
    SEND (0, 1)
    SEND (1, 1)
    RECV (0, 1)
    CHECK_SEQUENCE (MSG_READY)
    RECV (1, 1)
    CHECK_SEQUENCE (MSG_READY)

    Game tictac (FIELD_SIZE_M, FIELD_SIZE_N, WIN_SIZE);

    int result = INVALID_POSITION;
    while (true)
    {
        data[0] = MSG_TABLE;
	memcpy (data + 1, tictac.GetField (), FIELD_SIZE);
	SEND (0, 1 + FIELD_SIZE)
	MOVE_LOOP (0, MSG_PLAYER_1)

        data[0] = MSG_TABLE;
	memcpy (data + 1, tictac.GetField (), FIELD_SIZE);
	SEND (1, 1 + FIELD_SIZE)
	MOVE_LOOP (1, MSG_PLAYER_2)
    }
    close (client_communication_socket[0]);
    close (client_communication_socket[1]);
}
#undef CHECK_SEQUENCE
#undef RECV
#undef SEND
#undef MOVE_LOOP
void Server::start ()
{
    int sock = 0;
    int newSock = 0;

    int client_communication_socket[2] = {};
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error in socket() call"); exit(1);
    }


    if(bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror(" Error in bind() call"); exit(1);
    }
   
    if(listen(sock, MAX_REQUESTS) == -1)
    {
        perror("Error in listen() call"); exit(1);
    }
    char connectedClients = 0;
    while (true)
    {
        socklen_t addrlen = 0;
        int pid = 0;
        addrlen = sizeof (sockaddr_in);
        if (connectedClients == 0)
            bzero (clnt_addr, 2*sizeof (sockaddr_in));

        if (connectedClients == 1)
            bzero (clnt_addr + 1, sizeof (sockaddr_in));
        
        if((newSock = accept(sock, 
                             (sockaddr *)(clnt_addr + connectedClients), 
                             &addrlen)) == -1)
        {
            perror("Error in accept() call"); exit(1);
        }
        int nbytes = recv (newSock, buf, MAX_BUF, 0);
        if (!nbytes)
        {
            perror ("Got message of null length");
            exit (1);
        }

        if (buf[0] == MSG_CONNECT)
        {
            client_communication_socket[connectedClients] = newSock;
            connectedClients++;
            
	    if (connectedClients == 1)
	    {
	        char data[4] = {MSG_PLAYER_1, FIELD_SIZE_M, FIELD_SIZE_N, WIN_SIZE};
                int size = 4*sizeof (char);
                send (client_communication_socket[0], &data, size, 0);
		continue;
	    }
	    char data[4] = {MSG_PLAYER_2, FIELD_SIZE_M, FIELD_SIZE_N, WIN_SIZE};
            int size = 4*sizeof (char);
            send (client_communication_socket[1], &data, size, 0);
            
            sockaddr_in forked_server_addr = serv_addr;
            int forkedPort = PORTNUM + 1; 
	    bool createdForkedServer = false;
	    int forkedSocket = socket (AF_INET, SOCK_STREAM, 0);
	    if (forkedSocket == -1)
	    {
                perror ("Failed to create socket for fork");
		exit (1);
	    }
	    while (!createdForkedServer)
            {
               forked_server_addr.sin_port = htons (static_cast<u_short> (forkedPort));
               if (bind (forkedSocket, 
                         (struct sockaddr*)&forked_server_addr, 
			 sizeof (forked_server_addr)) != -1) break;
	       forkedPort ++;
	    }
            data[0] = MSG_NEW_PORT;
	    *(reinterpret_cast<short*> (data + 1)) = static_cast <short> (forkedPort);
            
            send (client_communication_socket[0], &data, size, 0);
            send (client_communication_socket[1], &data, size, 0);
	 
	    connectedClients = 0;
            if ((pid = fork ()) == -1)
            {
                perror ("fork failed"); exit (1);
            }

            if (!pid) 
	    {
                nport = forkedPort;
		serv_addr = forked_server_addr;

		new_game (forkedSocket);
	        close (forkedSocket);	
		return;
	    }
            else  
	    {
                close (client_communication_socket[0]);
                close (client_communication_socket[1]);
		//break;
		continue;
	    }
        }
    }

    close(sock);

}

void Server::stop ()
{
    int sock = 0; 
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror(" Error socket()"); exit(1);
    }

    
    if(connect(sock, 
               (struct sockaddr *)&serv_addr, 
               sizeof(serv_addr)) == -1)
    {
        perror("Error connect()"); exit(1);
    }

    send (sock, TERMINATE_MESSAGE, sizeof (TERMINATE_MESSAGE), 0);

    close (sock);
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------


Client::Client (std::string addr_, int port_num) : 
    serv_addr (),
    hp        (NULL),
    nport     (port_num),
    addr      (addr_)
{

}

Client::~Client()
{

}

#define SEND(n) \
nbytes = send (sock, data, n, 0); \
if (nbytes == -1) \
{ perror ("send failed"); exit (1); }

#define RECV(n) \
nbytes = recv (sock, data, n, 0); \
if (nbytes < 0) \
{ perror ("recv failed"); exit (1); }

#define CHECK_SEQUENCE(x) if (data[0] != x) {perror ("Client-server protocol mismatch"); exit (1);}

void Client::play ()
{
    printf ("Conneting to server..\n");
    int sock = 0;
    
    sockaddr_in serv_addr;
    hostent *hp;
    char data[MAX_BUF] = {};
    char data1[MAX_BUF] = {};
    int nbytes = 0;
    
// Преобразует строку имени хоста в ip-адрес   
    if((hp = gethostbyname(addr.c_str ())) == 0)
    {
        perror("Error gethostbyname()"); exit(3);
    }
    
    bzero(&serv_addr, (size_t)sizeof(serv_addr));
    bcopy(hp->h_addr, &(serv_addr.sin_addr), hp->h_length);
    serv_addr.sin_family = hp->h_addrtype;
    serv_addr.sin_port = htons((ushort)nport);

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror(" Error socket()"); exit(1);
    }

    fprintf(stderr, "Server's host address: %s\n", inet_ntoa(serv_addr.sin_addr));

    if(connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Error connect()"); exit(1);
    }
    
    data[0] = MSG_CONNECT; 

    SEND (1)
    RECV (4)
    if (data[0] != MSG_PLAYER_1 && data[0] != MSG_PLAYER_2) {CHECK_SEQUENCE (MSG_PLAYER_1)}
    const char PLAYER = data[0];
    const char m = data[1];
    const char n = data[2];
    const char winSize = data[3];
    printf ("Playing a %d x %d game, win length of %d\nYou are player %d\n", 
            m, n, winSize, PLAYER == MSG_PLAYER_1 ? 1 : 2);

    RECV (3)
    CHECK_SEQUENCE (MSG_NEW_PORT)
    serv_addr.sin_port = htons(*((ushort*) (data+1)));
    close (sock);
    sleep(1);
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror(" Error socket()"); exit(1);
    }

    if(connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Error connect()"); exit(1);
    }

    data[0] = MSG_CONNECTED;
    data[1] = PLAYER;
    SEND (2)
    RECV (1)
    CHECK_SEQUENCE (MSG_BEGIN_REQUEST)
    printf ("Beginning game\nPress ENTER when ready\n");
    getchar ();
    
    data[0] = MSG_READY;
    SEND (1)
    char* field = data1;
    //-------------------------------------
    system ("tput reset");
    for (int y = 0; y < m; y++)
    { 
	for (int x = 0; x < n; x++)
	    printf ("+-");
	printf ("+\n|");
        for (int x = 0; x < n; x++)
        {
            printf (" |");
	}
	printf ("\n");
    }
    for (int x = 0; x < n; x++)
        printf ("+-");
    printf ("+\n");
    //------------------------------------
    if (PLAYER == MSG_PLAYER_2) printf ("Waiting for opponent\n");
    while (true)
    {
        RECV (1+m*n)
	memcpy (data1, data + 1, m*n);
	if (data[0] != MSG_TABLE) break;
	printf ("Got table\n");
	int c = 0;
	int cursor = 0;
	while (field[cursor]) cursor++;
        //system ("/bin/stty raw");
	while (true)
   	{
	    system ("tput reset");
            switch (c)
	    {
		case 'w':
		    cursor -= n;
		    while (field[cursor] && cursor >= 0) cursor -= n;
		    while (cursor < 0 || field[cursor]) cursor += n;
		    break;
		case 'a':
		    cursor --;
		    while (field[cursor] && cursor >= 0) cursor--;
		    while (cursor < 0 || field[cursor]) cursor++;
		    break;
		case 's':
		    cursor += n;
		    while (field[cursor] && cursor < n*m) cursor += n;
                    while (cursor >= m*n || field[cursor]) cursor -= n;
		    break;
		case 'd':
		    cursor++;
		    while (field[cursor] && cursor < n*m) cursor ++;
		    while (cursor >= m*n || field[cursor]) cursor --;
		    break;
                default: 
		    break;
	    }
	    //------------------------------------------------
            for (int y = 0; y < m; y++)
            { 
		for (int x = 0; x < n; x++)
		    printf ("+-");
		printf ("+\n|");
	        for (int x = 0; x < n; x++)
	        {
	            char currentC = 0;
		    if (field[y*n+x] == MSG_PLAYER_1) currentC = 'x';
		    else
		    if (field[y*n+x] == MSG_PLAYER_2) currentC = 'o';
		    else
		    if (y*n+x == cursor) currentC = 'v';
		    else
		    if (!field[y*n+x]) currentC = ' ';
                    printf ("%c|", currentC);
		}
		printf ("\n");
	    }
	    
            for (int x = 0; x < n; x++)
	        printf ("+-");
            printf ("+");
            //------------------------------------------------
	    while ((c = getchar ()) == '\n');
	    if (c == 'G') break;
	}
        field[cursor] = PLAYER;
	data[0] = PLAYER;
	data[1] = cursor;
	SEND (2)
	//system ("/bin/stty cooked");
	RECV (1)
	system ("tput reset");
	//-----------------------------------
        for (int y = 0; y < m; y++)
        { 
	    for (int x = 0; x < n; x++)
	        printf ("+-");
	    printf ("+\n|");
	    for (int x = 0; x < n; x++)
     	    {
                char currentC = 0;
                if (field[y*n+x] == MSG_PLAYER_1) currentC = 'x';
		else
		if (field[y*n+x] == MSG_PLAYER_2) currentC = 'o';
		else
		if (!field[y*n+x]) currentC = ' ';
                printf ("%c|", currentC);
	    }
            printf ("\n");
	}
	//-----------------------------------
	if (data[0] != MSG_ACCEPT) break;
	printf ("Waiting for opponent\n");
    }
    if (data[0] == MSG_WIN) printf ("You won\n");
    else
    if (data[0] == MSG_LOSE) printf ("You lost\n");
    else
    if (data[0] == MSG_DRAW) printf ("Draw\n");
    else
    {
	printf ("ERROR: %d\n", data[0]);
        perror ("\"Decline\" error occured"); exit (1);
    }
    close (sock);
    return;
}

#undef SEND
#undef RECV
#undef CHECK_SEQUENCE

