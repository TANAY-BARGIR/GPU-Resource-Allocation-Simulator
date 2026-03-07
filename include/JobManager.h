#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#include "GPUDevice.h"
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

/**
 * Represents a single GPU job.
 */
struct Job {
  string id;
  int memoryUnits;
  int startBlock; // first allocated unit
  char label;     // single-char label for visualization (A–Z)
  int duration;   // seconds; 0 = manual-release only
  chrono::steady_clock::time_point startTime;
};

/**
 * JobManager — handles job submission, release, and bookkeeping.
 * Phase 1: immediate allocation only (no queue / priority).
 */
class JobManager {
public:
  explicit JobManager(GPUDevice &gpu);

  // Submit a job.  duration <= 0 → system estimates runtime.
  bool submitJob(const string &jobId, int memUnits, int duration,
                 string &outMsg);

  // Release a running job.  Returns true if found and released.
  bool releaseJob(const string &jobId, string &outMsg);

  // Auto-release jobs whose duration has elapsed.
  // Returns number of jobs released; messages appended to outMsg.
  int tickExpiredJobs(string &outMsg);

  // Seconds remaining for a job (< 0 if already expired, -1 if manual).
  int remainingSeconds(const string &jobId) const;

  // Estimate runtime for a given memory request (seconds).
  static int estimateDuration(int memUnits);

  // Query helpers
  bool jobExists(const string &jobId) const;
  const Job *getJob(const string &jobId) const;

  const unordered_map<string, Job> &activeJobs() const;

  // Resolve which job owns a given memory unit (for visualization).
  const Job *jobAtUnit(int unit) const;

  // Assign a unique label character to a job (A–Z, then wraps).
  char nextLabel();

  // Stats
  int totalSubmitted() const { return submitted_; }
  int totalCompleted() const { return completed_; }
  int totalFailed() const { return failed_; }

private:
  GPUDevice &gpu_;
  unordered_map<string, Job> jobs_;
  int submitted_ = 0;
  int completed_ = 0;
  int failed_ = 0;
  int labelCounter_ = 0;
};

#endif // JOB_MANAGER_H
