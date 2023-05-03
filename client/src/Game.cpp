
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <Game.h>
#include <event.h>
#include <algorithm>

Game::Game(std::string name):name(name),usersToMsgs()      
{
}



std::vector<std::string> &Game::getMsgsByuserName (std::string username,std::vector<Game> data)
{
    auto it = usersToMsgs.find(username);
	if (it != usersToMsgs.end()){
    	return it->second;
    }
	else {
        std::vector<std::string> emptyVec;
        return emptyVec;
	}
}

std::string Game::getGameName(){
    return name;
}

std::map<std::string, std::vector<std::string>> Game::getusersToMsgs(){
    return usersToMsgs;
}


void Game::addEvent(std::string username,std::string msg){
    auto it = usersToMsgs.find(username);
	if (it == usersToMsgs.end()){
    	std::pair<std::string,std::vector<std::string>> newpair;
        newpair.first = username;
        std::vector<std::string> newMsgVec;
        newMsgVec.push_back(msg);
        newpair.second = newMsgVec;
		usersToMsgs.insert(newpair);
    }
	else {
        usersToMsgs.at(username).push_back(msg);
	}
}


