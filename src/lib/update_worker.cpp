#include "command_line_runner.h"
#include "update_worker.h"
#include <iostream>

namespace upd {

update_worker::update_worker(
  worker_status& status,
  update_job& job,
  std::mutex& mutex,
  std::condition_variable& output_cv
):
  status_(status),
  job_(job),
  mutex_(mutex),
  output_cv_(output_cv),
  thread_(&update_worker::run_, this) {}

void update_worker::run_() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (status_ != worker_status::shutdown) {
    if (status_ != worker_status::in_progress) {
      cv_.wait(lock);
      continue;
    }
    status_ = worker_status::in_progress;
    mutex_.unlock();
    command_line_runner::run(job_.root_path, job_.target, job_.depfile_fds);
    mutex_.lock();
    status_ = worker_status::finished;
    output_cv_.notify_all();
  }
}

void update_worker::notify() {
  cv_.notify_one();
}

void update_worker::join() {
  thread_.join();
}

}
