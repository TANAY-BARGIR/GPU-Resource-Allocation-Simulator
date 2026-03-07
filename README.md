<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus" alt="C++17"/>
  <img src="https://img.shields.io/badge/Platform-Linux-lightgrey?style=for-the-badge&logo=linux" alt="Linux"/>
  <img src="https://img.shields.io/badge/Build-Make-green?style=for-the-badge&logo=gnu" alt="Make"/>
  <img src="https://img.shields.io/badge/License-Academic-orange?style=for-the-badge" alt="License"/>
</p>

<h1 align="center">GPU Resource Allocation Simulator</h1>

<p align="center">
  <b>A Segment-Tree Based GPU Memory Management Engine</b><br/>
  <i>Interactive CLI simulator that demonstrates O(log N) contiguous memory allocation, real-time visualization, and fragmentation analysis — built entirely in standard C++17.</i>
</p>

---

## 📌 Project Overview

Managing GPU memory efficiently is a critical challenge in modern computing — from deep learning training to real-time rendering. When multiple jobs compete for limited GPU VRAM, the allocator must find **contiguous** free blocks quickly, release them cleanly, and prevent **fragmentation** from degrading performance.

This project builds a **GPU Resource Allocator Simulator** that models a virtual GPU (inspired by the NVIDIA Tesla T4) and uses a **Segment Tree** as its core allocation engine. It is **not** a real hardware driver or CUDA scheduler — it is a systems-level simulator designed to demonstrate how advanced data structures solve real resource management problems.

### 🎯 Aim

> Design and implement an efficient GPU memory allocator using a **Segment Tree** data structure. The system should allocate and deallocate contiguous memory blocks in **O(log N)** time, automatically merge adjacent free regions, and provide rich visual feedback through an interactive command-line interface.

---

## ✨ Key Features

| Feature | Description |
|---|---|
| **Segment Tree Allocation** | O(log N) first-fit contiguous block allocation with automatic free-block merging |
| **Interactive CLI** | MySQL-style command loop with colored output, Unicode borders, and an ASCII art banner |
| **Memory Map Visualization** | Compressed ASCII bar showing allocated vs. free memory at a glance |
| **Block Layout Table** | Detailed range-wise breakdown of every allocated and free region |
| **NVIDIA-SMI Style GPU Info** | Simulated hardware panel displaying GPU name, architecture, VRAM usage, and utilization |
| **Timed & Manual Jobs** | Jobs can auto-expire after a duration or remain until manually released |
| **System-Estimated Runtimes** | When no duration is provided, the system estimates runtime proportional to memory requested |
| **Fragmentation Analysis** | Real-time fragmentation score: `1 − (largest_free / total_free)` × 100 |
| **Job Statistics** | Tracks submitted, completed, and failed job counts across the session |
| **Utilization Bar** | One-line progress bar displayed after every allocation or deallocation |

---

## 🏗️ Architecture

```
┌─────────────────────────────┐
│         main.cpp            │  Entry point
├─────────────────────────────┤
│           CLI               │  Interactive command loop & parser
├─────────────────────────────┤
│        Visualizer           │  ASCII rendering engine
├──────────┬──────────────────┤
│JobManager│    GPUDevice     │  Job lifecycle + virtual GPU model
├──────────┴──────────────────┤
│       Segment Tree          │  O(log N) allocation engine
└─────────────────────────────┘
```

### Component Breakdown

- **`SegmentTree`** — The core data structure. Each node tracks `prefixFree`, `suffixFree`, `maxFree`, and `totalLen`. During updates, adjacent free blocks are merged automatically by combining the left child's `suffixFree` with the right child's `prefixFree`. No separate merge pass is needed.

- **`GPUDevice`** — A thin wrapper that holds simulated GPU specs (Tesla T4X, 1024 VRAM units, 2560 cores, Turing architecture) and owns a `SegmentTree` instance. Provides convenience methods for used/free memory and utilization percentage.

- **`JobManager`** — Manages the full job lifecycle: submission → allocation → running → completion/release. Supports both timed jobs (auto-release after duration) and manual jobs. Assigns unique labels (A–Z) for visual identification on the memory map.

- **`Visualizer`** — Renders all ASCII output: the memory bar, block layout table, GPU info panel (NVIDIA-SMI style), system status, fragmentation score, and the quick utilization bar.

- **`CLI`** — Implements the interactive command loop with parsing, validation, case-insensitive matching, and colored ANSI output. Automatically displays the memory map after mutations.

---

## 📂 Project Structure

