#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

Executor::Executor(std::string name, int low_watermark, int high_watermark,
                   int max_queue_size, int idle_time):
                   _name(name), _low_watermark(low_watermark),
                   _high_watermark(high_watermark), _max_queue_size(max_queue_size),
                   _idle_time(idle_time) { }

Executor::~Executor() {}

void Executor::Start() {
  _state = State::kRun;
  _free_threads = 0;
  for (int i=0; i<_low_watermark; ++i) {
      _curr_threads++;
      std::thread _work(&_perform_task, this);
      _work.detach();
  }
}

void Executor::Stop(bool await) {
  std::unique_lock<std::mutex> lock(_mutex);
  _state = State::kStopping;
  empty_condition.notify_all();

  while(!_threads.empty()) {
      _stop_cv.wait(lock);
  }
  _state = State::kStopped;
}

void Executor::_add_thread() {
  std::thread _work(&_perform_task, this);
  _work.detach();
}

void _perform_task(Executor *exec) {
  while (exec->_state == Executor::State::kRun) {
      std::function<void()> task;
      auto time_until = std::chrono::system_clock::now()
                      + std::chrono::milliseconds(exec->_idle_time);

      {
        std::unique_lock<std::mutex> lock(exec->_mutex);
        while (exec->_tasks.empty() && exec->_state == Executor::State::kRun) {
            exec->_free_threads++;
            if (exec->empty_condition.wait_until(lock, time_until) == std::cv_status::timeout) {
                if (exec->_curr_threads > exec->_low_watermark) {
                    exec->_curr_threads--;
                    return ;
                } else {
                    exec->empty_condition.wait(lock);
                }
            }
            exec->_free_threads--;
        }
        task = exec->_tasks.front();
        exec->_tasks.pop_front();
      }

      task();
  }

  {
    std::unique_lock<std::mutex> lock(exec->_mutex);
    exec->_curr_threads--;
    if (exec->_curr_threads == 0) {
        exec->_stop_cv.notify_all();
    }
  }
}

}
} // namespace Afina
