#include "Scheduler.h"
#include <algorithm>
#include <fstream>
#include <sstream>

using namespace std;

Scheduler::Scheduler(JobManager &jobs) : jobs_(jobs) {}

// ─────────────────────────────────────────────────────────────────────────────
// tick — advance the simulation by one step
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::tick(string &log) {
  log.clear();
  ++currentTick_;

  // 1. Drip-feed one file job into fileQueue
  drainFileJob(log);

  // 2. Admission control: move jobs from admission queues → pendingQueue
  admitJobs(log);

  // 3. Update running jobs: decrement remaining, release completed
  updateRunningJobs(log);

  // 4. Restore one buffered job if possible
  restoreBufferedJob(log);

  // 5. Age waiting jobs and re-sort
  ageWaitingJobs();

  // 6. Schedule highest-priority pending jobs
  scheduleJobs(log);
}

// ─────────────────────────────────────────────────────────────────────────────
// drainFileJob — push one job from fileJobs → fileQueue per tick
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::drainFileJob(string &log) {
  (void)log;
  if (fileIndex_ >= (int)fileJobs_.size())
    return;

  // Safety valve: pause file intake when buffer is full
  if ((int)buffer_.size() >= BUFFER_CAP) {
    return; // file jobs stay in fileJobs_, not pushed
  }

  Job job = fileJobs_[fileIndex_];
  job.arrivalTick = currentTick_;
  fileQueue_.push(job);
  ++fileIndex_;
}

