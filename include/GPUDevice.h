#ifndef GPU_DEVICE_H
#define GPU_DEVICE_H

#include "SegmentTree.h"
#include <string>

using namespace std;

/**
 * Simulated GPU configuration — mimics real GPU specs for display purposes.
 */
struct GPUConfig {
  string name = "Tesla T4X";
  string architecture = "Turing (Simulated)";
  string driverVer = "535.129.03";
  string cudaVer = "12.2 (Simulated)";
  string busId = "00000000:00:04.0";
  int totalVRAM = 1024; // memory units
  int computeCores = 2560;
  int tensorCores = 320;
  int smCount = 40; // streaming multiprocessors
  string memoryType = "GDDR6";
  int tdpWatts = 70; // simulated power cap
};

/**
 * GPUDevice — owns the segment tree and provides a resource interface.
 */
class GPUDevice {
public:
  GPUDevice();
  explicit GPUDevice(const GPUConfig &cfg);

  const GPUConfig &config() const;
  SegmentTree &memory();
  const SegmentTree &memory() const;

  int usedMemory() const;
  int freeMemory() const;
  double utilization() const; // 0.0 – 1.0

private:
  GPUConfig cfg_;
  SegmentTree mem_;
};

#endif // GPU_DEVICE_H
