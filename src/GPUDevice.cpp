#include "GPUDevice.h"

GPUDevice::GPUDevice() : cfg_(), mem_(cfg_.totalVRAM) {}

GPUDevice::GPUDevice(const GPUConfig &cfg) : cfg_(cfg), mem_(cfg.totalVRAM) {}

const GPUConfig &GPUDevice::config() const { return cfg_; }

SegmentTree &GPUDevice::memory() { return mem_; }
const SegmentTree &GPUDevice::memory() const { return mem_; }

int GPUDevice::usedMemory() const { return cfg_.totalVRAM - mem_.totalFree(); }

int GPUDevice::freeMemory() const { return mem_.totalFree(); }

double GPUDevice::utilization() const {
  return static_cast<double>(usedMemory()) / cfg_.totalVRAM;
}
