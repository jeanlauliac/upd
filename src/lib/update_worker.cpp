#include "update_worker.h"
#include "run_command_line.h"
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

namespace upd {

pseudoterminal::pseudoterminal() {
  fd_ = posix_openpt(O_RDWR | O_NOCTTY);
  if (fd_ < 0) throw std::runtime_error("could not open pseudoterminal");
  if (grantpt(fd_) != 0) {
    throw std::runtime_error("could not grantpt()");
  }
  if (unlockpt(fd_) != 0) {
    throw std::runtime_error("could not unlockpt()");
  }
  ptsname_ = ::ptsname(fd_);
}

pseudoterminal::~pseudoterminal() { close(fd_); }

update_worker::update_worker(worker_status &status, command_line_result &result,
                             update_job &job, std::mutex &mutex,
                             std::condition_variable &output_cv)
    : status_(status), result_(result), job_(job), mutex_(mutex),
      output_cv_(output_cv), thread_(&update_worker::run_, this) {}

void update_worker::run_() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (status_ != worker_status::shutdown) {
    if (status_ != worker_status::in_progress) {
      cv_.wait(lock);
      continue;
    }
    status_ = worker_status::in_progress;
    mutex_.unlock();
    std::exception_ptr eptr;
    auto result =
        run_command_line(job_.target, stderr_pty_.fd(), stderr_pty_.ptsname());
    mutex_.lock();
    result_ = std::move(result);
    status_ = worker_status::finished;
    output_cv_.notify_all();
  }
}

void update_worker::notify() { cv_.notify_one(); }

void update_worker::join() { thread_.join(); }

} // namespace upd
