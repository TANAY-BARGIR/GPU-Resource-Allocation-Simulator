#include "Visualizer.h"
#include "Scheduler.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

Visualizer::Visualizer(const GPUDevice &gpu, const JobManager &jobs,
                       const Scheduler &sched)
    : gpu_(gpu), jobs_(jobs), sched_(sched) {}

void Visualizer::line(int width, char fill, char edge) {
  cout << edge << string(width - 2, fill) << edge << endl;
}

void Visualizer::doubleLine(int width) {
  cout << '+' << string(width - 2, '=') << '+' << endl;
}

static string padRight(const string &s, int width) {
  if ((int)s.size() >= width)
    return s.substr(0, width);
  return s + string(width - s.size(), ' ');
}

static string repeat(const string &s, int n) {
  string result;
  result.reserve(s.size() * n);
  for (int i = 0; i < n; ++i)
    result += s;
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// NVIDIA-SMI-style GPU info
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printGPUInfo() const {
  const auto &cfg = gpu_.config();
  int W = 79;

  cout << endl;
  line(W);
  cout << "| " << padRight("GPU Resource Manager Simulator", W - 25)
       << padRight("v2.0", 21) << " |" << endl;
  cout << "| " << padRight("Driver Version: " + cfg.driverVer, W / 2 - 2)
       << "  " << padRight("CUDA Version: " + cfg.cudaVer, W / 2 - 3) << " |"
       << endl;
  line(W, '-', '+');

  // GPU Row Header
  cout << "| " << padRight("GPU", 4) << padRight("Name", 16)
       << padRight("Bus-Id", 18) << "| " << padRight("Memory-Usage", 19)
       << "| " << padRight("GPU-Util", 14) << " |" << endl;
  doubleLine(W);

  // GPU Row Data
  ostringstream memStr;
  memStr << gpu_.usedMemory() << " / " << cfg.totalVRAM << " Units";
  ostringstream utilStr;
  utilStr << fixed << setprecision(0) << (gpu_.utilization() * 100) << "%";

  cout << "| " << padRight("0", 4) << padRight(cfg.name, 16)
       << padRight(cfg.busId, 18) << "| " << padRight(memStr.str(), 19) << "| "
       << padRight(utilStr.str(), 14) << " |" << endl;
  line(W, '-', '+');

  // Detailed specs
  cout << "| " << padRight("Arch: " + cfg.architecture, 38)
       << padRight("Cores: " + to_string(cfg.computeCores), 18)
       << padRight("TDP: " + to_string(cfg.tdpWatts) + "W", 19) << " |"
       << endl;
  cout << "| " << padRight("SMs: " + to_string(cfg.smCount), 19)
       << padRight("Tensor Cores: " + to_string(cfg.tensorCores), 19)
       << padRight("Mem Type: " + cfg.memoryType, W - 2 - 2 - 19 - 19) << " |"
       << endl;
  line(W, '-', '+');

  // Running processes table
  cout << "| " << padRight("Running Jobs", W - 4) << " |" << endl;
  doubleLine(W);

  const auto &activeJobs = jobs_.activeJobs();
  if (activeJobs.empty()) {
    cout << "| " << padRight("  No running jobs found.", W - 4) << " |"
         << endl;
  } else {
    cout << "| " << padRight("  JobID", 12) << padRight("Label", 6)
         << padRight("Blocks", 16) << padRight("Memory", 8)
         << padRight("Priority", 10) << padRight("Remain", 8)
         << padRight("Status", W - 2 - 2 - 12 - 6 - 16 - 8 - 10 - 8) << " |"
         << endl;
    line(W, '-', '|');

    vector<const Job *> sorted;
    for (auto &[id, job] : activeJobs)
      sorted.push_back(&job);
    sort(sorted.begin(), sorted.end(), [](const Job *a, const Job *b) {
      return a->startBlock < b->startBlock;
    });

    for (auto *job : sorted) {
      ostringstream range;
      range << job->startBlock << "–" << (job->startBlock + job->memoryUnits - 1);

      string remStr = to_string(job->remainingTicks) + "t";

      cout << "| " << padRight("  " + job->id, 12)
           << padRight(string(1, job->label), 6)
           << padRight(range.str(), 16)
           << padRight(to_string(job->memoryUnits), 8)
           << padRight(priorityToString(job->basePriority), 10)
           << padRight(remStr, 8)
           << padRight("RUNNING", W - 2 - 2 - 12 - 6 - 16 - 8 - 10 - 8)
           << " |" << endl;
    }
  }
  line(W);
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// ASCII Memory Map
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printMemoryMap() const {
  const int BAR_WIDTH = 64;
  const int INNER_W = BAR_WIDTH + 4;
  int total = gpu_.config().totalVRAM;
  double scale = (double)BAR_WIDTH / total;

  cout << endl;
  {
    ostringstream hdr;
    hdr << "═ GPU Memory Map (" << total << " units) ";
    string h = hdr.str();
    cout << "  ╔" << h << repeat("═", INNER_W - (int)h.size() + 2) << "╗"
         << endl;
  }

  // Numbered ruler
  int tickInterval = total / 4;
  if (tickInterval <= 0)
    tickInterval = total;

  string numLine(BAR_WIDTH, ' ');
  string tickLine(BAR_WIDTH, '-');

  for (int val = 0; val <= total; val += tickInterval) {
    int pos = (int)(val * scale);
    if (pos >= BAR_WIDTH)
      pos = BAR_WIDTH - 1;

    string label = to_string(val);

    if (val == total) {
      int startPos = BAR_WIDTH - (int)label.size();
      if (startPos < 0)
        startPos = 0;
      for (int j = 0; j < (int)label.size() && (startPos + j) < BAR_WIDTH; ++j)
        numLine[startPos + j] = label[j];
      tickLine[BAR_WIDTH - 1] = '|';
    } else {
      for (int j = 0; j < (int)label.size() && (pos + j) < BAR_WIDTH; ++j)
        numLine[pos + j] = label[j];
      tickLine[pos] = '|';
    }
  }

  cout << "  ║ " << padRight(numLine, INNER_W - 1) << "║" << endl;
  cout << "  ║ " << padRight(tickLine, INNER_W - 1) << "║" << endl;

  // Memory bar
  string bar(BAR_WIDTH, '.');
  vector<const Job *> sorted;
  for (auto &[id, job] : jobs_.activeJobs())
    sorted.push_back(&job);
  sort(sorted.begin(), sorted.end(), [](const Job *a, const Job *b) {
    return a->startBlock < b->startBlock;
  });

  for (auto *job : sorted) {
    int barStart = (int)(job->startBlock * scale);
    int barEnd = (int)((job->startBlock + job->memoryUnits) * scale);
    barEnd = min(barEnd, BAR_WIDTH);
    for (int i = barStart; i < barEnd; ++i) {
      bar[i] = job->label;
    }
  }
  cout << "  ║ " << padRight(bar, INNER_W - 1) << "║" << endl;

  // Legend
  if (sorted.empty()) {
    cout << "  ║ " << padRight("(all memory free)", INNER_W - 1) << "║"
         << endl;
  } else {
    ostringstream legend;
    for (auto *job : sorted) {
      legend << job->label << "=" << job->id << "  ";
    }
    legend << ".=free";
    cout << "  ║ " << padRight(legend.str(), INNER_W - 1) << "║" << endl;
  }
  cout << "  ╚" << repeat("═", INNER_W) << "╝" << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Block Range Layout Table
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printBlockLayout() const {
  int total = gpu_.config().totalVRAM;

  struct Segment {
    int start, length;
    string owner;
    char label;
  };

  vector<Segment> segments;

  vector<const Job *> sorted;
  for (auto &[id, job] : jobs_.activeJobs())
    sorted.push_back(&job);
  sort(sorted.begin(), sorted.end(), [](const Job *a, const Job *b) {
    return a->startBlock < b->startBlock;
  });

  int cursor = 0;
  for (auto *job : sorted) {
    if (job->startBlock > cursor) {
      segments.push_back({cursor, job->startBlock - cursor, "FREE", '.'});
    }
    segments.push_back(
        {job->startBlock, job->memoryUnits, job->id, job->label});
    cursor = job->startBlock + job->memoryUnits;
  }
  if (cursor < total) {
    segments.push_back({cursor, total - cursor, "FREE", '.'});
  }

  cout << endl;
  cout << "  Block Layout" << endl;
  cout << "  " << repeat("─", 55) << endl;

  for (auto &seg : segments) {
    ostringstream range;
    range << setw(5) << right << seg.start << " – " << setw(5) << left
          << (seg.start + seg.length - 1);
    string tag = (seg.owner == "FREE")
                     ? "\033[32m FREE \033[0m"
                     : "\033[33m " + seg.owner + " \033[0m";

    cout << "  " << range.str() << "  :  " << tag << "  (" << seg.length
         << " units)" << endl;
  }
  cout << "  " << repeat("─", 55) << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fragmentation Score
// ─────────────────────────────────────────────────────────────────────────────

int Visualizer::fragmentationScore() const {
  int freeMem = gpu_.freeMemory();
  if (freeMem == 0)
    return 0;
  int largest = gpu_.memory().largestFreeBlock();
  double ratio = 1.0 - (double)largest / freeMem;
  return (int)(ratio * 100);
}

// ─────────────────────────────────────────────────────────────────────────────
// Status — combined view (Phase 2: includes tick, pending, buffer info)
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printStatus() const {
  const int BOX_W = 60;
  int total = gpu_.config().totalVRAM;
  int used = gpu_.usedMemory();
  int free = gpu_.freeMemory();
  int largest = gpu_.memory().largestFreeBlock();
  int fragScore = fragmentationScore();
  int activeCount = (int)jobs_.activeJobs().size();
  int pendingCount = (int)sched_.pendingQueue().size();
  int bufferCount = (int)sched_.buffer().size();
  int tick = sched_.currentTick();

  auto row = [&](const string &content) {
    cout << "  │" << padRight(content, BOX_W) << "│" << endl;
  };

  cout << endl;
  cout << "  ┌──────────────────────── System Status "
       << "─────────────────────┐" << endl;

  {
    ostringstream o;
    o << "  Tick              : " << setw(6) << tick;
    if (sched_.isPaused())
    o << "  ⏸ PAUSED          ";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Total Memory      : " << setw(6) << total << " units";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Used  Memory      : " << setw(6) << used << " units  (" << fixed
      << setprecision(1) << (gpu_.utilization() * 100) << "%)";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Free  Memory      : " << setw(6) << free << " units";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Largest Free Block: " << setw(5) << largest << " units";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Fragmentation     :   " << setw(3) << fragScore << "%";
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Running Jobs      :     " << setw(3) << activeCount;
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Pending Jobs      :     " << setw(3) << pendingCount;
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Buffered (CPU)    :     " << setw(3) << bufferCount << " / "
      << Scheduler::BUFFER_CAP;
    row(o.str());
  }
  {
    ostringstream o;
    o << "  Admission Queues  : user=" << sched_.userQueueSize()
      << "  file=" << sched_.fileQueueSize()
      << "  upcoming=" << sched_.fileJobsRemaining();
    row(o.str());
  }

  cout << "  ├──────────────────────── Job Statistics "
       << "────────────────────┤" << endl;
  {
    ostringstream o;
    o << "  Submitted: " << setw(4) << jobs_.totalSubmitted()
      << "   Completed: " << setw(4) << jobs_.totalCompleted()
      << "   Failed: " << setw(4) << jobs_.totalFailed();
    row(o.str());
  }
  cout << "  └────────────────────────────────────────────────────────────┘"
       << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Quick bar — compact one-liner after submit / release
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printQuickBar() const {
  const int BAR_W = 40;
  int total = gpu_.config().totalVRAM;
  int used = gpu_.usedMemory();
  int filled = (total > 0) ? (int)((double)used / total * BAR_W) : 0;

  cout << "  Memory: [";
  cout << "\033[33m" << repeat("█", filled) << "\033[0m";
  cout << "\033[32m" << repeat("░", BAR_W - filled) << "\033[0m";
  cout << "] " << used << "/" << total << " units (" << fixed
       << setprecision(1) << (gpu_.utilization() * 100) << "%)" << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Queue display — pending + admission queue info
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printQueue() const {
  const auto &pending = sched_.pendingQueue();

  cout << endl;
  cout << "  ┌───────────────────── Pending Queue (" << pending.size()
       << "/" << Scheduler::QUEUE_LIMIT << ") ───────────────────┐"
       << endl;

  if (pending.empty()) {
    cout << "  │  (empty)                                                     │"
         << endl;
  } else {
    cout << "  │  #  JobID          Mem    Ticks  Priority  Score  Wait       │"
         << endl;
    cout << "  │   ─────────────────────────────────────────────────────────  │"
         << endl;
    int idx = 1;
    for (auto &job : pending) {
      ostringstream o;
      o << "  " << setw(2) << idx++ << " " << padRight(job.id, 14)
        << setw(5) << job.memoryUnits << "  " << setw(5) << job.remainingTicks
        << "   " << padRight(priorityToString(job.basePriority), 8)
        << setw(5) << job.priorityScore << "  " << setw(4) << job.waitTicks
        << "t";
      cout << "  │" << padRight(o.str(), 62) << "│" << endl;
    }
  }

  cout << "  ├───────────────────── Admission Queues ───────────────────────┤"
       << endl;
  {
    ostringstream o;
    o << "  User queue: " << sched_.userQueueSize()
      << "   File queue: " << sched_.fileQueueSize()
      << "   File jobs remaining: " << sched_.fileJobsRemaining();
    cout << "  │" << padRight(o.str(), 62) << "│" << endl;
  }
  cout << "  └──────────────────────────────────────────────────────────────┘"
       << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Buffer display — evicted jobs in CPU memory
// ─────────────────────────────────────────────────────────────────────────────

void Visualizer::printBuffer() const {
  const auto &buf = sched_.buffer();

  cout << endl;
  cout << "  ┌───────────────────── CPU Buffer (" << buf.size() << "/"
       << Scheduler::BUFFER_CAP << ") ─────────────────────┐"
       << endl;

  if (buf.empty()) {
    cout << "  │  (empty — no evicted jobs)                                 │"
         << endl;
  } else {
    cout << "  │  #  JobID          Mem    RemTicks  Priority  Source       │"
         << endl;
    cout << "  │  ───────────────────────────────────────────────────────── │"
         << endl;
    int idx = 1;
    for (auto &job : buf) {
      ostringstream o;
      o << "  " << setw(2) << idx++ << " " << padRight(job.id, 14)
        << setw(5) << job.memoryUnits << "  " << setw(8) << job.remainingTicks
        << "  " << padRight(priorityToString(job.basePriority), 8) << "  "
        << (job.fromFile ? "FILE" : "CLI ");
      cout << "  │" << padRight(o.str(), 60) << "│" << endl;
    }
  }

  cout << "  └────────────────────────────────────────────────────────────┘"
       << endl;
  cout << endl;
}
