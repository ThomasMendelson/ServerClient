#include "ClientHandler.h"
#include <iostream>
#include <mutex>
#include <event.h>

ClientHandler::ClientHandler() : connectionHandler(nullptr), state(WAIT_TO_LOG_IN), username(""), receiptCounter(0), subIdCounter(0) {}

ClientHandler::ClientHandler(const ClientHandler& other): 
state(other.state), 
username(other.username),
receiptCounter(other.receiptCounter),
subIdCounter(other.subIdCounter){
    connectionHandler = new ConnectionHandler(other.connectionHandler-> host_, other.connectionHandler-> port_);
}


ClientHandler::ClientHandler(const ClientHandler&& other):
state(other.state), 
username(other.username),
receiptCounter(other.receiptCounter),
subIdCounter(other.subIdCounter),
connectionHandler(other.connectionHandler){}

ClientHandler& ClientHandler::operator=(const ClientHandler& other){
    if(this != &other){
        delete connectionHandler;
        connectionHandler = new ConnectionHandler(other.connectionHandler-> host_, other.connectionHandler-> port_);
        state = other.state; 
        username = other.username;
        receiptCounter = other.receiptCounter;
        subIdCounter = other.subIdCounter;
    }
    return *this;
}

ClientHandler& ClientHandler::operator=(const ClientHandler&& other){
    if(this != &other){
        delete connectionHandler;
        connectionHandler = other.connectionHandler;
        state = other.state; 
        username = other.username;
        receiptCounter = other.receiptCounter;
        subIdCounter = other.subIdCounter;
    }
    return *this;
}

ClientHandler::~ClientHandler(){
    delete connectionHandler;
}


void ClientHandler::setState(ClientState newstate)
{
    lock_guard<mutex> sync(statekey);
    state = newstate;
    sync.~lock_guard();
}

bool ClientHandler::cheakState(ClientState otherstate){
    lock_guard<mutex> sync(statekey);
    bool toReturn = state == otherstate;
    return toReturn;
}

