#include "CLI.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

CLI::CLI(GPUDevice &gpu, JobManager &jobs)
    : gpu_(gpu), jobs_(jobs), vis_(gpu, jobs) {}

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
   ║          GPU Resource Allocator Simulator  v1.0                   ║
   ║          Segment-Tree Based Memory Management Engine              ║
   ║                                                                   ║
   ╚═══════════════════════════════════════════════════════════════════╝
)" << "\033[0m";
  cout << "  Type \033[1mhelp\033[0m to see available commands.\n" << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Help
// ─────────────────────────────────────────────────────────────────────────────

void CLI::cmdHelp() {
  cout << endl;
  cout << "  ┌────────────────────────── Available Commands "
          "──────────────────────────┐"
       << endl;
  cout << "  │                                                                 "
          "       │"
       << endl;
  cout << "  │  \033[1msubmit\033[0m <jobID> <mem> [time]  Submit a job "
          "(optional duration in sec)  │"
       << endl;
  cout << "  │  \033[1mrelease\033[0m <jobID>            Release a running "
          "job's resources          │"
       << endl;
  cout << "  │  \033[1mstatus\033[0m                     Show system status & "
          "job statistics        │"
       << endl;
  cout << "  │  \033[1mmap\033[0m                        Display GPU memory "
          "map visualization       │"
       << endl;
  cout << "  │  \033[1mgpu-info\033[0m                   Show GPU hardware "
          "details (SMI style)      │"
       << endl;
  cout << "  │  \033[1mfree\033[0m                       Query largest "
          "contiguous free block        │"
       << endl;
  cout << "  │  \033[1mclear\033[0m                      Clear the terminal "
          "screen                  │"
       << endl;
  cout << "  │  \033[1mhelp\033[0m                       Show this help "
          "message                     │"
       << endl;
  cout << "  │  \033[1mexit\033[0m                       Quit the simulator    "
          "                     │"
       << endl;
  cout << "  │                                                                 "
          "       │"
       << endl;
  cout << "  │  \033[2mExamples:\033[0m                                        "
          "                     │"
       << endl;
  cout << "  │    GPU-Allocator> submit jobA 256                               "
          "       │"
       << endl;
  cout << "  │    GPU-Allocator> submit jobB 200 30   (runs for 30 sec)        "
          "       │"
       << endl;
  cout << "  │    GPU-Allocator> release jobA                                  "
          "       │"
       << endl;
  cout << "  │    GPU-Allocator> map                                           "
          "       │"
       << endl;
  cout << "  │                                                                 "
          "       │"
       << endl;
  cout << "  "
          "└───────────────────────────────────────────────────────────────────"
          "─────┘"
       << endl;
  cout << endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Command handlers
// ─────────────────────────────────────────────────────────────────────────────

void CLI::cmdSubmit(const vector<string> &args) {
  if (args.size() < 3) {
    cout << "  ✗ Usage: submit <jobID> <memory_units> [duration_sec]" << endl;
    return;
  }
  const string &jobId = args[1];

  // Parse memory units
  int memUnits = 0;
  try {
    memUnits = stoi(args[2]);
  } catch (...) {
    cout << "  ✗ Error: '" << args[2] << "' is not a valid integer." << endl;
    return;
  }

  // Parse optional duration (0 → system estimates)
  int duration = 0;
  if (args.size() >= 4) {
    try {
      duration = stoi(args[3]);
      if (duration <= 0) {
        cout << "  ✗ Error: Duration must be a positive integer." << endl;
        return;
      }
    } catch (...) {
      cout << "  ✗ Error: '" << args[3] << "' is not a valid duration." << endl;
      return;
    }
  }

  string msg;
  bool ok = jobs_.submitJob(jobId, memUnits, duration, msg);
  cout << msg << endl;

  // Auto-show quick memory bar after allocation
  vis_.printQuickBar();
  if (ok) {
    vis_.printMemoryMap();
  }
}

void CLI::cmdRelease(const vector<string> &args) {
  if (args.size() < 2) {
    cout << "  ✗ Usage: release <jobID>" << endl;
    return;
  }
  string msg;
  jobs_.releaseJob(args[1], msg);
  cout << msg << endl;
  vis_.printQuickBar();
  vis_.printMemoryMap();
}

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

void CLI::cmdClear() {
  cout << "\033[2J\033[1;1H"; // ANSI escape: clear screen, move cursor to top
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop
// ─────────────────────────────────────────────────────────────────────────────

void CLI::run() {
  printBanner();

  string line;
  while (true) {
    // Auto-release expired timed jobs
    {
      string expMsg;
      int released = jobs_.tickExpiredJobs(expMsg);
      if (released > 0) {
        cout << expMsg << endl;
      }
    }

    cout << "\033[1;36mGPU-Allocator>\033[0m ";

    if (!getline(cin, line)) {
      cout << endl;
      break; // EOF
    }

    // Sanitize: build clean string keeping only printable ASCII
    // Converts tabs to spaces, discards all other control characters.
    {
      string clean;
      clean.reserve(line.size());
      for (unsigned char c : line) {
        if (c >= 32 && c < 127) {
          clean += static_cast<char>(c);
        } else if (c == '\t') {
          clean += ' ';
        }
        // discard everything else (NUL, ESC, backspace, etc.)
      }
      line = clean;
    }
    // Trim leading and trailing whitespace
    {
      auto lpos = line.find_first_not_of(" ");
      if (lpos == string::npos) {
        continue; // blank line
      }
      auto rpos = line.find_last_not_of(" ");
      line = line.substr(lpos, rpos - lpos + 1);
    }

    vector<string> tokens = tokenize(line);
    if (tokens.empty())
      continue;

    // Normalize command to lowercase
    string cmd = tokens[0];
    transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return tolower(c); });

    if (cmd == "exit" || cmd == "quit") {
      cout << "\n  Shutting down GPU Resource Allocator. Goodbye!\n" << endl;
      break;
    } else if (cmd == "submit") {
      cmdSubmit(tokens);
    } else if (cmd == "release") {
      cmdRelease(tokens);
    } else if (cmd == "status") {
      cmdStatus();
    } else if (cmd == "map") {
      cmdMap();
    } else if (cmd == "gpu-info" || cmd == "gpuinfo" || cmd == "gpu_info") {
      cmdGpuInfo();
    } else if (cmd == "free") {
      cmdLargestFree();
    } else if (cmd == "help") {
      cmdHelp();
    } else if (cmd == "clear" || cmd == "cls") {
      cmdClear();
    } else {
      cout << "  ✗ Unknown command: '" << cmd
           << "'. Type \033[1mhelp\033[0m for available commands." << endl;
    }
  }
}
