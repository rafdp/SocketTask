
all: server client

server.o: server.cpp SocketLib.h
	g++ -c $<

client.o: client.cpp SocketLib.h
	g++ -c $<

server: server.o SocketLib.o
	g++ -o $@ $< SocketLib.o

client: client.o SocketLib.o
	g++ -o $@ $< SocketLib.o

SocketLib.o: SocketLib.cpp
	g++ -c $<