```
Project/
├── include/
│   ├── SegmentTree.h       # Segment tree interface
│   ├── GPUDevice.h         # GPU model & config struct
│   ├── JobManager.h        # Job lifecycle manager
│   ├── Visualizer.h        # ASCII rendering engine
│   └── CLI.h               # Command-line interface
├── src/
│   ├── main.cpp            # Entry point
│   ├── SegmentTree.cpp     # Allocation engine implementation
│   ├── GPUDevice.cpp       # GPU device implementation
│   ├── JobManager.cpp      # Job submission/release logic
│   ├── Visualizer.cpp      # All visualization rendering
│   └── CLI.cpp             # Interactive command loop
├── Makefile                 # Build rules
├── .gitignore
└── README.md
```

---

## 🔧 Prerequisites

| Requirement | Details |
|---|---|
| **Compiler** | `g++` with C++17 support |
| **OS** | Linux (tested on Ubuntu) |
| **Build Tool** | GNU Make (optional, manual `g++` invocation also works) |
| **External Libraries** | **None** — only the C++ Standard Library is used |

---

## 🚀 Build & Run

### Using Make (recommended)

```bash
# Clone or navigate to the project directory
cd Project/

# Build the executable
make

# Run the simulator
./gpualloc
```

### Manual Compilation

```bash
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude -o gpualloc \
    src/main.cpp src/SegmentTree.cpp src/GPUDevice.cpp \
    src/JobManager.cpp src/CLI.cpp src/Visualizer.cpp

./gpualloc
```

### Clean Build Artifacts

```bash
make clean
```

---

## 💻 Usage

When you launch `./gpualloc`, you enter an interactive CLI environment:

```
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
  Type help to see available commands.

GPU-Allocator>
```

### Available Commands

| Command | Description |
|---|---|
| `submit <jobID> <memory> [duration]` | Submit a job requesting `<memory>` units. Optionally specify duration in seconds; if omitted, the system estimates it automatically. |
| `release <jobID>` | Release a running job and free its GPU resources. |
| `status` | Display system status: memory usage, fragmentation, active jobs, and block layout. |
| `map` | Render the GPU memory map visualization with block layout. |
| `gpu-info` | Show GPU hardware details in an NVIDIA-SMI inspired table. |
| `free` | Query the largest contiguous free memory block. |
| `help` | Display the help menu with all commands and examples. |
| `clear` | Clear the terminal screen. |
| `exit` | Shut down the simulator and return to the shell. |

> **Note:** Commands are case-insensitive. Aliases like `gpu_info`, `gpuinfo`, and `gpu-info` are all accepted.

---

## 📸 Example Session

### 1. Submit a Job

```
GPU-Allocator> submit jobA 256
  ✓ Job 'jobA' started successfully.
    Allocated blocks : 0 – 255
    Resource usage   : 256 / 1024 units
  Memory: [██████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 256/1024 units (25.0%)

  ╔═ GPU Memory Map (1024 units) ══════════════════════════════════════╗
  ║ [AAAAAAAAAAAAAAAA................................................] ║
  ║  A=jobA  .=free                                                    ║
  ╚════════════════════════════════════════════════════════════════════╝
```

### 2. Submit Another Job

```
GPU-Allocator> submit jobB 512
  ✓ Job 'jobB' started successfully.
    Allocated blocks : 256 – 767
    Resource usage   : 768 / 1024 units
  Memory: [██████████████████████████████░░░░░░░░░░] 768/1024 units (75.0%)

  ╔═ GPU Memory Map (1024 units) ══════════════════════════════════════╗
  ║ [AAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB................] ║
  ║  A=jobA  B=jobB  .=free                                            ║
  ╚════════════════════════════════════════════════════════════════════╝
```

### 3. Check System Status

```
GPU-Allocator> status

  ┌─────────────────── System Status ────────────────────┐
  │  Total Memory     :   1024 units                     │
  │  Used  Memory     :    768 units  (75.0%)            │
  │  Free  Memory     :    256 units                     │
  │  Largest Free Block:   256 units                     │
  │  Fragmentation    :     0%                           │
  │  Active Jobs      :       2                          │
  ├─────────────────── Job Statistics ───────────────────┤
  │  Submitted:    2   Completed:    0   Failed:    0    │
  └──────────────────────────────────────────────────────┘

  Block Layout
  ───────────────────────────────────────────────────────
      0 – 255    :   jobA   (256 units)
    256 – 767    :   jobB   (512 units)
    768 – 1023   :   FREE   (256 units)
  ───────────────────────────────────────────────────────
```

### 4. View GPU Hardware Info

```
GPU-Allocator> gpu-info

+-----------------------------------------------------------------------------+
| GPU Resource Manager Simulator                        v1.0                  |
| Driver Version: 535.129.03             CUDA Version: 12.2 (Simulated)       |
+-----------------------------------------------------------------------------+
| GPU Name            Bus-Id            | Memory-Usage       | GPU-Util       |
+=============================================================================+
| 0   Tesla T4X       00000000:00:04.0  | 768 / 1024 Units   | 75%            |
+-----------------------------------------------------------------------------+
| Arch: Turing (Simulated)              Cores: 2560       TDP: 70W            |
| SMs: 40            Tensor Cores: 320  Mem Type: GDDR6                       |
+-----------------------------------------------------------------------------+
```