bool ClientHandler::connecting()
{
    while (cheakState(ClientState::WAIT_TO_LOG_IN)){
        const short bufsize = 1024; 
        char buf[bufsize];
        cin.getline(buf, bufsize);
        string line(buf);

        vector<string> words = splitBychar(line, ' ');
        if (words[0] == "login")
        {
            if (words.size() == 4 && words.at(1).find(":") != -1)
            {
                username = words[2];
                string host = words[1].substr(0, words[1].find(":"));
                int port = stoi(words[1].substr(words[1].find(":") + 1, 4));
                connectionHandler = new ConnectionHandler(host, port);
                if (!connectionHandler->connect())
                {
                    cerr << "Cannot connect to " << host << ":" << port << std::endl;
                    return false;
                }
                setState(ClientState::WAIT_TO_CONNECT);
                string answer = "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\n";
                answer.append("login:" + username + "\n");
                answer.append("passcode:" + words[3]);
                answer.append("\n\n\u0000");
                sendframe(answer);
            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }
        else
        {
            cout << "not logged in yet send a login command" << endl;
        }
    }
    return true;
}
void ClientHandler::ProcessRequest()
{
    while (!cheakState(ClientState::DISCONNECTED) && !cheakState(ClientState::WAIT_TO_DISCONNECT))
    {
        const short bufsize = 1024; 
        char buf[bufsize];
        cin.getline(buf, bufsize);
        string line(buf);
        if(cheakState(ClientState::DISCONNECTED) || cheakState(ClientState::WAIT_TO_DISCONNECT))
            break;
        vector<string> words = splitBychar(line, ' ');
        if (words[0] == "login")
        {
            cout << "Already logged in" << endl;
        }
        else if (words[0] == "join")
        {
            if (words.size() == 2)
            {
                lock_guard<mutex> sync(datalock);
                auto it = TopicToSubid.find(words[1]);
	            if (it == TopicToSubid.end()){
                    TopicToSubid[words[1]] = subIdCounter++;
                    string answer = "SUBSCRIBE\ndestination:/" + words[1] + "\nid:" + to_string(TopicToSubid[words[1]]);
                    answer.append("\nreceipt:"+to_string(receiptCounter++)); //recipt for connect
                    answer.append("\n\n\u0000");
                    cout << "Joined channel "<< words[1] <<endl;
                    sendframe(answer);
                }
                else{
                    cout<<"allready Joined to channel : "<< words[1] <<endl;
                }
                sync.~lock_guard();

            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }
        else if (words[0] == "exit")
        {
            if (words.size() == 2)
            {
                lock_guard<mutex> sync(datalock);
                if (TopicToSubid.find(words[1]) == TopicToSubid.end())
                {
                    cout << "You are not subscribed to this topic" << endl;
                }
                else{

                    string answer = "UNSUBSCRIBE\nid:" + to_string(TopicToSubid[words[1]]);
                    answer.append("\nreceipt:" + to_string(receiptCounter++)); // recipt for exit
                    answer.append("\n\n\u0000");

                    TopicToSubid.erase(words[1]);
                    cout<< "Exited channel "<< words[1]<< endl;
                    sendframe(answer);
                }
                sync.~lock_guard();
            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }
        else if (words[0] == "summary")
        {
            if (words.size() == 4)
            {
                string output = createSummaryString(words[1], words[2]);
                if(output.compare("")!=0)
                    writeToFile(output, words[3]);
            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }
        else if (words[0] == "report")
        {
            if (words.size() == 2)
            {
                names_and_events nameAndEvents;
                try
                {
                    nameAndEvents = parseEventsFile(words[1]);
                }
                catch (const char *msg)
                {
                    return;
                }

                // not subscribed to topic
                string gamename = nameAndEvents.team_a_name + "_" + nameAndEvents.team_b_name;
                lock_guard<mutex> sync(datalock);
                auto it = TopicToSubid.find(gamename);
                if (it == TopicToSubid.end())
                {
                    cout << "you are not subscribed to this game" << endl;
                }
                else{
                    sync.~lock_guard();
                    for (auto &event : nameAndEvents.events)
                    {

                        string tosend = CreateEventFrame(event);
                        sendframe(tosend);
                        lock_guard<mutex> sync(datalock);
                        std::string Gamename = event.get_team_a_name() + "_" + event.get_team_b_name();
                        bool exist = false;
                        for (Game &g : data)
                        {
                            if (g.getGameName().compare(Gamename) == 0)
                            {
                                g.addEvent(username, tosend);
                                exist = true;

                            }
                            
                        }
                        if (!exist)
                        {
                            Game g1(Gamename);
                            g1.addEvent(username, tosend);
                            data.push_back(g1);
                        }
                    }
                    sync.~lock_guard();
                }
            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }

        else if (words[0] == "logout")
        {
            if (words.size() == 1)
            {
                setState(ClientState::WAIT_TO_DISCONNECT);
                string answer = "DISCONNECT\nreceipt:" + to_string(receiptCounter++) + "\n\n\u0000";
                sendframe(answer);

            }
            else
            {
                cout << "Wrong input" << endl;
            }
        }
        else
        {
            cout << "Wrong input" << endl;
        }
    }
}
void ClientHandler::sendframe(string frame)
{
    if (!connectionHandler->sendLine(frame))
    {
        cout << "Disconnected. Exiting...\n"
             << endl;
        setState(ClientState::DISCONNECTED);
    }
}

string ClientHandler::CreateEventFrame(const Event env)
{
    string toReturn = "SEND\ndestination:/" + env.get_team_a_name() + "_" + env.get_team_b_name() + "\n\n";
    toReturn = toReturn + "user:" + username + "\n";
    toReturn = toReturn + "event name:" + env.get_name() + "\n";
    toReturn = toReturn + "time:" + to_string(env.get_time()) + "\n";
    toReturn = toReturn + "general game updates:" + "\n";
    for (pair<string, string> elem : env.get_game_updates())
    {
        toReturn = toReturn + "\t" + elem.first + ": " + elem.second + "\n";
    }
    toReturn = toReturn + "team a updates:\n";
    for (pair<string, string> elem : env.get_team_a_updates())
    {
        toReturn = toReturn + "    " + elem.first + ": " + elem.second + "\n";
    }
    toReturn = toReturn + "team b updates:\n";
    for (pair<string, std::string> elem : env.get_team_b_updates())
    {
        toReturn = toReturn + "    " + elem.first + ": " + elem.second + "\n";
    }

    toReturn = toReturn + "description:\n" + env.get_discription();
    return toReturn;
}

vector<string> ClientHandler::splitBychar(const string &frame, char delimiter)
{
    stringstream ss(frame);
    vector<string> toReturn;
    string tmp;
    while (getline(ss, tmp, delimiter))
    {
        toReturn.push_back(tmp);
    }
    return toReturn;
}

string ClientHandler::createSummaryString(const string &Gamename, const string &reportermane)
{
    lock_guard<mutex> sync(datalock);
    auto it = TopicToSubid.find(Gamename);
    if (it == TopicToSubid.end())
    {
        cout << "you are not subscribed to this game: " << Gamename << endl;
        return "";
    }
    vector<string> eventsToReturn;
    for (Game &gm : data)
    {
        if (gm.getGameName().compare(Gamename) == 0)
        {    

            eventsToReturn = gm.getMsgsByuserName(reportermane,data);

        }
    }
    sync.~lock_guard();

    if(eventsToReturn.size()==0){
        cout<< "no games reported by "<< reportermane<<" for game "<<Gamename<<endl;
        return "";
    }
    map<string, vector<pair<string, string>>> eventMap;
    pair<string, vector<pair<string, string>>> generalStats;
    pair<string, vector<pair<string, string>>> teamAstats;
    pair<string, vector<pair<string, string>>> teamBstats;
    pair<string, vector<pair<string, string>>> gameEventReports;
    generalStats.first = "General stats:";
    generalStats.second = vector<pair<string, string>>();
    teamAstats.first = "Team a stats:";
    teamAstats.second = vector<pair<string, string>>();
    teamBstats.first = "Team b stats:";
    teamBstats.second = vector<pair<string, string>>();
    gameEventReports.first = "Game event reports:";
    gameEventReports.second = vector<pair<string, string>>();
    
    eventMap.insert(generalStats);
    eventMap.insert(teamAstats);
    eventMap.insert(teamBstats);
    eventMap.insert(gameEventReports);
    

    for (string event : eventsToReturn)
    {
        map<string, vector<pair<string, string>>> tmpMap;
        parseEvent(event,tmpMap);
        updateMap(eventMap, tmpMap);

    }
    string finalString = mapToString(eventMap, Gamename);
    return finalString;
}

void ClientHandler::updateMap(map<string, vector<pair<string, string>>> &eventMap, map<string, vector<pair<string, string>>> &tmpMap)
{
    vector<pair<string, string>> tmpvec;
    for (pair<string, vector<pair<string, string>>> elem : tmpMap)
    {
        for (unsigned int i = 0; i < elem.second.size(); i++)
        {
            for (pair<string, vector<pair<string, string>>> section : eventMap){
                {
                if(section.first.compare(elem.first)==0){
                    bool found = false;
                    for (unsigned int j = 0; j < section.second.size(); j++)
                    {
                        if ((elem.second[i].first.compare(section.second[j].first) == 0)&&(section.first.compare("Game event reports:") != 0))
                        {
                            eventMap.at(elem.first)[j].second = elem.second[i].second;
                            section.second[j].second = elem.second[i].second; 
                            found = true;
                        }else if(section.first.compare("Game event reports:")==0&&(!found)){

                            eventMap.at(elem.first).push_back(elem.second[i]);
                            found = true;
                        }
                    }
                    if (!found){
                        eventMap.at(elem.first).push_back(elem.second[i]);
                        section.second.push_back(elem.second[i]);
                    }
                }
                }
                
            }
        }
    }
}

string ClientHandler::mapToString(map<string, vector<pair<string, string>>> eventMap, string gamename)
{
    string teamA = splitBychar(gamename, '_').front();
    string teamB = splitBychar(gamename, '_').back();
    string gamenametodisp = splitBychar(gamename, '_').front() + " vs " + splitBychar(gamename, '_').back();
    string towrite = gamenametodisp + "\nGame stats:\n";

    vector<pair<string, string>> elem = eventMap.at("General stats:");
    towrite = towrite + "General stats:" + "\n";
    for (unsigned int i = 0; i < elem.size(); i++)
    {
            towrite += elem[i].first + ": " + elem[i].second + "\n";
    }
    elem = eventMap.at("Team a stats:");
    towrite = towrite + teamA +" stats:" + "\n";
    for (unsigned int i = 0; i < elem.size(); i++)
    {
            towrite += elem[i].first + ": " + elem[i].second + "\n";
    }
    elem = eventMap.at("Team b stats:");
    towrite = towrite + teamB +" stats:" + "\n";
    for (unsigned int i = 0; i < elem.size(); i++)
    {
            towrite += elem[i].first + ": " + elem[i].second + "\n";
    }
    elem = eventMap.at("Game event reports:");
    towrite = towrite + "Game event reports:" + "\n";
    for (unsigned int i = 0; i < elem.size(); i++)
    {
        if(elem[i].first.compare("description:")!=0)
            towrite += elem[i].first + " - " + elem[i].second + ":\n";
            else
            towrite += "\n" + elem[i].second + "\n\n";
    }
    
    return towrite;
}

void ClientHandler::parseEvent(string event,map<string, vector<pair<string, string>>> &emap)
{
    pair<string, vector<pair<string, string>>> generalStats;
    pair<string, vector<pair<string, string>>> teamAstats;
    pair<string, vector<pair<string, string>>> teamBstats;
    pair<string, vector<pair<string, string>>> gameEventReports;
    generalStats.first = "General stats:";
    generalStats.second = vector<pair<string, string>>();
    teamAstats.first = "Team a stats:";
    teamAstats.second = vector<pair<string, string>>();
    teamBstats.first = "Team b stats:";
    teamBstats.second = vector<pair<string, string>>();
    gameEventReports.first = "Game event reports:";
    gameEventReports.second = vector<pair<string, string>>();
    vector<string> lines = splitBychar(event, '\n');
    string eventname = findString(event, "event name:", '\n').front();
    string eventtime = findString(event, "time:", '\n').front();
    pair<string, string> pairname;
    pairname.second = eventname;
    pairname.first = eventtime;
    gameEventReports.second.push_back(pairname);
    int index = 0;
    for (unsigned int i = 0; i < lines.size(); i++)
    {
        if (lines[i].compare("general game updates:")==0)
            index = 1;
        if (lines[i].compare("team a updates:")==0)
            index = 2;
        if (lines[i].compare("team b updates:")==0)
            index = 3;
        if (lines[i].compare("description:")==0)
            index = 4;

        if (index == 1)
        {
            vector<string> statvec = splitBychar(lines[i], ':');
            if (statvec.size() !=1)
            {
                pair<string, string> pairvec;
                pairvec.first = statvec[0];
                pairvec.second = statvec[1];
                generalStats.second.push_back(pairvec);
            }
        }
        if (index == 2)
        {
            vector<string> statvec = splitBychar(lines[i], ':');
            if (statvec.size() !=1)
            {
                pair<string, string> pairvec;
                pairvec.first = statvec[0];
                pairvec.second = statvec[1];
                teamAstats.second.push_back(pairvec);
            }
        }
        if (index == 3)
        {
            vector<string> statvec = splitBychar(lines[i], ':');
            if (statvec.size() !=1)
            {
                pair<string, string> pairvec;
                pairvec.first = statvec[0];
                pairvec.second = statvec[1];
                teamBstats.second.push_back(pairvec);
            }
        }
        if (index == 4)
        {
            index = 5;
            vector<string> statvec = splitBychar(lines[i], ':');
            if (statvec.size() !=1)
            {
                pair<string, string> pairvec;
                pairvec.first = statvec[0];
                pairvec.second = statvec[1];
                gameEventReports.second.push_back(pairvec);
            }
        }else if(index == 5&& (lines[i].compare("\n")!=0)){
            pair<string, string> pairvec = {"description:",lines[i]};
            gameEventReports.second.push_back(pairvec);
        }

    }
    emap.insert(generalStats);
    emap.insert(teamAstats);
    emap.insert(teamBstats);
    emap.insert(gameEventReports);
}

vector<string> ClientHandler::findString(string main, string secondery, char delimiter)
{
    vector<string> toReturn;
    string toPush = "";
    unsigned int pos = main.find(secondery);
    if (pos != string::npos)
    {
        pos = pos + secondery.length();
        while (main.at(pos) != delimiter)
        {
            toPush += main.at(pos);
            pos++;
        }
    }
    toReturn.push_back(toPush);
    return toReturn;
}

void ClientHandler::parseFrame()
{
    while (!cheakState(ClientState::DISCONNECTED)){
        string frameFromServer;
        if (!connectionHandler->getLine(frameFromServer))
        {
            std::cout << "Disconnected Exiting...\n"
                      << std::endl;
            setState(ClientState::DISCONNECTED);
        }

        int len = frameFromServer.length();
        vector<string> lines = splitBychar(frameFromServer, '\n');
        string command; 
        command = "null"; 
        if (lines.size()>=1)
            command = lines[0];
        string body = frameFromServer.substr(frameFromServer.find('\n') + 1, len);
        if (command.compare("MESSAGE") == 0)
        {
            map<string, vector<pair<string, string>>> eventStr;
            parseEvent(frameFromServer,eventStr);
            string reportername = findString(frameFromServer, "user:", '\n').front();
            if (reportername.compare(username)!=0)
            {
                string gamename = findString(frameFromServer, "destination:/", '\n').front();
                lock_guard<mutex> sync(datalock);
                bool exist = false;
                for (Game &g : data)
                {
                    if (g.getGameName().compare(gamename) == 0)
                    {
                        g.addEvent(reportername, frameFromServer);
                        exist = true;
                    }
                }
                if (!exist)
                {
                    Game g1(gamename);
                    g1.addEvent(reportername, frameFromServer);
                    data.push_back(g1);
                }
                sync.~lock_guard();
            }
        }
        else if (command.compare("RECEIPT") == 0)
        {
            if (cheakState(ClientState::WAIT_TO_DISCONNECT) && (stoi(lines[1].substr(lines[1].find("receipt-id") + 11, lines[1].size() - 1))==receiptCounter-1))
            {
                setState(ClientState::DISCONNECTED);
                connectionHandler->close();

            }
        }
        else if (command.compare("ERROR") == 0)
        {
            string error = body.substr(0, body.find('\n'));
            cout << error << endl;
            setState(ClientState::DISCONNECTED);
            connectionHandler->close();
        }
        else if (command.compare("CONNECTED") == 0)
        {
            if (cheakState(ClientState::WAIT_TO_CONNECT))
            {
                cout<<"connected to server"<< endl;
                setState(ClientState::CONNECTED);
                break;
            }
        }
    }
}







