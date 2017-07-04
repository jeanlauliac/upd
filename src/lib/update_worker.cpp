#include "command_line_runner.h"
#include "update_worker.h"

namespace upd {

update_worker::update_worker():
  running_(true), thread_(&update_worker::start_update_thread_, this) {}

update_worker::~update_worker() {
  {
    std::lock_guard<std::mutex> lg(mutex_);
    running_ = false;
  }
  cv_.notify_one();
  thread_.join();
}

void update_worker::start_update_thread_() {
  std::unique_lock<std::mutex> lk(mutex_);
  while (running_) {
    if (!job_) {
      cv_.wait(lk);
      continue;
    }
    command_line_runner::run(job_->root_path, job_->target, job_->depfile_fds);
    job_.reset();
    cv_.notify_one();
  }
}

void update_worker::process(update_job job) {
  {
    std::lock_guard<std::mutex> lg(mutex_);
    job_ = std::make_unique<update_job>(job);
  }
  cv_.notify_one();
  {
    std::unique_lock<std::mutex> lk(mutex_);
    while (job_) cv_.wait(lk);
  }
}

}
