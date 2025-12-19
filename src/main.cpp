#include "server/WebServer.h"
#include <iostream>

int main(int argc, char* argv[]){
    if(argc!= 2){
        //参数不合法
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }
    int port = atoi(argv[1]);
    WebServer server(port);
    if(!server.Start()){
        //启动失败
        std::cout << "服务器启动失败" << std::endl;
        return 1;
    }
    return 0;
}