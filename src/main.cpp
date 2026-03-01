#include "server/WebServer.h"
#include <iostream>
#include <sys/signalfd.h>
#include <signal.h>

int main(int argc, char* argv[]){
    if(argc!= 2){
        //参数不合法
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);

    if(sigprocmask(SIG_BLOCK, &sigset, nullptr) == -1){
        std::cerr << "sigprocmask error" << std::endl;
        return 1;
    }
    int port = atoi(argv[1]);
    Logger::Instance().Init("./logs/webserver.log", LogLevel::INFO);
    WebServer server(port);
    if(!server.Start()){
        //启动失败
        std::cout << "服务器启动失败" << std::endl;
        return 1;
    }
    return 0;
}