//
// Created by Kian on 2021/8/18.
//

#ifndef EASYWEBSERVER_SERVER_BASE_H
#define EASYWEBSERVER_SERVER_BASE_H

#include <memory>

namespace EasyWeb{
    // socket type有HTTP和HTTPS两种
    template <typename socket_type>
    class server_base {
    public:
        // 用于启动服务，设置为public，因为要被调用
        start();

    protected:
        // 不同socket type需要有不同的实现，因此设置为虚函数
        virtual void accept(){ };

        // 处理请求和应答，这部分与socket type无关，因此放到基类中实现
        void process_request_and_reponse(std::shared_ptr<socket_type> socket) const;
    };
}

#endif //EASYWEBSERVER_SERVER_BASE_H
