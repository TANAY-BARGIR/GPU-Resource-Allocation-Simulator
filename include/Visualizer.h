#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "GPUDevice.h"
#include "JobManager.h"
#include <string>

/**
 * Visualizer — renders GPU state as formatted ASCII output.
 *
 *   • NVIDIA-SMI-style GPU info table
 *   • ASCII memory bar
 *   • Block range table
 *   • Fragmentation summary
 */
class Visualizer {
public:
  Visualizer(const GPUDevice &gpu, const JobManager &jobs);

  // Full NVIDIA-SMI-style info panel with running jobs table
  void printGPUInfo() const;

  // ASCII memory bar  [AAAA....BBBB....]
  void printMemoryMap() const;

  // Table of block ranges
  void printBlockLayout() const;

  // Combined status: running jobs + stats + fragmentation
  void printStatus() const;

  // Compact one-line utilization bar (shown after submit/release)
  void printQuickBar() const;

  // Print a horizontal separator
  static void line(int width = 79, char fill = '-', char edge = '+');
  static void doubleLine(int width = 79);

  // Fragmentation score: 0 = no fragmentation, 100 = maximally fragmented
  int fragmentationScore() const;

private:
  const GPUDevice &gpu_;
  const JobManager &jobs_;
};

#endif // VISUALIZER_H