### 5. Release a Job

```
GPU-Allocator> release jobA
  ✓ Job 'jobA' released.
    Freed blocks: 0 – 255 (256 units)
  Memory: [████████████████████░░░░░░░░░░░░░░░░░░░░] 512/1024 units (50.0%)
```

### 6. Observe Fragmentation

After releasing `jobA`, free memory is split into two non-contiguous regions (0–255 and 768–1023). The fragmentation score will now reflect this split, demonstrating how the segment tree tracks and merges free blocks.

### 7. Exit

```
GPU-Allocator> exit
  Shutting down GPU Resource Allocator. Goodbye!
```

---

## ⚙️ How It Works

### The Segment Tree — Core Allocation Engine

The segment tree is a complete binary tree built over the GPU's memory units (default: 1024). Every internal node stores four values:

| Field | Meaning |
|---|---|
| `prefixFree` | Length of the longest contiguous free run starting from the **left edge** of the segment |
| `suffixFree` | Length of the longest contiguous free run ending at the **right edge** of the segment |
| `maxFree` | Length of the longest contiguous free run **anywhere** within the segment |
| `totalLen` | Total number of units in the segment (constant after construction) |

#### Allocation — `allocate(size)` → O(log N)

Uses a **first-fit** descent strategy:
1. Check the **left child** — if `left.maxFree >= size`, recurse left.
2. Check the **cross boundary** — if `left.suffixFree + right.prefixFree >= size`, allocate at the junction.
3. Check the **right child** — if `right.maxFree >= size`, recurse right.
4. If none fit, return failure.

#### Deallocation — `deallocate(start, size)` → O(log N)

Marks the specified range as free and propagates merging information upward. The key insight: **automatic merging** happens naturally during the `update()` pass. When two siblings both have adjacent free edges, the parent's `maxFree` absorbs the combined span. No separate merge pass or free-list traversal is needed.

#### Query — `largestFreeBlock()` → O(1)

Simply returns `tree[1].maxFree` — the root's maximum free segment.

### Why a Segment Tree?

| Approach | Allocate | Deallocate | Largest Free | Merge Cost |
|---|---|---|---|---|
| Linear Scan | O(N) | O(N) | O(N) | O(N) |
| Free List | O(K) | O(1) | O(K) | O(K) |
| **Segment Tree** | **O(log N)** | **O(log N)** | **O(1)** | **O(0)** — built-in |

The segment tree provides superior worst-case performance and eliminates explicit merge operations entirely.

### Data Structures Summary

| Structure | C++ Type | Purpose |
|---|---|---|
| Segment Tree | `vector<Node>` | O(log N) contiguous block allocation |
| Job Registry | `unordered_map<string, Job>` | Active job → allocation lookup |
| GPU Memory State | `vector<bool>` | Ground-truth per-unit occupancy status |
| CLI Tokens | `vector<string>` | Parsed command arguments |

---

## 📊 Fragmentation Score

The simulator computes a real-time fragmentation score using:

```
fragmentation = (1 − largest_free_block / total_free_memory) × 100
```

- **0%** → All free memory is in one contiguous block (no fragmentation).
- **100%** → Maximally fragmented (free memory scattered into tiny pieces).

This metric directly demonstrates the segment tree's ability to track and merge free regions.

---

## 🔮 Future Scope (Phase 2)

The current implementation covers **Phase 1** — immediate allocation with no scheduling. Planned extensions include:

| Feature | Description |
|---|---|
| **Priority Scheduling** | Jobs tagged with priority levels; a priority queue dispatches higher-priority jobs first |
| **Job Waiting Queue** | Jobs that can't be allocated immediately enter a queue and are auto-dispatched when resources free up |
| **Wait-Time Estimation** | The system estimates how long a queued job must wait based on running job durations |
| **Workload Types** | Different job types (AI training, rendering, inference) with characteristic runtime profiles |

---

## 🛠️ Technology Stack

- **Language:** C++17
- **Standard Libraries Used:** `<vector>`, `<unordered_map>`, `<string>`, `<sstream>`, `<iomanip>`, `<iostream>`, `<chrono>`, `<ctime>`, `<algorithm>`
- **Build System:** GNU Make
- **Platform:** Linux (Ubuntu)
- **External Dependencies:** None

---

## 👥 Authors

Developed by **Tanay Bargir**.

---

<p align="center">
  <i>Built with ❤️ and Segment Trees.</i>
</p>
