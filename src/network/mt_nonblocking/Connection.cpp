#include "Connection.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/logger.h>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <afina/logging/Service.h>

#include "protocol/Parser.h"

namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start() {
    // Start() calls only once by one thread so we don't need lock
    state = State::Alive;
    command_to_execute.reset();
    argument_for_command.resize(0);
    parser.Reset();
    results_to_write.clear();
    arg_remains = 0;
    _written_bytes = 0;
    _readed_bytes = 0;
    _event.events = Masks::read;
}

// See Connection.h
void Connection::OnError() {
    std::unique_lock<std::mutex> lock(_lock);
    state = State::Dead;
}

// See Connection.h
void Connection::OnClose() {
    std::unique_lock<std::mutex> lock(_lock);
    state = State::Dead;
}

// See Connection.h
void Connection::DoRead() {
  std::unique_lock<std::mutex> lock(_lock);
  int client_socket = _socket;
  command_to_execute = nullptr;
  try {
      while ((new_bytes = read(client_socket, client_buffer + _readed_bytes, sizeof(client_buffer) - _readed_bytes)) >
             0) {
          _readed_bytes += new_bytes;
          // Single block of data read from the socket could trigger inside actions a multiple times,
          // for example:
          // - read#0: [<command1 start>]
          // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
          while (_readed_bytes > 0) {
              // There is no command yet
              if (!command_to_execute) {
                  std::size_t parsed = 0;
                  if (parser.Parse(client_buffer, _readed_bytes, parsed)) {
                      // There is no command to be launched, continue to parse input stream
                      // Here we are, current chunk finished some command, process it
                      command_to_execute = parser.Build(arg_remains);
                      if (arg_remains > 0) {
                          arg_remains += 2;
                      }
                  }

                  // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                  // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                  if (parsed == 0) {
                      break;
                  } else {
                      std::memmove(client_buffer, client_buffer + parsed, _readed_bytes - parsed);
                      _readed_bytes -= parsed;
                  }
              }

              // There is command, but we still wait for argument to arrive...
              if (command_to_execute && arg_remains > 0) {
                  // There is some parsed command, and now we are reading argument
                  std::size_t to_read = std::min(arg_remains, std::size_t(_readed_bytes));
                  argument_for_command.append(client_buffer, to_read);

                  std::memmove(client_buffer, client_buffer + to_read, _readed_bytes - to_read);
                  arg_remains -= to_read;
                  _readed_bytes -= to_read;
              }

              // There is command & argument - RUN!
              if (command_to_execute && arg_remains == 0) {
                  std::string result_to_write;
                  command_to_execute->Execute(*pStorage, argument_for_command, result_to_write);
                  result_to_write += "\r\n";
                  // Save results for better time
                  results_to_write.push_back(result_to_write);

                  // Prepare for the next command
                  command_to_execute.reset();
                  argument_for_command.resize(0);
                  parser.Reset();

                  _event.events = Masks::read_write;
              }
          } // while (read_bytes)
      }
      if (_readed_bytes > 0) {
          throw std::runtime_error(std::string(strerror(errno)));
      }
  } catch (std::runtime_error &ex) {
  }
}

// See Connection.h
void Connection::DoWrite() {
  std::unique_lock<std::mutex> lock(_lock);
  static int max_iov = 128;
  int results_num = results_to_write.size();
  // assert(results_num > 0);

  auto results_it = results_to_write.begin();
  struct iovec results_iov[max_iov];

  for (int i = 0; i < max_iov && i < results_num; ++i, ++results_it) {
      results_iov[i].iov_base = &(*results_it)[0];
      results_iov[i].iov_len = (*results_it).size();
  }
  results_iov[0].iov_base = results_iov[0].iov_base + _written_bytes;
  results_iov[0].iov_len -= _written_bytes;

  int written = writev(_socket, results_iov, results_num);
  _written_bytes += written;

  auto end_it = results_to_write.begin();
  for (auto it=&results_iov[0]; _written_bytes > (*it).iov_len; ++it, ++end_it) {
      _written_bytes -= (*it).iov_len;
  }

  results_to_write.erase(results_to_write.begin(), end_it);
  if (results_to_write.size() == 0) {
      _event.events = Masks::read;
  }
}

} // namespace MTnonblock
} // namespace Network
} // namespace Afina
