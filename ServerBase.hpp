//
// Created by Kian on 2021/8/18.
//

#ifndef EASYWEBSERVER_SERVER_BASE_H
#define EASYWEBSERVER_SERVER_BASE_H

#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <regex>

#include "boost/asio.hpp"

namespace EasyWeb{

    struct Request{
        // 请求方法，POST、GET；请求路径；http版本
        std::string method, path, http_version;
        // 使用智能指针记录content
        std::shared_ptr<std::istream> content;
        // 使用哈希容器记录header，不要求顺序，因此使用无序的key-value容器
        std::unordered_map<std::string, std::string> header;
        // 路径匹配规则
        std::smatch path_match;
    };

    // 服务器资源类型，决定了用户如何使用这个库
    /*
     * std::map 用于存储请求路径的正则表达式
     * std::unordered_map用于存储对应请求方法的处理方法
     * 函数用lambda匿名函数保存
     * 通过这种方式，可以在使用时方便地定义想要的方法。
     *
     * 使用方式可以如下：
     * 处理访问 /info 的 GET 请求, 返回请求的信息
     * server.resource["^/info/?$"]["GET"] = [](ostream& response, Request& request) {
     *     处理请求及资源
     *     ...
     *     };
     */
    typedef std::map<std::string, std::unordered_map<std::string,
    std::function<void(std::ostream&, Request&)>>> resource_type;

    // socket type有HTTP和HTTPS两种
    template <typename socket_type>
    class ServerBase {
    public:
        // 构造函数
        ServerBase(unsigned short port=8820, size_t num_threads=1):
        endpoint(boost::asio::ip::tcp::v4(), port),
        acceptor(m_io_service, endpoint),
        num_threads(num_threads) {}


        // 用于启动服务，设置为public，因为要被调用
        void start();

        // 用于服务器访问资源处理方式
        resource_type resource;
        // 用于保存默认资源的处理方式
        resource_type default_resource;

    protected:
        // Boost::asio库要求每个应用具有一个io_service调度器
        // 实现TCP连接，需要一个对应的acceptor，初始化acceptor需要socket和endpoint
        // asio 库中的 io_service 是调度器，所有的异步 IO 事件都要通过它来分发处理
        // 换句话说, 需要 IO 的对象的构造函数，都需要传入一个 io_service 对象
        boost::asio::io_service m_io_service;
        // IP 地址、端口号、协议版本构成一个 endpoint，并通过这个 endpoint 在服务端生成
        // tcp::acceptor 对象，并在指定端口上等待连接
        boost::asio::ip::tcp::endpoint endpoint;
        // 所以，一个 acceptor 对象的构造都需要 io_service 和 endpoint 两个参数
        boost::asio::ip::tcp::acceptor acceptor;

        // 不同socket type需要有不同的实现，因此设置为虚函数
        virtual void accept(){ };

        // 处理请求和应答，这部分与socket type无关，因此放到基类中实现
        void process_request_and_reponse(std::shared_ptr<socket_type> socket) const;

        // 所有的资源及默认资源都会在 vector 尾部添加, 并在 start() 中创建
        std::vector<resource_type::iterator> all_resources;

        // 为了实现服务器同时应对多个连接，因此设置线程池
        size_t num_threads;
        std::vector<std::thread> threads;
    };


}

#endif //EASYWEBSERVER_SERVER_BASE_H
