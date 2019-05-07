#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <mutex>
#include <cassert>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <protocol/Parser.h>

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps) : _socket(s), pStorage(ps) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    enum class State { Alive, Dead };

    struct Masks {
        static const int read = ((EPOLLIN | EPOLLRDHUP) | EPOLLERR);
        static const int write = ((EPOLLOUT | EPOLLRDHUP) | EPOLLERR);
        static const int read_write = (((EPOLLIN | EPOLLRDHUP) | EPOLLERR) | EPOLLOUT);
    };

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
    friend class Worker;
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    State state = State::Alive;
    std::mutex _lock;

    // all we need to read and do commands
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> pStorage;

    // save state of reading
    int _written_bytes;
    int _readed_bytes;
    int new_bytes;
    char client_buffer[4096];

    std::vector<std::string> results_to_write;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
