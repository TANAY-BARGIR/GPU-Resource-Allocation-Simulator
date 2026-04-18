#include "JobManager.h"
#include <algorithm>
#include <sstream>

using namespace std;

JobManager::JobManager(GPUDevice &gpu) : gpu_(gpu) {}

char JobManager::nextLabel() {
  char c = 'A' + (labelCounter_ % 26);
  ++labelCounter_;
  return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Allocate GPU memory for a job.  On success, sets job.startBlock.
// ─────────────────────────────────────────────────────────────────────────────

bool JobManager::allocateJob(Job &job, string &outMsg) {
  ostringstream oss;

  if (job.memoryUnits <= 0) {
    oss << "  ✗ Error: Memory request must be a positive integer.";
    outMsg = oss.str();
    return false;
  }
  if (job.memoryUnits > gpu_.config().totalVRAM) {
    oss << "  ✗ Error: Requested " << job.memoryUnits
        << " units exceeds total GPU memory (" << gpu_.config().totalVRAM
        << " units).";
    outMsg = oss.str();
    return false;
  }

  int pos = gpu_.memory().allocate(job.memoryUnits);
  if (pos == -1) {
    int largest = gpu_.memory().largestFreeBlock();
    int freeMem = gpu_.freeMemory();
    oss << "  ✗ Allocation failed for '" << job.id << "' (" << job.memoryUnits
        << " units).\n"
        << "    Available free memory  : " << freeMem << " units\n"
        << "    Largest contiguous block: " << largest << " units\n";
    if (freeMem >= job.memoryUnits) {
      oss << "    ⚠ Sufficient total memory exists but is fragmented.";
    } else {
      oss << "    → Need " << (job.memoryUnits - freeMem) << " more units.";
    }
    outMsg = oss.str();
    return false;
  }

  job.startBlock = pos;
  job.isRunning = true;

  oss << "  ✓ Job '" << job.id << "' allocated  [" << pos << "–"
      << (pos + job.memoryUnits - 1) << "]  " << job.memoryUnits << " units  "
      << priorityToString(job.basePriority) << "  " << job.totalTicks
      << " ticks";
  outMsg = oss.str();
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Release a running job — free its GPU memory.
// ─────────────────────────────────────────────────────────────────────────────

bool JobManager::releaseJob(const string &jobId, string &outMsg) {
  ostringstream oss;
  auto it = jobs_.find(jobId);
  if (it == jobs_.end()) {
    oss << "  ✗ Error: Job '" << jobId << "' not found.";
    outMsg = oss.str();
    return false;
  }

  const Job &job = it->second;
  gpu_.memory().deallocate(job.startBlock, job.memoryUnits);
  ++completed_;

  oss << "  ✓ Job '" << jobId << "' completed.  Freed [" << job.startBlock
      << "–" << (job.startBlock + job.memoryUnits - 1) << "] ("
      << job.memoryUnits << " units)";
  outMsg = oss.str();

  jobs_.erase(it);
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Register / unregister (bookkeeping for active-jobs map)
// ─────────────────────────────────────────────────────────────────────────────

void JobManager::registerJob(const Job &job) { jobs_[job.id] = job; }

void JobManager::unregisterJob(const string &jobId) { jobs_.erase(jobId); }

bool JobManager::jobExists(const string &jobId) const {
  return jobs_.count(jobId) > 0;
}

const Job *JobManager::getJob(const string &jobId) const {
  auto it = jobs_.find(jobId);
  return (it != jobs_.end()) ? &it->second : nullptr;
}

Job *JobManager::getJobMut(const string &jobId) {
  auto it = jobs_.find(jobId);
  return (it != jobs_.end()) ? &it->second : nullptr;
}

const unordered_map<string, Job> &JobManager::activeJobs() const {
  return jobs_;
}

const Job *JobManager::jobAtUnit(int unit) const {
  for (auto &[id, job] : jobs_) {
    if (unit >= job.startBlock && unit < job.startBlock + job.memoryUnits)
      return &job;
  }
  return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Find adjacent eviction candidate — left or right neighbor with lowest
// priority.  Only directly adjacent jobs (sharing a boundary) are considered.
// Returns nullptr if no suitable candidate.
// ─────────────────────────────────────────────────────────────────────────────

Job *JobManager::findAdjacentEvictCandidate(const Job &requester) {
  int reqStart = requester.startBlock;
  int reqEnd = requester.startBlock + requester.memoryUnits;

  Job *bestCandidate = nullptr;

  for (auto &[id, job] : jobs_) {
    if (job.id == requester.id)
      continue;

    int jobStart = job.startBlock;
    int jobEnd = job.startBlock + job.memoryUnits;

    // Check if directly adjacent (left or right neighbor)
    bool adjacent = (jobEnd == reqStart) || (jobStart == reqEnd);
    if (!adjacent)
      continue;

    // Pick the one with lowest priority; tie-break by fewer remaining ticks
    if (!bestCandidate ||
        job.basePriority < bestCandidate->basePriority ||
        (job.basePriority == bestCandidate->basePriority &&
         job.remainingTicks < bestCandidate->remainingTicks)) {
      bestCandidate = &job;
    }
  }

  return bestCandidate;
}