// ─────────────────────────────────────────────────────────────────────────────
// admitJobs — move jobs from admission queues → bounded pendingQueue
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::admitJobs(string &log) {
  (void)log;
  // User jobs first
  while (!userQueue_.empty() && (int)pendingQueue_.size() < QUEUE_LIMIT) {
    pendingQueue_.push_back(userQueue_.front());
    userQueue_.pop();
  }
  // Then file jobs
  while (!fileQueue_.empty() && (int)pendingQueue_.size() < QUEUE_LIMIT) {
    pendingQueue_.push_back(fileQueue_.front());
    fileQueue_.pop();
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// updateRunningJobs — decrement remainingTicks, release completed
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::updateRunningJobs(string &log) {
  vector<string> completed;

  // Collect all active job IDs first (can't modify map while iterating)
  vector<string> allIds;
  for (auto &[id, job] : jobs_.activeJobs()) {
    allIds.push_back(id);
  }

  // Decrement remaining ticks, find completed
  for (auto &id : allIds) {
    Job *job = jobs_.getJobMut(id);
    if (!job || !job->isRunning)
      continue;

    job->remainingTicks--;
    if (job->remainingTicks <= 0) {
      completed.push_back(id);
    }
  }

  // Release completed jobs
  for (auto &id : completed) {
    string msg;
    jobs_.releaseJob(id, msg);
    log += "\n" + msg;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// restoreBufferedJob — FIFO, full-alloc-or-skip, one per tick
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::restoreBufferedJob(string &log) {
  if (buffer_.empty())
    return;

  // Try the first buffered job (FIFO)
  Job &candidate = buffer_.front();

  // Attempt full allocation
  string msg;
  if (jobs_.allocateJob(candidate, msg)) {
    candidate.isRunning = true;
    jobs_.registerJob(candidate);
    log += "\n  ↩ Restored '" + candidate.id + "' from buffer → GPU.  " + msg;
    buffer_.erase(buffer_.begin());
  }
  // If allocation fails, skip — do NOT try next job (one attempt per tick)
}

// ─────────────────────────────────────────────────────────────────────────────
// ageWaitingJobs — increment waitTicks, recalculate priorityScore
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::ageWaitingJobs() {
  for (auto &job : pendingQueue_) {
    job.waitTicks++;
    int score = priorityBaseScore(job.basePriority) + (job.waitTicks * 3);
    // CLI bonus
    if (!job.fromFile)
      score += 10;
    // Clamp
    job.priorityScore = min(max(score, 0), 100);
  }

  // Sort descending by priorityScore (highest first)
  sort(pendingQueue_.begin(), pendingQueue_.end(),
       [](const Job &a, const Job &b) {
         if (a.priorityScore != b.priorityScore)
           return a.priorityScore > b.priorityScore;
         return a.arrivalTick < b.arrivalTick; // tie-break: earlier arrival
       });
}

// ─────────────────────────────────────────────────────────────────────────────
// scheduleJobs — try to allocate top pending jobs
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::scheduleJobs(string &log) {
  vector<int> allocated; // indices to remove

  for (int i = 0; i < (int)pendingQueue_.size(); ++i) {
    Job &job = pendingQueue_[i];
    string msg;
    if (jobs_.allocateJob(job, msg)) {
      job.isRunning = true;
      jobs_.registerJob(job);
      log += "\n" + msg;
      allocated.push_back(i);
    }
    // If allocation fails, keep in queue (don't break — smaller jobs later
    // might still fit)
  }

  // Remove allocated jobs from pending (reverse order to keep indices valid)
  for (int i = (int)allocated.size() - 1; i >= 0; --i) {
    pendingQueue_.erase(pendingQueue_.begin() + allocated[i]);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// submitFromCLI — create a job from CLI and push to userQueue
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::submitFromCLI(const string &id, int mem, int ticks,
                              Priority pri, string &outMsg) {
  ostringstream oss;

  // Validate
  if (id.empty()) {
    outMsg = "  ✗ Error: Job ID cannot be empty.";
    jobs_.incFailed();
    return;
  }
  if (jobs_.jobExists(id)) {
    outMsg = "  ✗ Error: Job '" + id + "' already exists.";
    jobs_.incFailed();
    return;
  }
  // Check if it's already in pending or user queue
  for (auto &j : pendingQueue_) {
    if (j.id == id) {
      outMsg = "  ✗ Error: Job '" + id + "' already in pending queue.";
      jobs_.incFailed();
      return;
    }
  }
  if (mem <= 0) {
    outMsg = "  ✗ Error: Memory request must be a positive integer.";
    jobs_.incFailed();
    return;
  }
  if (ticks <= 0) {
    outMsg = "  ✗ Error: Tick count must be a positive integer.";
    jobs_.incFailed();
    return;
  }

  Job job;
  job.id = id;
  job.memoryUnits = mem;
  job.totalTicks = ticks;
  job.remainingTicks = ticks;
  job.basePriority = pri;
  job.priorityScore = priorityBaseScore(pri) + 10; // CLI bonus
  job.arrivalTick = currentTick_;
  job.waitTicks = 0;
  job.isRunning = false;
  job.fromFile = false;
  job.label = jobs_.nextLabel();

  jobs_.incSubmitted();
  userQueue_.push(job);

  oss << "  ⊕ Job '" << id << "' queued  [" << mem << " units, " << ticks
      << " ticks, " << priorityToString(pri) << ", score="
      << job.priorityScore << "]";
  outMsg = oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// loadFile — read a batch jobs file
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::loadFile(const string &filename, string &outMsg) {
  ifstream fin(filename);
  if (!fin.is_open()) {
    outMsg = "  ✗ Error: Cannot open file '" + filename + "'";
    return;
  }

  int count = 0;
  string line;
  while (getline(fin, line)) {
    if (line.empty())
      continue;

    istringstream iss(line);
    string id;
    int mem = 0, ticks = 0;
    string priStr = "MID";

    iss >> id >> mem >> ticks;
    if (id.empty() || mem <= 0 || ticks <= 0)
      continue;

    if (iss >> priStr) {
      // got priority
    }

    Job job;
    job.id = id;
    job.memoryUnits = mem;
    job.totalTicks = ticks;
    job.remainingTicks = ticks;
    job.basePriority = stringToPriority(priStr);
    job.priorityScore = priorityBaseScore(job.basePriority);
    job.arrivalTick = 0; // will be set when dripped into fileQueue
    job.waitTicks = 0;
    job.isRunning = false;
    job.fromFile = true;
    job.label = jobs_.nextLabel();

    jobs_.incSubmitted();
    fileJobs_.push_back(job);
    ++count;
  }

  ostringstream oss;
  oss << "  ✓ Loaded " << count << " jobs from '" << filename
      << "'.  They will arrive one per tick.";
  if (!fileJobs_.empty()) {
    oss << "\n    Jobs: ";
    for (int i = 0; i < min((int)fileJobs_.size(), 5); ++i) {
      if (i > 0)
        oss << ", ";
      oss << fileJobs_[i].id;
    }
    if ((int)fileJobs_.size() > 5)
      oss << " ...";
  }
  outMsg = oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// expandJob — runtime memory expansion with eviction support
// ─────────────────────────────────────────────────────────────────────────────

void Scheduler::expandJob(const string &jobId, int extraMem, string &outMsg) {
  ostringstream oss;

  Job *job = jobs_.getJobMut(jobId);
  if (!job) {
    outMsg = "  ✗ Error: Job '" + jobId + "' not found or not running.";
    return;
  }
  if (!job->isRunning) {
    outMsg = "  ✗ Error: Job '" + jobId + "' is not running (cannot expand).";
    return;
  }
  if (extraMem <= 0) {
    outMsg = "  ✗ Error: Extra memory must be positive.";
    return;
  }

  int reqEnd = job->startBlock + job->memoryUnits;
  int totalCap = jobs_.gpu().config().totalVRAM;

  // Case 1: Check if contiguous space exists right after the job
  bool canExpandRight = true;
  if (reqEnd + extraMem > totalCap) {
    canExpandRight = false;
  } else {
    for (int i = reqEnd; i < reqEnd + extraMem; ++i) {
      // Check via segment tree's used_ status
      const Job *occupant = jobs_.jobAtUnit(i);
      if (occupant != nullptr) {
        canExpandRight = false;
        break;
      }
    }
  }

  if (canExpandRight) {
    // Direct expansion — mark extra units as used
    jobs_.gpu().memory().allocate(extraMem); // This uses best-fit, not ideal
    // Actually we need to mark specific units. Let's use deallocate/reallocate.
    // Better approach: deallocate old, reallocate new size
    int oldStart = job->startBlock;
    int oldSize = job->memoryUnits;
    int newSize = oldSize + extraMem;

    jobs_.gpu().memory().deallocate(oldStart, oldSize);
    int newPos = jobs_.gpu().memory().allocate(newSize);

    if (newPos == oldStart) {
      // Great, expanded in place
      job->memoryUnits = newSize;
      oss << "  ✓ Job '" << jobId << "' expanded by " << extraMem
          << " units.  Now [" << oldStart << "–" << (oldStart + newSize - 1)
          << "] (" << newSize << " units)";
      outMsg = oss.str();
      return;
    } else {
      // Rollback — couldn't expand in place after dealloc/realloc
      jobs_.gpu().memory().deallocate(newPos, newSize);
      jobs_.gpu().memory().allocate(oldSize); // re-allocate old
      // Fall through to eviction
    }
  }

  // Case 2: Try eviction of one adjacent neighbor
  Job *victim = jobs_.findAdjacentEvictCandidate(*job);
  if (!victim) {
    oss << "  ✗ Expansion failed for '" << jobId << "': no adjacent job to "
        << "evict and no contiguous free space.";
    outMsg = oss.str();
    return;
  }

  // Check if buffer is full
  if ((int)buffer_.size() >= BUFFER_CAP) {
    oss << "  ✗ Expansion failed for '" << jobId
        << "': CPU buffer is full (cap=" << BUFFER_CAP << ").";
    outMsg = oss.str();
    return;
  }

  // Evict the victim to buffer
  string victimId = victim->id;
  int victimStart = victim->startBlock;
  int victimSize = victim->memoryUnits;

  // Save victim to buffer (preserve remaining ticks)
  Job bufferedVictim = *victim;
  bufferedVictim.isRunning = false;
  bufferedVictim.startBlock = -1;
  buffer_.push_back(bufferedVictim);

  // Free victim's GPU memory
  jobs_.gpu().memory().deallocate(victimStart, victimSize);
  jobs_.unregisterJob(victimId);

  oss << "  ⚠ Evicted '" << victimId << "' ("
      << priorityToString(bufferedVictim.basePriority)
      << ") to CPU buffer.\n";

  // Now try expanding
  int oldStart = job->startBlock;
  int oldSize = job->memoryUnits;
  int newSize = oldSize + extraMem;

  jobs_.gpu().memory().deallocate(oldStart, oldSize);
  int newPos = jobs_.gpu().memory().allocate(newSize);

  if (newPos != -1) {
    job->startBlock = newPos;
    job->memoryUnits = newSize;
    // Update in active jobs map
    jobs_.registerJob(*job);
    oss << "  ✓ Job '" << jobId << "' expanded to " << newSize << " units ["
        << newPos << "–" << (newPos + newSize - 1) << "]";
  } else {
    // Expansion still failed even after eviction — re-allocate original
    int restored = jobs_.gpu().memory().allocate(oldSize);
    job->startBlock = restored;
    jobs_.registerJob(*job);
    oss << "  ✗ Expansion still failed after eviction. Job '" << jobId
        << "' kept at original size.";
  }

  outMsg = oss.str();
}
