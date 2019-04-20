#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <protocol/Parser.h>

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    enum class State { Alive, Dead };

    struct Masks {
        static const int read = ((EPOLLIN | EPOLLRDHUP) | EPOLLERR);
        static const int write = ((EPOLLOUT | EPOLLRDHUP) | EPOLLERR);
        static const int read_write = (((EPOLLIN | EPOLLRDHUP) | EPOLLERR) | EPOLLOUT);
    };

    Connection(int s, std::shared_ptr<Afina::Storage> ps) : _socket(s), pStorage(ps) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const {
        if (state == State::Alive) {
            return true;
        }
        return false;
    }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    State state = State::Alive;

    // всё что нам как обычно надо для чтения и выполнения команд
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> pStorage;

    // сохраняем состояние чтения
    int _written_bytes;
    int _readed_bytes;
    int new_bytes;
    char client_buffer[4096];

    std::vector<std::string> results_to_write;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
