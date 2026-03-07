#include "CLI.h"
#include "GPUDevice.h"
#include "JobManager.h"

int main() {
  // Create the simulated GPU with default configuration
  GPUDevice gpu;

  // Create the job manager tied to this GPU
  JobManager jobs(gpu);

  // Launch the interactive CLI
  CLI cli(gpu, jobs);
  cli.run();

  return 0;
}
