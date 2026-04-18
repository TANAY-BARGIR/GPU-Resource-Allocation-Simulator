#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include "GPUDevice.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Priority levels
// ─────────────────────────────────────────────────────────────────────────────

enum Priority { PRI_LOW = 0, PRI_MID = 1, PRI_HIGH = 2, PRI_IMP = 3 };

inline int priorityBaseScore(Priority p) {
  switch (p) {
  case PRI_IMP:
    return 90;
  case PRI_HIGH:
    return 60;
  case PRI_MID:
    return 40;
  case PRI_LOW:
  default:
    return 20;
  }
}

inline string priorityToString(Priority p) {
  switch (p) {
  case PRI_IMP:
    return "IMP";
  case PRI_HIGH:
    return "HIGH";
  case PRI_MID:
    return "MID";
  case PRI_LOW:
  default:
    return "LOW";
  }
}

inline Priority stringToPriority(const string &s) {
  if (s == "IMP" || s == "imp")
    return PRI_IMP;
  if (s == "HIGH" || s == "high")
    return PRI_HIGH;
  if (s == "MID" || s == "mid")
    return PRI_MID;
  return PRI_LOW;
}

// ─────────────────────────────────────────────────────────────────────────────
// Job structure  (Phase 2 — tick-based, priority-aware)
// ─────────────────────────────────────────────────────────────────────────────

struct Job {
  string id;
  int memoryUnits = 0;
  int startBlock = -1;  // -1 if not allocated
  char label = '?';

  int totalTicks = 0;     // total ticks to execute
  int remainingTicks = 0; // decremented each tick while running

  Priority basePriority = PRI_MID;
  int priorityScore = 40; // dynamic score (aging adjusts this)

  int arrivalTick = 0;
  int waitTicks = 0; // ticks spent waiting in pending queue

  bool isRunning = false;
  bool fromFile = false; // true = loaded from jobs.txt
};

// ─────────────────────────────────────────────────────────────────────────────
// JobManager — handles GPU allocation/deallocation and active job tracking.
// Phase 2: no longer does scheduling; Scheduler drives the lifecycle.
// ─────────────────────────────────────────────────────────────────────────────

class JobManager {
public:
  explicit JobManager(GPUDevice &gpu);

  // Attempt to allocate a job on GPU memory.  Updates job.startBlock.
  // Returns true if allocation succeeded.
  bool allocateJob(Job &job, string &outMsg);

  // Release a running job (free GPU memory).  Returns true if found.
  bool releaseJob(const string &jobId, string &outMsg);

  // Register an already-allocated job (used by scheduler after allocateJob).
  void registerJob(const Job &job);

  // Remove a job from active tracking (after release).
  void unregisterJob(const string &jobId);

  // Check if a job ID is currently active (running on GPU).
  bool jobExists(const string &jobId) const;
  const Job *getJob(const string &jobId) const;
  Job *getJobMut(const string &jobId);

  const unordered_map<string, Job> &activeJobs() const;

  // Find job that owns a given memory unit (for visualization).
  const Job *jobAtUnit(int unit) const;

  // Find an adjacent job (left or right neighbor) for eviction.
  // Returns nullptr if no neighbor or all neighbors are >= requesterPri.
  Job *findAdjacentEvictCandidate(const Job &requester);

  // Assign a unique label character (A–Z wrapping).
  char nextLabel();

  // Stats
  int totalSubmitted() const { return submitted_; }
  int totalCompleted() const { return completed_; }
  int totalFailed() const { return failed_; }
  void incSubmitted() { ++submitted_; }
  void incCompleted() { ++completed_; }
  void incFailed() { ++failed_; }

  GPUDevice &gpu() { return gpu_; }

private:
  GPUDevice &gpu_;
  unordered_map<string, Job> jobs_;
  int submitted_ = 0;
  int completed_ = 0;
  int failed_ = 0;
  int labelCounter_ = 0;
};

#endif // JOB_MANAGER_H
