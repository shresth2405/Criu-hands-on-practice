# CRIU Hands-On Practice

A collection of C exercises exploring Linux process internals — the concepts that underpin [CRIU (Checkpoint/Restore In Userspace)](https://criu.org/). Each exercise isolates a specific kernel subsystem that CRIU must interact with to checkpoint and restore a process.

---

## Exercises

### [`exercise_a.c`](./exercise_a.c) — Process Inspector & Comparator

**Goal:** Read and compare the internal state of two live processes using `/proc`.

Given two PIDs, it collects from each process:
- **Namespaces** — reads `/proc/<pid>/ns/` symlinks and extracts inode numbers to determine if two processes share the same namespace
- **File descriptors** — walks `/proc/<pid>/fd/` to count open FDs and detect shared open files (by matching inode numbers)
- **Memory map** — parses `/proc/<pid>/maps` to summarise VMAs: total count, anonymous vs file-backed, shared vs private, and the presence of `[heap]`, `[stack]`, `[vdso]`, `[vvar]`
- **Identity** — reads `PID`, `TGID`, and `PPID` from `/proc/<pid>/status`

Then it prints a side-by-side comparison of all of the above for the two processes.

> **CRIU connection:** Before checkpointing, CRIU performs exactly this kind of inspection to capture every piece of state it will need to restore later.

---

### [`exercise_b.c`](./exercise_b.c) — ptrace Signal Monitor

**Goal:** Use `ptrace` to observe a target process's signal lifecycle.

Attaches to a process by PID and enters a loop using `waitpid`. Whenever the target stops due to a signal, it prints the signal number and resumes the process with `PTRACE_CONT`. The loop exits when the process terminates.

The file also contains commented-out code for a more detailed experiment: reading CPU registers (`user_regs_struct`) before and after a single-step (`PTRACE_SINGLESTEP`) to observe how `RIP`, `RAX`, and `RSP` change instruction-by-instruction.

> **CRIU connection:** CRIU uses `ptrace` to freeze (seize) a process before dumping it, and to inspect/modify register state during restore.

---

### [`exercise_c.c`](./exercise_c.c) — Self Memory Map Reader

**Goal:** Parse a process's own virtual memory layout from `/proc/self/maps`.

Opens `/proc/self/maps` and prints each VMA (Virtual Memory Area) with:
- Start and end address (hex)
- Permission flags (`rwxp` / `rwxs`)
- File offset

> **CRIU connection:** CRIU reads `/proc/<pid>/maps` (and `smaps`) to enumerate every memory region that must be saved to a dump image and later restored via `mmap`.

---

### [`exercise_d.c`](./exercise_d.c) — File Descriptor Checkpoint & Restore

**Goal:** Simulate the checkpoint/restore cycle for a process's open file descriptors.

1. **Checkpoint** — walks `/proc/self/fd/`, and for each open FD records:
   - The resolved path (via `readlink`)
   - Open flags (`fcntl F_GETFL`)
   - Current file offset (`lseek`)
2. Closes all recorded FDs.
3. **Restore** — reopens each file using the saved path and flags, duplicates it onto the original FD number with `dup2`, and seeks back to the saved offset.

> **CRIU connection:** This is a miniature version of what CRIU does with file descriptors during dump and restore — one of the most complex parts of the process, since FDs can point to pipes, sockets, eventfds, and more.

---

### [`proc_lab.c`](./proc_lab.c) — Multi-threaded Target Process

**Goal:** Provide a simple long-running process with a worker thread to use as a target for the other exercises.

Spawns one background `pthread` (both the main thread and the worker sleep in an infinite loop). Run this first, note its PID, then point `exercise_a` or `exercise_b` at it.

```bash
# Compile and run as a background target
gcc -o proc_lab proc_lab.c -lpthread && ./proc_lab &
```

---

## Building

```bash
# Exercise A
gcc -o exercise_a exercise_a.c

# Exercise B
gcc -o exercise_b exercise_b.c

# Exercise C
gcc -o exercise_c exercise_c.c

# Exercise D
gcc -o exercise_d exercise_d.c

# proc_lab (target process)
gcc -o proc_lab proc_lab.c -lpthread
```

## Running

```bash
# Start a target process
./proc_lab &
TARGET_PID=$!

# Compare two processes (e.g., proc_lab and the current shell)
./exercise_a $TARGET_PID $$

# Monitor signals sent to proc_lab
./exercise_b $TARGET_PID

# Dump your own process's memory map
./exercise_c

# Checkpoint and restore your own FDs
./exercise_d
```
