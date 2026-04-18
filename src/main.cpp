#include "CLI.h"
#include "GPUDevice.h"
#include "JobManager.h"
#include "Scheduler.h"

int main() {
  // Create the simulated GPU with default configuration
  GPUDevice gpu;

  // Create the job manager tied to this GPU
  JobManager jobs(gpu);

  // Create the scheduler (drives job lifecycle)
  Scheduler sched(jobs);

  // Launch the interactive CLI
  CLI cli(gpu, jobs, sched);
  cli.run();

  return 0;
}
