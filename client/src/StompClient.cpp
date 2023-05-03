#include <iostream>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <ClientHandler.h>

using namespace std;

int main (int argc, char *argv[]) {
    ClientHandler* clientHandle = new ClientHandler();
    while(true){
        if(!clientHandle->connecting()) return -1;
        clientHandle->parseFrame();
        std::thread write_th(&ClientHandler::ProcessRequest, &(*clientHandle));
        clientHandle->parseFrame();
        write_th.join();
        clientHandle->setState(WAIT_TO_LOG_IN);
    }
    
    delete clientHandle;
    return 0;
}