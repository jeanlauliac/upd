#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace upd {

struct update_job {
  const std::string root_path;
  command_line target;
  int depfile_fds[2];
};

struct update_worker {
  update_worker();
  ~update_worker();
  update_worker(update_worker&) = delete;
  void process(update_job job);

private:
  void start_update_thread_();

  std::mutex mutex_;
  std::condition_variable cv_;
  bool running_;
  std::unique_ptr<update_job> job_;
  std::thread thread_;
};

}
