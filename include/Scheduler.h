#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "JobManager.h"
#include <queue>
#include <string>
#include <vector>

using namespace std;

/**
 * Scheduler — drives the entire job lifecycle in Phase 2.
 *
 * Two-level admission control:
 *   Layer 1: userQueue / fileQueue  (arrival)
 *   Layer 2: pendingQueue           (bounded, priority-sorted)
 *
 * Continuous tick loop updates running jobs, ages waiting jobs,
 * restores buffered jobs, and schedules pending jobs.
 */
class Scheduler {
public:
  explicit Scheduler(JobManager &jobs);

  // ── Tick ──────────────────────────────────────────────────────────────────
  // Advance the simulation by one tick.  Appends log messages to `log`.
  void tick(string &log);

  // ── Job submission (from CLI / file) ─────────────────────────────────────
  void submitFromCLI(const string &id, int mem, int ticks, Priority pri,
                     string &outMsg);
  void loadFile(const string &filename, string &outMsg);

  // ── Runtime expansion ────────────────────────────────────────────────────
  void expandJob(const string &jobId, int extraMem, string &outMsg);

  // ── Pause / resume ──────────────────────────────────────────────────────
  bool isPaused() const { return paused_; }
  void pause() { paused_ = true; }
  void resume() { paused_ = false; }

  // ── Accessors for visualization ──────────────────────────────────────────
  int currentTick() const { return currentTick_; }
  const vector<Job> &pendingQueue() const { return pendingQueue_; }
  const vector<Job> &buffer() const { return buffer_; }
  int userQueueSize() const { return (int)userQueue_.size(); }
  int fileQueueSize() const { return (int)fileQueue_.size(); }
  int fileJobsRemaining() const {
    return (int)fileJobs_.size() - fileIndex_;
  }

  static const int QUEUE_LIMIT = 10;
  static const int BUFFER_CAP = 5;

private:
  JobManager &jobs_;

  // Layer 1: admission queues
  queue<Job> userQueue_;
  queue<Job> fileQueue_;

  // Layer 2: scheduler queue (sorted by priorityScore descending)
  vector<Job> pendingQueue_;

  // CPU buffer for evicted jobs
  vector<Job> buffer_;

  // File batch loading
  vector<Job> fileJobs_;
  int fileIndex_ = 0;

  int currentTick_ = 0;
  bool paused_ = false;

  // Internal helpers
  void admitJobs(string &log);
  void updateRunningJobs(string &log);
  void restoreBufferedJob(string &log);
  void ageWaitingJobs();
  void scheduleJobs(string &log);
  void drainFileJob(string &log);
};

#endif // SCHEDULER_H
