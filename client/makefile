CFLAGS:=-c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS:=-lboost_system -lpthread

all:clean StompWCIClient

EchoClient: bin/ConnectionHandler.o bin/echoClient.o bin/ClientHandler.o
	g++ -o bin/EchoClient bin/ConnectionHandler.o bin/echoClient.o bin/ClientHandler.o $(LDFLAGS)

StompWCIClient: bin/ConnectionHandler.o bin/StompClient.o bin/echoClient.o bin/event.o bin/Game.o bin/ClientHandler.o
	g++ -o bin/StompWCIClient bin/ConnectionHandler.o bin/StompClient.o bin/echoClient.o bin/event.o bin/Game.o bin/ClientHandler.o $(LDFLAGS)

bin/ConnectionHandler.o: src/ConnectionHandler.cpp
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/echoClient.o: src/echoClient.cpp
	g++ $(CFLAGS) -o bin/echoClient.o src/echoClient.cpp

bin/event.o: src/event.cpp
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

bin/Game.o: src/Game.cpp
	g++ $(CFLAGS) -o bin/Game.o src/Game.cpp

bin/StompClient.o: src/StompClient.cpp
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/ClientHandler.o: src/ClientHandler.cpp
	g++ $(CFLAGS) -o bin/ClientHandler.o src/ClientHandler.cpp


.PHONY: clean
clean:
	rm -f bin/*
	