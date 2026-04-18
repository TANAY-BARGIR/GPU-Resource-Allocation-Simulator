#include "CLI.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

// Non-blocking input
#include <poll.h>
#include <unistd.h>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

CLI::CLI(GPUDevice &gpu, JobManager &jobs, Scheduler &sched)
    : gpu_(gpu), jobs_(jobs), sched_(sched), vis_(gpu, jobs, sched) {}

// ─────────────────────────────────────────────────────────────────────────────
// Non-blocking stdin check  (Linux: poll on fd 0)
// Returns true if there is data to read within timeoutMs.
// ─────────────────────────────────────────────────────────────────────────────

bool CLI::inputAvailable(int timeoutMs) {
  struct pollfd pfd;
  pfd.fd = STDIN_FILENO;
  pfd.events = POLLIN;
  int ret = poll(&pfd, 1, timeoutMs);
  return (ret > 0 && (pfd.revents & POLLIN));
}

// ─────────────────────────────────────────────────────────────────────────────
// Tokenize input  ("submit jobA 256" → ["submit", "jobA", "256"])
// ─────────────────────────────────────────────────────────────────────────────

vector<string> CLI::tokenize(const string &input) {
  vector<string> tokens;
  istringstream iss(input);
  string tok;
  while (iss >> tok)
    tokens.push_back(tok);
  return tokens;
}

// ─────────────────────────────────────────────────────────────────────────────
// Welcome banner
// ─────────────────────────────────────────────────────────────────────────────

