//
// Created by Kian on 2021/8/23.
//

#ifndef EASYWEBSERVER_SERVER_HTTP_H
#define EASYWEBSERVER_SERVER_HTTP_H

#include "server_base.hpp"

namespace EasyWeb{
    typedef boost::asio::ip::tcp::socket HTTP;

    // 继承模板类
    class HttpServer : public ServerBase<HTTP>{
    public:
        // 通过端口号、线程数来构造 Web 服务器, HTTP 服务器比较简单，不需要做相关配置文件的初始化
        explicit HttpServer(unsigned int port=8820, size_t num_threads=1): ServerBase<HTTP>(port, num_threads){};
    private:
        // 实现accept方法
        void accept() override{
            // 为当前连接创建一个新的 socket
            // Shared_ptr 用于传递临时对象给匿名函数
            // socket 会被推导为 std::shared_ptr<HTTP> 类型
            auto socket = std::make_shared<HTTP>(m_io_service);

            acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
                // 立即启动并接受一个连接
                accept();
                // 如果出现错误
                if(!ec) process_request_and_respond(socket);
            });

        }
    };

}

#endif //EASYWEBSERVER_SERVER_HTTP_H
