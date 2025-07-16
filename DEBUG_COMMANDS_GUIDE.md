# Pepin OS Debug Commands Guide

## Overview
Your operating system now includes powerful debugging tools to monitor memory usage, detect leaks, and troubleshoot issues. This guide covers all the new commands and how to use them effectively.

## 🔧 Memory Debugging Commands

### 1. `heap` - Show Heap Statistics
**Purpose**: Display current heap usage and allocation statistics

**Usage**:
```bash
pepin$ heap
```

**Output Example**:
```
Heap Status:
============
Total Size    : 8388608 bytes
Used Size     : 2048 bytes (0%)
Free Size     : 8386560 bytes (99%)
Peak Usage    : 4096 bytes
Allocations   : 12
Deallocations : 8
Active Allocs : 4
```

**What it tells you**:
- Total heap size (8MB)
- Current memory usage
- Peak memory usage since boot
- Number of allocations and deallocations
- Active allocations (potential leaks if high)

### 2. `leaks` - Check for Memory Leaks
**Purpose**: Detect and display memory leaks with detailed information

**Usage**:
```bash
pepin$ leaks
```

**Output Example**:
```
Memory Leak Check:
==================
Leak 1: 512 bytes at 0x200100 (fs.c:153)
Leak 2: 1024 bytes at 0x200400 (shell.c:209)
Total leaks: 2
```

**What it tells you**:
- Number of memory leaks detected
- Size of each leak
- Memory address of leaked blocks
- Source file and line number (when available)

### 3. `heapmap` - Show Detailed Heap Map
**Purpose**: Display a detailed view of all heap blocks

**Usage**:
```bash
pepin$ heapmap
```

**Output Example**:
```
Heap Map:
=========
Block 0: 0x200000 size=512 USED
Block 1: 0x200220 size=1024 FREE
Block 2: 0x200640 size=2048 USED
Block 3: 0x200e40 size=8382464 FREE
```

**What it tells you**:
- Each heap block with its address
- Size of each block
- Whether block is USED or FREE
- Helps identify fragmentation patterns

### 4. `defrag` - Defragment Heap
**Purpose**: Merge adjacent free blocks to reduce fragmentation

**Usage**:
```bash
pepin$ defrag
```

**Output Example**:
```
Current heap status:
[shows current heap stats]

heap   : Starting defragmentation...
heap   : Defragmentation complete

Heap status after defragmentation:
[shows improved heap stats]
```

**When to use**:
- When you notice high fragmentation
- After running many programs
- Before launching memory-intensive operations

## 📁 File System Debugging Commands

### 5. `fsstat` - Show File System Statistics
**Purpose**: Display file system usage and statistics

**Usage**:
```bash
pepin$ fsstat
```

**Output Example**:
```
File System Statistics:
======================
Total files: 5/32
Total size: 1500 bytes
Next free sector: 108
```

**What it tells you**:
- Number of files created vs. maximum allowed
- Total space used by all files
- Next available disk sector

### 6. `mem` - Enhanced Memory Information
**Purpose**: Show overall memory information including heap status

**Usage**:
```bash
pepin$ mem
```

**Output Example**:
```
Memory information:
===================
Heap start: 0x200000
Page heap start: 0xA00000

[Followed by detailed heap status]
```

## 🚀 Practical Usage Scenarios

### Scenario 1: Debugging File Operations
```bash
# Check memory before operations
pepin$ heap

# Perform file operations
pepin$ create test.txt 100
pepin$ write test.txt "Hello World"
pepin$ cat test.txt

# Check for leaks after operations
pepin$ leaks
pepin$ heap
```

### Scenario 2: Monitoring ELF Execution
```bash
# Check heap before execution
pepin$ heap

# Execute ELF file
pepin$ exec calc.elf

# Check for memory leaks
pepin$ leaks

# Show heap map to see memory layout
pepin$ heapmap
```

### Scenario 3: Debugging Tasks
```bash
# Check system health before tasks
pepin$ heap
pepin$ leaks

# Start background tasks
pepin$ tasks

# Monitor running processes
pepin$ ps

# Check memory usage after tasks
pepin$ heap
pepin$ leaks
```

### Scenario 4: System Maintenance
```bash
# Check file system usage
pepin$ fsstat

# Check heap fragmentation
pepin$ heapmap

# Defragment if needed
pepin$ defrag

# Verify improvement
pepin$ heap
```

## 🔍 Troubleshooting Guide

### High Memory Usage
1. Run `heap` to check current usage
2. Run `leaks` to identify memory leaks
3. Run `heapmap` to see fragmentation
4. Run `defrag` to consolidate free blocks

### System Becoming Slow
1. Check `fsstat` for file system usage
2. Check `heap` for memory pressure
3. Look for high number of active allocations
4. Consider restarting if leaks are excessive

### After Running Tasks
1. Always run `heap` to check memory usage
2. Run `ps` to see active processes
3. Monitor with `leaks` for memory leaks
4. Use `heapmap` to visualize memory layout

## 📊 Understanding Output

### Heap Status Indicators
- **Used Size < 50%**: Good memory usage
- **Used Size > 80%**: High memory pressure
- **Active Allocs > 20**: Potential memory leaks
- **Peak Usage**: Helps size future operations

### Memory Leak Indicators
- **0 leaks**: System is clean
- **1-3 small leaks**: Usually acceptable
- **Many leaks or large sizes**: Needs investigation

### Fragmentation Indicators
- **Few large FREE blocks**: Good
- **Many small FREE blocks**: Fragmented
- **Alternating USED/FREE**: High fragmentation

## 🎯 Best Practices

1. **Regular Monitoring**: Run `heap` periodically
2. **Leak Detection**: Use `leaks` after major operations
3. **Proactive Maintenance**: Run `defrag` when fragmented
4. **File System Health**: Monitor with `fsstat`
5. **Before Critical Operations**: Always check system health

## 🛠️ Integration Status

✅ **All fixes permanently integrated**:
- Cat command now displays file contents correctly
- ELF execution works without heap validation errors
- Tasks command safely loads background processes
- Enhanced debugging capabilities available

Your Pepin OS is now equipped with professional-grade debugging tools!