void CLI::printBanner() const {
  cout << endl;
  cout << "\033[36m"; // cyan
  cout << R"(
   ╔═══════════════════════════════════════════════════════════════════╗
   ║                                                                   ║
   ║    ██████╗ ██████╗ ██╗   ██╗     █████╗ ██╗     ██╗      ██████╗  ║
   ║   ██╔════╝ ██╔══██╗██║   ██║    ██╔══██╗██║     ██║     ██╔═══██╗ ║
   ║   ██║  ███╗██████╔╝██║   ██║    ███████║██║     ██║     ██║   ██║ ║
   ║   ██║   ██║██╔═══╝ ██║   ██║    ██╔══██║██║     ██║     ██║   ██║ ║
   ║   ╚██████╔╝██║     ╚██████╔╝    ██║  ██║███████╗███████╗╚██████╔╝ ║
   ║    ╚═════╝ ╚═╝      ╚═════╝     ╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝  ║
   ║                                                                   ║
   ║          GPU Resource Allocator Simulator  v2.0                   ║
   ║          Priority-Driven Workload Scheduler                       ║
   ║                                                                   ║
   ╚═══════════════════════════════════════════════════════════════════╝
)" << "\033[0m";
  cout << "  Type \033[1mhelp\033[0m to see available commands.\n"
       << "  System runs continuously. Use \033[1mpause\033[0m/\033[1mresume"
       << "\033[0m to control.\n"
       << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// One-line tick summary
// ─────────────────────────────────────────────────────────────────────────────

void CLI::printTickSummary() const {
  int running = (int)jobs_.activeJobs().size();
  int pending = (int)sched_.pendingQueue().size();
  int buffered = (int)sched_.buffer().size();
  int tick = sched_.currentTick();

  cout << "\033[2m  [Tick " << tick << "] " << running << " running, "
       << pending << " pending, " << buffered << " buffered";
  if (sched_.isPaused())
    cout << "  ⏸ PAUSED";
  cout << "\033[0m" << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Help
// ─────────────────────────────────────────────────────────────────────────────

void CLI::cmdHelp() {
  cout << endl;
  cout << "  ┌───────────────────────── Available Commands "
          "───────────────────────────┐"
       << endl;
  cout << "  │                                                                  "
          "      │"
       << endl;
  cout << "  │  \033[1msubmit\033[0m <id> <mem> <ticks> [IMP|HIGH|MID|LOW]  "
          "Submit a job            │"
       << endl;
  cout << "  │  \033[1mrelease\033[0m <id>                    Manually release "
          "a running job        │"
       << endl;
  cout << "  │  \033[1mexpand\033[0m <id> <extra_mem>          Expand a "
          "running job's memory        │"
       << endl;
  cout << "  │  \033[1mload\033[0m <file>                      Load batch "
          "jobs from file            │"
       << endl;
  cout << "  │  \033[1mtick\033[0m [n]                         Advance n "
          "ticks manually             │"
       << endl;
  cout << "  │  \033[1mpause\033[0m                            Pause "
          "automatic tick progression     │"
       << endl;
  cout << "  │  \033[1mresume\033[0m                           Resume "
          "automatic ticks               │"
       << endl;
  cout << "  │  \033[1mqueue\033[0m                            Show pending "
          "& admission queues      │"
       << endl;
  cout << "  │  \033[1mbuffer\033[0m                           Show CPU "
          "buffer (evicted jobs)       │"
       << endl;
  cout << "  │  \033[1mstatus\033[0m                           System "
          "status & job statistics       │"
       << endl;
  cout << "  │  \033[1mmap\033[0m                              GPU memory "
          "map visualization         │"
       << endl;
  cout << "  │  \033[1mgpu-info\033[0m                         GPU hardware "
          "details (SMI style)     │"
       << endl;
  cout << "  │  \033[1mfree\033[0m                             Largest "
          "contiguous free block        │"
       << endl;
  cout << "  │  \033[1mclear\033[0m / \033[1mhelp\033[0m / "
          "\033[1mexit\033[0m                                    "
          "               │"
       << endl;
  cout << "  │                                                                  "
          "      │"
       << endl;
  cout << "  │  \033[2mExamples:\033[0m                                        "
          "                     │"
       << endl;
  cout << "  │    > submit jobA 256 15 HIGH                                     "
          "      │"
       << endl;
  cout << "  │    > load jobs.txt                                               "
          "      │"
       << endl;
  cout << "  │    > expand jobA 100                                             "
          "      │"
       << endl;
  cout << "  │    > tick 5                                                      "
          "      │"
       << endl;
  cout << "  │                                                                  "
          "      │"
       << endl;
  cout << "  "
          "└────────────────────────────────────────────────────────────────────"
          "────┘"
       << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Command handlers
// ─────────────────────────────────────────────────────────────────────────────

void CLI::cmdSubmit(const vector<string> &args) {
  if (args.size() < 4) {
    cout << "  ✗ Usage: submit <jobID> <memory> <ticks> [IMP|HIGH|MID|LOW]"
         << endl;
    return;
  }

  const string &jobId = args[1];
  int memUnits = 0, ticks = 0;

  try {
    memUnits = stoi(args[2]);
  } catch (...) {
    cout << "  ✗ Error: '" << args[2] << "' is not a valid integer." << endl;
    return;
  }
  try {
    ticks = stoi(args[3]);
  } catch (...) {
    cout << "  ✗ Error: '" << args[3] << "' is not a valid integer." << endl;
    return;
  }

  Priority pri = PRI_MID;
  if (args.size() >= 5) {
    string priStr = args[4];
    transform(priStr.begin(), priStr.end(), priStr.begin(),
              [](unsigned char c) { return toupper(c); });
    pri = stringToPriority(priStr);
  }

  string msg;
  sched_.submitFromCLI(jobId, memUnits, ticks, pri, msg);
  cout << msg << endl;
}

void CLI::cmdRelease(const vector<string> &args) {
  if (args.size() < 2) {
    cout << "  ✗ Usage: release <jobID>" << endl;
    return;
  }
  string msg;
  if (jobs_.releaseJob(args[1], msg)) {
    cout << msg << endl;
  } else {
    cout << msg << endl;
  }
  vis_.printQuickBar();
}

void CLI::cmdExpand(const vector<string> &args) {
  if (args.size() < 3) {
    cout << "  ✗ Usage: expand <jobID> <extra_memory>" << endl;
    return;
  }
  int extraMem = 0;
  try {
    extraMem = stoi(args[2]);
  } catch (...) {
    cout << "  ✗ Error: '" << args[2] << "' is not a valid integer." << endl;
    return;
  }

  string msg;
  sched_.expandJob(args[1], extraMem, msg);
  cout << msg << endl;
  vis_.printQuickBar();
  vis_.printMemoryMap();
}

void CLI::cmdLoad(const vector<string> &args) {
  if (args.size() < 2) {
    cout << "  ✗ Usage: load <filename>" << endl;
    return;
  }
  string msg;
  sched_.loadFile(args[1], msg);
  cout << msg << endl;
}

void CLI::cmdTick(const vector<string> &args) {
  int n = 1;
  if (args.size() >= 2) {
    try {
      n = stoi(args[1]);
      if (n <= 0)
        n = 1;
    } catch (...) {
      n = 1;
    }
  }

  for (int i = 0; i < n; ++i) {
    string log;
    sched_.tick(log);
    if (!log.empty())
      cout << log << endl;
  }
  printTickSummary();
  vis_.printQuickBar();
}

void CLI::cmdQueue() { vis_.printQueue(); }

void CLI::cmdBuffer() { vis_.printBuffer(); }

void CLI::cmdStatus() {
  vis_.printStatus();
  vis_.printBlockLayout();
}

void CLI::cmdMap() {
  vis_.printMemoryMap();
  vis_.printBlockLayout();
}

void CLI::cmdGpuInfo() { vis_.printGPUInfo(); }

void CLI::cmdLargestFree() {
  int largest = gpu_.memory().largestFreeBlock();
  int free = gpu_.freeMemory();
  int frag = vis_.fragmentationScore();
  cout << endl;
  cout << "  Largest contiguous free block: \033[1m" << largest
       << " units\033[0m" << endl;
  cout << "  Total free memory            : " << free << " units" << endl;
  cout << "  Fragmentation score          : " << frag << "%" << endl;
  cout << endl;
}

void CLI::cmdPause() {
  sched_.pause();
  cout << "  ⏸ Simulation paused. Use 'resume' or 'tick [n]' to advance."
       << endl;
}

void CLI::cmdResume() {
  sched_.resume();
  cout << "  ▶ Simulation resumed. Ticks advance automatically." << endl;
}

void CLI::cmdClear() {
  cout << "\033[2J\033[1;1H";
}

// ─────────────────────────────────────────────────────────────────────────────
// processCommand — dispatch a single command line
// ─────────────────────────────────────────────────────────────────────────────

void CLI::processCommand(const string &rawLine) {
  // Sanitize
  string line;
  line.reserve(rawLine.size());
  for (unsigned char c : rawLine) {
    if (c >= 32 && c < 127)
      line += static_cast<char>(c);
    else if (c == '\t')
      line += ' ';
  }

  // Trim
  auto lpos = line.find_first_not_of(" ");
  if (lpos == string::npos)
    return;
  auto rpos = line.find_last_not_of(" ");
  line = line.substr(lpos, rpos - lpos + 1);

  vector<string> tokens = tokenize(line);
  if (tokens.empty())
    return;

  string cmd = tokens[0];
  transform(cmd.begin(), cmd.end(), cmd.begin(),
            [](unsigned char c) { return tolower(c); });

  if (cmd == "exit" || cmd == "quit") {
    cout << "\n  Shutting down GPU Workload Scheduler. Goodbye!\n" << endl;
    running_ = false;
  } else if (cmd == "submit") {
    cmdSubmit(tokens);
  } else if (cmd == "release") {
    cmdRelease(tokens);
  } else if (cmd == "expand") {
    cmdExpand(tokens);
  } else if (cmd == "load") {
    cmdLoad(tokens);
  } else if (cmd == "tick") {
    cmdTick(tokens);
  } else if (cmd == "queue") {
    cmdQueue();
  } else if (cmd == "buffer") {
    cmdBuffer();
  } else if (cmd == "status") {
    cmdStatus();
  } else if (cmd == "map") {
    cmdMap();
  } else if (cmd == "gpu-info" || cmd == "gpuinfo" || cmd == "gpu_info") {
    cmdGpuInfo();
  } else if (cmd == "free") {
    cmdLargestFree();
  } else if (cmd == "pause") {
    cmdPause();
  } else if (cmd == "resume") {
    cmdResume();
  } else if (cmd == "help") {
    cmdHelp();
  } else if (cmd == "clear" || cmd == "cls") {
    cmdClear();
  } else {
    cout << "  ✗ Unknown command: '" << cmd
         << "'. Type \033[1mhelp\033[0m for available commands." << endl;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop — continuous with non-blocking input
// ─────────────────────────────────────────────────────────────────────────────

void CLI::run() {
  printBanner();
  sched_.pause(); // start paused so user can see banner and type commands

  // Detect if stdin is a TTY (interactive) or pipe
  bool interactive = isatty(STDIN_FILENO);

  if (!interactive) {
    // ── Piped / scripted input: simple blocking read ──
    string line;
    while (running_ && getline(cin, line)) {
      processCommand(line);
    }
    return;
  }

  // ── Interactive TTY: continuous loop with non-blocking input ──
  while (running_) {
    // Advance one tick if not paused
    if (!sched_.isPaused()) {
      string tickLog;
      sched_.tick(tickLog);
      if (!tickLog.empty())
        cout << tickLog << endl;
      printTickSummary();
    }

    // Show prompt
    cout << "\033[1;36mGPU-Alloc>\033[0m " << flush;

    // Non-blocking wait for input (300ms timeout)
    // When paused, wait indefinitely (block on input to save CPU)
    int timeout = sched_.isPaused() ? -1 : 300;

    if (inputAvailable(timeout)) {
      string line;
      if (!getline(cin, line)) {
        cout << endl;
        break; // EOF
      }
      processCommand(line);
    }
  }
}

