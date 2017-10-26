#pragma once

#include "run_command_line.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace upd {

struct update_job {
  std::string root_path;
  command_line target;
  int depfile_fds[2];
};

enum class worker_status { idle, in_progress, finished, shutdown };

/**
 * Because we start a thread referencing the internal condition variable,
 * instances cannot be moved (not copied).
 */
struct update_worker {
  update_worker(worker_status &status, command_line_result &result,
                update_job &job, std::mutex &mutex,
                std::condition_variable &output_cv);
  update_worker(update_worker &) = delete;
  update_worker(update_worker &&) = delete;
  void notify();
  void join();

private:
  void run_();

  worker_status &status_;
  command_line_result &result_;
  update_job &job_;
  std::mutex &mutex_;
  std::condition_variable &output_cv_;
  std::condition_variable cv_;
  std::thread thread_;
};

} // namespace upd
