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
#include <thread>

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
        // 构造函数若可以只接受一个参数，则需要声明显式explicit，
        // 因为若有重载，在调用时参数类型可能被编译器隐式转化，导致使用错误的构造函数。
        explicit ServerBase(unsigned short port=8820, size_t num_threads=1):
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

        // 解析请求的信息
        Request parse_request(std::istream& istream) const;
        // 处理请求和应答，这部分与socket type无关，因此放到基类中实现
        void process_request_and_respond(std::shared_ptr<socket_type> socket) const;

        void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const;

        // 所有的资源及默认资源都会在 vector 尾部添加, 并在 start() 中创建
        std::vector<resource_type::iterator> all_resources;

        // 为了实现服务器同时应对多个连接，因此设置线程池
        size_t num_threads;
        std::vector<std::thread> threads;
    };

    template<typename socket_type>
    void ServerBase<socket_type>::start() {
        // 默认资源放在 vector 的末尾, 用作默认应答
        for(auto iter=resource.begin(); iter!= resource.end();++iter){
            all_resources.push_back(iter);
        }
        for(auto iter=default_resource.begin(); iter!= default_resource.end();++iter){
            all_resources.push_back(iter);
        }

        //调用accept()方式
        accept();

        // 运行线程,若为多线程，则运行n-1个线程
        for(size_t n = 1; n < num_threads; ++ n){
            // emplace_back() 在实现时，则是直接在容器尾部创建这个元素，省去了拷贝或移动元素的过程
            threads.emplace_back([this](){
                m_io_service.run();
            });
        }

        // 运行主线程
        m_io_service.run();
        // 添加其他线程
        for (auto & t : threads){
            t.join();
        }

    }

    template<typename socket_type>
    Request ServerBase<socket_type>::parse_request(std::istream &istream) const {
//        std::istreambuf_iterator<char> eos;
//        std::string str(std::istreambuf_iterator<char>(istream), eos);
        Request request;

        // 用于解析请求方法(GET/POST)、请求路径以及 HTTP 版本
        // 即每个请求报文的第一行，method path version
        // 第一个^为正则表达式的开始，$为结束，(expr)表示expr子模式，[^x]表示匹配除了x之外的字符，*表示多个
        std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch sub_match;

        // 按行解析
        std::string line;
        // 从istream中获取第一行，并删除string容器中的最后一个空白符号
        getline(istream, line);
        std::cout<<line;
        line.pop_back();
        if(std::regex_match(line, sub_match, e)){
            request.method = sub_match[1];
            request.path = sub_match[2];
            request.http_version = sub_match[3];

            // 继续解析Request Header剩余部分
            bool matched;
            // header信息
            e = "^([^:]*): ?(.*)$";
            do{
                getline(istream, line);
                line.pop_back();
                matched = std::regex_match(line, sub_match, e);
                if(matched){
                    request.header[sub_match[1]] = sub_match[2];
                }
            } while (matched==true);
        }
        return request;
    }

    template<typename socket_type>
    void ServerBase<socket_type>::process_request_and_respond(std::shared_ptr<socket_type> socket) const {
        // 为 async_read_untile() 创建新的读缓存
        // shared_ptr 用于传递临时对象给匿名函数
        // auto 会被推导为 std::shared_ptr<boost::asio::streambuf>
        auto read_buffer = std::make_shared<boost::asio::streambuf>();

        // async_read_until异步读取
        // 给定一个streambuf和一个分隔符，asio遇到分隔符时返回，
        // 首先从streambuf中读取请求头的内容（\r\n\r\n为分界）
        // 接下去读取剩余部分
        // bytes_transferred是指async_read_until读到分隔符为止的长度
        // 匿名函数用于读取
        boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
                                      [this, socket, read_buffer](
                                              const boost::system::error_code& ec,
                                              size_t bytes_transferred){
            if (!ec){
                // stream中接收的总长度
                size_t total = read_buffer->size();

                // convert from streambuf to istream
                std::istream stream(read_buffer.get());

                // 首先读取request中的header
                // request 智能指针指向为Request对象创建的区域
                auto request = std::make_shared<Request>();
                // 赋值
                *request = parse_request(stream);

                size_t num_additional_bytes = total - bytes_transferred;
                // 判断请求头中指示的请求体内容长度
                if(request->header.count("Content-Length")>0){
                    boost::asio::async_read(*socket, *read_buffer,
                    // read or write until an exact number of bytes has been transferred
                    boost::asio::transfer_exactly(
                            stoull(request->header["Content-Length"]) - num_additional_bytes),
                            [this, socket, read_buffer, request](const boost::system::error_code& ec,
                                    size_t bytes_transferred){
                                if(!ec){
                                    request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
                                    respond(socket, request);
                                }
                            });
                } else{
                    respond(socket, request);
                }
            }
        });
    }

    template<typename socket_type>
    void ServerBase<socket_type>::respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const{
        // 对请求路径和方法进行匹配查找，并生成响应
        for (auto res_iter: all_resources){
            std::regex e(res_iter->first);
            std::smatch sm_res;
            if(std::regex_match(request->path, sm_res, e)){
                // 若请求路径存在，且有对应的请求方法，则进行响应
                if(res_iter->second.count(request->method) > 0){
                    // TODO
                    // move 右值引用？？？？
                    request->path_match = move(sm_res);
                    auto write_buffer = std::make_shared<boost::asio::streambuf>();
                    std::ostream response(write_buffer.get());
                    res_iter->second[request->method](response, *request);

                    // ????
                    boost::asio::async_write(
                            *socket, *write_buffer,
                            [this, socket, request, write_buffer]
                            (const boost::system::error_code& ec, size_t bytes_transferred){
                                // HTTP 持久连接(HTTP 1.1), 递归调用
                                if(!ec && stof(request->http_version)>1.05)
                                    process_request_and_respond(socket);
                            });
                    return;
                }
            }
        }

    }

}

#endif //EASYWEBSERVER_SERVER_BASE_H
