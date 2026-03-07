#include "JobManager.h"
#include <sstream>

using namespace std;

JobManager::JobManager(GPUDevice &gpu) : gpu_(gpu) {}

char JobManager::nextLabel() {
  char c = 'A' + (labelCounter_ % 26);
  ++labelCounter_;
  return c;
}

int JobManager::estimateDuration(int memUnits) {
  // Rule: base 10 seconds + 1 second per 20 memory units
  return 10 + memUnits / 20;
}

bool JobManager::submitJob(const string &jobId, int memUnits, int duration,
                           string &outMsg) {
  ++submitted_;
  ostringstream oss;

  // Validation
  if (jobId.empty()) {
    ++failed_;
    outMsg = "  ✗ Error: Job ID cannot be empty.";
    return false;
  }
  if (jobs_.count(jobId)) {
    ++failed_;
    outMsg = "  ✗ Error: Job '" + jobId + "' already exists.";
    return false;
  }
  if (memUnits <= 0) {
    ++failed_;
    outMsg = "  ✗ Error: Memory request must be a positive integer.";
    return false;
  }
  if (memUnits > gpu_.config().totalVRAM) {
    ++failed_;
    oss << "  ✗ Error: Requested " << memUnits
        << " units exceeds total GPU memory (" << gpu_.config().totalVRAM
        << " units).";
    outMsg = oss.str();
    return false;
  }

  // Attempt allocation
  int pos = gpu_.memory().allocate(memUnits);
  if (pos == -1) {
    ++failed_;
    int largest = gpu_.memory().largestFreeBlock();
    int freeMem = gpu_.freeMemory();
    oss << "  ✗ Allocation failed for '" << jobId << "' (" << memUnits
        << " units).\n"
        << "    Available free memory  : " << freeMem << " units\n"
        << "    Largest contiguous block: " << largest << " units\n";
    if (freeMem >= memUnits) {
      oss << "    ⚠ Sufficient total memory exists but is fragmented.\n"
          << "    → Release a running job to create a larger contiguous block.";
    } else {
      oss << "    → Free up at least " << (memUnits - freeMem)
          << " more units.";
    }
    outMsg = oss.str();
    return false;
  }

  // Determine runtime
  bool systemEstimated = false;
  if (duration <= 0) {
    duration = estimateDuration(memUnits);
    systemEstimated = true;
  }

  // Success
  Job job;
  job.id = jobId;
  job.memoryUnits = memUnits;
  job.startBlock = pos;
  job.label = nextLabel();
  job.duration = duration;
  job.startTime = chrono::steady_clock::now();
  jobs_[jobId] = job;

  oss << "  ✓ Job '" << jobId << "' started successfully.\n"
      << "    Allocated blocks : " << pos << " – " << (pos + memUnits - 1)
      << "\n"
      << "    Resource usage   : " << memUnits << " / "
      << gpu_.config().totalVRAM << " units\n"
      << "    Duration         : " << duration << "s";
  if (systemEstimated) {
    oss << " (system estimated)";
  }
  outMsg = oss.str();
  return true;
}

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

  oss << "  ✓ Job '" << jobId << "' released.\n"
      << "    Freed blocks: " << job.startBlock << " – "
      << (job.startBlock + job.memoryUnits - 1) << " (" << job.memoryUnits
      << " units)";
  outMsg = oss.str();

  jobs_.erase(it);
  return true;
}

int JobManager::tickExpiredJobs(string &outMsg) {
  auto now = chrono::steady_clock::now();
  vector<string> expired;

  for (auto &[id, job] : jobs_) {
    if (job.duration > 0) {
      auto elapsed =
          chrono::duration_cast<chrono::seconds>(now - job.startTime)
              .count();
      if (elapsed >= job.duration) {
        expired.push_back(id);
      }
    }
  }

  ostringstream oss;
  for (auto &id : expired) {
    auto it = jobs_.find(id);
    if (it == jobs_.end())
      continue;
    const Job &job = it->second;
    gpu_.memory().deallocate(job.startBlock, job.memoryUnits);
    ++completed_;
    oss << "\n  ✓ Job '" << id << "' finished automatically. ("
        << job.memoryUnits << " units freed)";
    jobs_.erase(it);
  }
  outMsg = oss.str();
  return (int)expired.size();
}

int JobManager::remainingSeconds(const string &jobId) const {
  auto it = jobs_.find(jobId);
  if (it == jobs_.end())
    return -1;
  const Job &job = it->second;
  if (job.duration <= 0)
    return -1; // manual job
  auto now = chrono::steady_clock::now();
  auto elapsed =
      chrono::duration_cast<chrono::seconds>(now - job.startTime)
          .count();
  int remaining = job.duration - (int)elapsed;
  return (remaining > 0) ? remaining : 0;
}

bool JobManager::jobExists(const string &jobId) const {
  return jobs_.count(jobId) > 0;
}

const Job *JobManager::getJob(const string &jobId) const {
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
