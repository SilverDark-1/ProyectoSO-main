# Pepin OS - Quick Reference Card

## 🚀 Fixed Issues
- ✅ Cat command displays file contents
- ✅ ELF execution works without heap errors  
- ✅ Tasks command safely loads background processes

## 🔧 New Debug Commands

### Memory Commands
```bash
heap        # Show heap statistics
leaks       # Check for memory leaks
heapmap     # Show detailed heap map
defrag      # Defragment heap
```

### System Commands
```bash
mem         # Enhanced memory information
fsstat      # File system statistics
ps          # Process information
```

## 📋 Quick Testing

### Test File Operations
```bash
create hello.txt 100
write hello.txt "Hello World"
cat hello.txt              # Should display: Hello World
```

### Test Memory Health
```bash
heap                        # Check heap usage
leaks                       # Check for leaks
```

### Test ELF Execution
```bash
heap                        # Before execution
exec calc.elf               # Should work without errors
heap                        # After execution
```

### Test Tasks (Experimental)
```bash
heap                        # Check memory
tasks                       # Load background tasks
ps                          # Check processes
```

## 🎯 When to Use Debug Commands

- **After file operations**: `heap` and `leaks`
- **Before ELF execution**: `heap`
- **System seems slow**: `heapmap` and `defrag`
- **Regular monitoring**: `heap` and `fsstat`

## 📊 Understanding Output

### Heap Status
- Used Size < 50% = Good
- Used Size > 80% = High pressure
- Active Allocs > 20 = Potential leaks

### Memory Leaks
- 0 leaks = System clean
- 1-3 small leaks = Usually OK
- Many/large leaks = Needs attention

## 🛠️ System is Ready!
All fixes are permanently integrated. Your OS is now stable and includes professional debugging tools.