#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>


class Game {
private:
    std::string name;
    std::map<std::string, std::vector<std::string>> usersToMsgs;

public:
    Game (std::string name);
    std::vector<std::string> &getMsgsByuserName (std::string name,std::vector<Game> data);
    std::string getGameName();
    void addEvent(std::string name,std::string msg);
    std::vector<std::string> betweenTwoFaram (std::string &msg,std::string &param1,std::string &param2);
    std::map<std::string, std::vector<std::string>> getusersToMsgs();

};

