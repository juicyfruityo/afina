#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <protocol/Parser.h>
#include <afina/Storage.h>
#include <afina/execute/Command.h>

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    enum class State {
        Embryo,
        Alive,
        Dead
    };

    Connection(int s) : _socket(s) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const {
      if (state == State::Alive) {
          return true;
      }
      return false;
    }

    void Start(std::shared_ptr<Afina::Storage> ps);

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;
    State state = State::Embryo;
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> pStorage;
    std::vector<std::string> results_to_write;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
