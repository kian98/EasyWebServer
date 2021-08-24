//
// Created by Kian on 2021/8/15.
//

#include <iostream>
#include "server_http.hpp"
#include "handler.hpp"

using namespace EasyWeb;

int main(){
    HttpServer server(12345, 4);
    std::cout << "Server starting at port: 12345" << std::endl;
    start_server<HttpServer>(server);
    return 0;
}