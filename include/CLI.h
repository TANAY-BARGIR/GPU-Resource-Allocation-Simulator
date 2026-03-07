#ifndef CLI_H
#define CLI_H

#include "GPUDevice.h"
#include "JobManager.h"
#include "Visualizer.h"
#include <string>
#include <vector>

using namespace std;

/**
 * CLI — interactive command loop for the GPU Resource Allocator Simulator.
 * Prompt: GPU-Allocator>
 */
class CLI {
public:
  CLI(GPUDevice &gpu, JobManager &jobs);

  // Start the interactive loop (blocks until user types 'exit').
  void run();

private:
  GPUDevice &gpu_;
  JobManager &jobs_;
  Visualizer vis_;

  // Command handlers
  void cmdSubmit(const vector<string> &args);
  void cmdRelease(const vector<string> &args);
  void cmdStatus();
  void cmdMap();
  void cmdGpuInfo();
  void cmdLargestFree();
  void cmdHelp();
  void cmdClear();

  // Tokenize user input
  static vector<string> tokenize(const string &input);

  // Print the welcome banner
  void printBanner() const;
};

#endif // CLI_H
