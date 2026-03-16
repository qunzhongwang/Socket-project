# IERGXXXX Socket Programming Project

> **Disclaimer:** This is my undergraduate course project. It is shared solely for the purpose of knowledge sharing and demonstrating programming techniques. All copyrighted materials have been completely removed — only code that is entirely written by me is included in this repository.

**Student ID:** X
**Name:** X

## Project Structure

```
socket/
├── student/
│   ├── student.cc            # STUDENT program (Steps 1-8)
│   ├── sockutil.cc           # Socket utility library (wrappers, robust I/O)
│   └── sockutil.h            # Header: constants (SID, ports), struct definitions
├── robot/
│   └── robot.cc              # Modified ROBOT source (for Steps 7-8)
├── Makefile                  # Build system
├── CMakeLists.txt            # Alternative CMake build
├── build/
│   ├── student               # STUDENT executable
│   └── robot                 # Modified ROBOT executable
├── Report.md                 # Design report with throughput results
├── throughput_results.csv    # Step 8 experiment data
├── throughput_linear.png     # Throughput graph (linear scale)
└── throughput_loglog.png     # Throughput graph (log-log scale)
```

## Build

Requires `g++` with C++11 support. Run from the project root:

```bash
make all        # Build robot and student into build/
make clean      # Remove compiled binaries
```

## How to Run

### Step 1-5, 7-8

```bash
# Terminal 1: Start ROBOT
make start-robot
# Terminal 2: Start STUDENT
make start-student
```

The experiment takes approximately **5 minutes** (Step 7: 30s + Step 8: 8 rounds x 30s). Results are exported to `throughput_results.csv`.

### Step 6 (Remote Execution)

By default, STUDENT connects to `127.0.0.1` (localhost). To run on two different machines, set the `REMOTE_ROBOT` environment variable:

```bash
# Machine A (runs ROBOT):
make start-robot

# Machine B (runs STUDENT, connects to Machine A):
REMOTE_ROBOT=1 make start-student
```

The remote ROBOT IP address is configured in `student/sockutil.h`:

```c
#define REMOTE_SERVER_IPADDR "172.16.75.1"
```

This is the default IE Computing Lab (iLab) configuration. The iLab machines are accessible only through the IE network. **To use a different pair of machines**, edit `REMOTE_SERVER_IPADDR` in `sockutil.h` to the target machine's IP address and rebuild. Any two machines that can reach each other over the network will work the same way -- there is nothing specific to the iLab setup other than the IP address.

## Source File Descriptions

| File                  | Description                                                                                                                              |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `student/student.cc`  | STUDENT program (Steps 1-8). Runs Steps 1-5, then performs the throughput experiment for Steps 7-8 and exports results to CSV.           |
| `robot/robot.cc`      | Modified ROBOT for Steps 7-8. After Steps 1-5, it reads `"bsXXX"` control messages from STUDENT and sends data for 30 seconds per round. |
| `student/sockutil.cc` | Shared utility library: CSAPP-style socket wrappers, robust `Sock_read`/`Sock_write` I/O helpers, CSV export, and runtime IP selection.  |
| `student/sockutil.h`  | Shared header: `LISTEN_PORT`, `SID`, IP addresses, `exp_result` struct, and function prototypes.                                         |
