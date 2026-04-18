#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "GPUDevice.h"
#include "JobManager.h"
#include <string>

// Forward declaration
class Scheduler;

/**
 * Visualizer — renders GPU state as formatted ASCII output.
 *
 * Phase 2 additions:
 *   • Priority column in running jobs table
 *   • Pending queue display
 *   • CPU buffer display
 *   • Tick info in status panel
 */
class Visualizer {
public:
  Visualizer(const GPUDevice &gpu, const JobManager &jobs,
             const Scheduler &sched);

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

  // Pending queue display
  void printQueue() const;

  // CPU buffer display
  void printBuffer() const;

  // Print a horizontal separator
  static void line(int width = 79, char fill = '-', char edge = '+');
  static void doubleLine(int width = 79);

  // Fragmentation score: 0 = no fragmentation, 100 = maximally fragmented
  int fragmentationScore() const;

private:
  const GPUDevice &gpu_;
  const JobManager &jobs_;
  const Scheduler &sched_;
};

#endif // VISUALIZER_H
