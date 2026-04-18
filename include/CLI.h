#ifndef CLI_H
#define CLI_H

#include "GPUDevice.h"
#include "JobManager.h"
#include "Scheduler.h"
#include "Visualizer.h"
#include <string>
#include <vector>

using namespace std;

/**
 * CLI — interactive command loop for the GPU Resource Allocator Simulator.
 * Phase 2: continuous loop with non-blocking input.
 */
class CLI {
public:
  CLI(GPUDevice &gpu, JobManager &jobs, Scheduler &sched);

  // Start the continuous simulation loop (blocks until user types 'exit').
  void run();

private:
  GPUDevice &gpu_;
  JobManager &jobs_;
  Scheduler &sched_;
  Visualizer vis_;
  bool running_ = true;

  // Command handlers
  void cmdSubmit(const vector<string> &args);
  void cmdRelease(const vector<string> &args);
  void cmdExpand(const vector<string> &args);
  void cmdLoad(const vector<string> &args);
  void cmdTick(const vector<string> &args);
  void cmdQueue();
  void cmdBuffer();
  void cmdStatus();
  void cmdMap();
  void cmdGpuInfo();
  void cmdLargestFree();
  void cmdPause();
  void cmdResume();
  void cmdHelp();
  void cmdClear();

  // Process a single command line
  void processCommand(const string &line);

  // Tokenize user input
  static vector<string> tokenize(const string &input);

  // Check if stdin has input available (non-blocking, timeout in ms)
  static bool inputAvailable(int timeoutMs);

  // Print the welcome banner
  void printBanner() const;

  // Print one-line tick summary
  void printTickSummary() const;
};

#endif // CLI_H
