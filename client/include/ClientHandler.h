#pragma once

#include <vector>
#include <string>
#include "ConnectionHandler.h"
#include "Game.h"
#include "../include/event.h"
#include <mutex>
#include <event.h>



using namespace std;

enum ClientState {
    DISCONNECTED,
    CONNECTED,
    WAIT_TO_LOG_IN,
    WAIT_TO_CONNECT,
    WAIT_TO_DISCONNECT
};


class ClientHandler {
public:
    ConnectionHandler* connectionHandler;
    string username;
    int receiptCounter;
    int subIdCounter;
    map<string, int> TopicToSubid;
    std::vector<Game> data;
    ClientState state;
    mutex statekey;
    mutex datalock;




public:
    ClientHandler();

    //Rule of 5
    ~ClientHandler();
    ClientHandler(const ClientHandler& other);
    ClientHandler(const ClientHandler&& other);
    ClientHandler& operator=(const ClientHandler& other);
    ClientHandler& operator=(const ClientHandler&& other);

    void setState(ClientState newstate);
    bool cheakState(ClientState otherstate);

    bool connecting();
    vector<string> splitBychar(const string &frame,char delimiter);
    void sendframe(string frame);
    void ProcessRequest();
    string CreateEventFrame (const Event env);
    void updateMap(map<string,vector<pair<string,string>>> &eventMap,map<string,vector<pair<string,string>>> &tmpMap);

    string createSummaryString(const string &Gamename,const string &reportermane);
    string mapToString(map<string,vector<pair<string,string>>> eventMap,string gamename);
    void parseEvent(string event,map<string, vector<pair<string, string>>> &tmpMap);
    vector<string> findString(string main,string secondery, char delimiter);
    void parseFrame();
    
};
