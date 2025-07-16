#include "mm.h"
#include "screen.h"
#include "lib.h"

static struct heap_block *heap_start = (struct heap_block *)HEAP_START;
static struct page_zone page_zones[PAGE_HEAP_ENTRIES];

// Heap debugging and tracking
static struct heap_stats heap_statistics = {0};
static struct allocation_info allocations[MAX_ALLOCATIONS];
static int allocation_count = 0;

void init_heap(void) {
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_MAX_SIZE - sizeof(struct heap_block);
    heap_start->next = 0;
    heap_start->used = 0;
    heap_start->prev = 0;  // Initialize prev pointer
    
    // Initialize heap statistics
    heap_statistics.total_size = HEAP_MAX_SIZE;
    heap_statistics.free_size = heap_start->size;
    heap_statistics.used_size = 0;
    heap_statistics.allocations = 0;
    heap_statistics.deallocations = 0;
    heap_statistics.peak_usage = 0;
    
    // Initialize allocation tracking
    memset(allocations, 0, sizeof(allocations));
    allocation_count = 0;
    
    print("heap   : kernel heap initialized at 0x");
    print_hex(HEAP_START);
    print(" with size 0x");
    print_hex(HEAP_MAX_SIZE);
    print("\n");
}

// Helper function to add allocation tracking
static void track_allocation(void *ptr, u32 size, const char *file, int line) {
    if (allocation_count < MAX_ALLOCATIONS) {
        allocations[allocation_count].ptr = ptr;
        allocations[allocation_count].size = size;
        allocations[allocation_count].file = file;
        allocations[allocation_count].line = line;
        allocations[allocation_count].active = 1;
        allocation_count++;
    }
}

// Helper function to remove allocation tracking
static void untrack_allocation(void *ptr) {
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr && allocations[i].active) {
            allocations[i].active = 0;
            return;
        }
    }
}

// Helper function to find previous block
static struct heap_block *find_prev_block(struct heap_block *block) {
    struct heap_block *curr = heap_start;
    while (curr && curr->next != block) {
        curr = curr->next;
    }
    return curr;
}

// Improved heap validation
static int validate_heap(void) {
    struct heap_block *curr = heap_start;
    u32 total_counted = 0;
    
    while (curr) {
        // Check magic number
        if (curr->magic != HEAP_MAGIC) {
            print("heap   : ERROR - Invalid magic in block at 0x");
            print_hex((u32)curr);
            print("\n");
            return 0;
        }
        
        // Check size alignment
        if (curr->size & 3) {
            print("heap   : ERROR - Unaligned size in block at 0x");
            print_hex((u32)curr);
            print("\n");
            return 0;
        }
        
        total_counted += curr->size + sizeof(struct heap_block);
        
        // Check for infinite loops
        if (total_counted > HEAP_MAX_SIZE) {
            print("heap   : ERROR - Heap corruption detected\n");
            return 0;
        }
        
        curr = curr->next;
    }
    
    return 1;
}

// Improved coalescing with both previous and next blocks
static void coalesce_blocks(struct heap_block *block) {
    if (!block || block->used) return;
    
    // Coalesce with next block
    if (block->next && !block->next->used) {
        block->size += block->next->size + sizeof(struct heap_block);
        block->next = block->next->next;
    }
    
    // Coalesce with previous block
    struct heap_block *prev = find_prev_block(block);
    if (prev && !prev->used) {
        prev->size += block->size + sizeof(struct heap_block);
        prev->next = block->next;
    }
}

void *kmalloc_debug(u32 size, const char *file, int line) {
    struct heap_block *curr, *prev = 0;
    u32 total_size;
    
    // Validate heap before allocation
    if (!validate_heap()) {
        print("heap   : ERROR - Heap validation failed before allocation\n");
        return 0;
    }
    
    // Align size to 4 bytes
    size = (size + 3) & ~3;
    if (size < HEAP_MIN_SIZE) size = HEAP_MIN_SIZE;
    
    total_size = size + sizeof(struct heap_block);
    
    curr = heap_start;
    while (curr) {
        if (!curr->used && curr->size >= size) {
            // Split block if there's enough space left
            if (curr->size > total_size + sizeof(struct heap_block) + HEAP_MIN_SIZE) {
                struct heap_block *new_block = (struct heap_block *)((u32)curr + total_size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = curr->size - total_size;
                new_block->next = curr->next;
                new_block->used = 0;
                new_block->prev = curr;
                
                curr->size = size;
                curr->next = new_block;
                
                if (new_block->next) {
                    new_block->next->prev = new_block;
                }
            }
            
            curr->used = 1;
            void *ptr = (void *)((u32)curr + sizeof(struct heap_block));
            
            // Update statistics
            heap_statistics.allocations++;
            heap_statistics.used_size += curr->size;
            heap_statistics.free_size -= curr->size;
            if (heap_statistics.used_size > heap_statistics.peak_usage) {
                heap_statistics.peak_usage = heap_statistics.used_size;
            }
            
            // Track allocation
            track_allocation(ptr, size, file, line);
            
            return ptr;
        }
        prev = curr;
        curr = curr->next;
    }
    
    // No suitable block found
    print("heap   : ERROR - Out of memory! Requested: ");
    print_dec(size);
    print(" bytes\n");
    print("heap   : Available free space: ");
    print_dec(heap_statistics.free_size);
    print(" bytes\n");
    
    return 0;
}

void *kmalloc(u32 size) {
    return kmalloc_debug(size, "unknown", 0);
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    struct heap_block *block = (struct heap_block *)((u32)ptr - sizeof(struct heap_block));
    
    if (block->magic != HEAP_MAGIC) {
        print("heap   : ERROR - Invalid free! Magic: 0x");
        print_hex(block->magic);
        print(" at 0x");
        print_hex((u32)block);
        print("\n");
        return;
    }
    
    if (!block->used) {
        print("heap   : ERROR - Double free detected at 0x");
        print_hex((u32)ptr);
        print("\n");
        return;
    }
    
    // Update statistics
    heap_statistics.deallocations++;
    heap_statistics.used_size -= block->size;
    heap_statistics.free_size += block->size;
    
    // Untrack allocation
    untrack_allocation(ptr);
    
    block->used = 0;
    
    // Improved coalescing
    coalesce_blocks(block);
}

// Heap defragmentation
void defragment_heap(void) {
    struct heap_block *curr = heap_start;
    
    print("heap   : Starting defragmentation...\n");
    
    while (curr) {
        if (!curr->used) {
            coalesce_blocks(curr);
        }
        curr = curr->next;
    }
    
    print("heap   : Defragmentation complete\n");
}

// Get heap statistics
struct heap_stats get_heap_stats(void) {
    return heap_statistics;
}

// Print heap status
void print_heap_status(void) {
    print("Heap Status:\n");
    print("============\n");
    print("Total Size    : ");
    print_dec(heap_statistics.total_size);
    print(" bytes\n");
    
    print("Used Size     : ");
    print_dec(heap_statistics.used_size);
    print(" bytes (");
    print_dec((heap_statistics.used_size * 100) / heap_statistics.total_size);
    print("%)\n");
    
    print("Free Size     : ");
    print_dec(heap_statistics.free_size);
    print(" bytes (");
    print_dec((heap_statistics.free_size * 100) / heap_statistics.total_size);
    print("%)\n");
    
    print("Peak Usage    : ");
    print_dec(heap_statistics.peak_usage);
    print(" bytes\n");
    
    print("Allocations   : ");
    print_dec(heap_statistics.allocations);
    print("\n");
    
    print("Deallocations : ");
    print_dec(heap_statistics.deallocations);
    print("\n");
    
    print("Active Allocs : ");
    print_dec(heap_statistics.allocations - heap_statistics.deallocations);
    print("\n");
}

// Check for memory leaks
void check_memory_leaks(void) {
    int active_leaks = 0;
    
    print("Memory Leak Check:\n");
    print("==================\n");
    
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i].active) {
            active_leaks++;
            print("Leak ");
            print_dec(active_leaks);
            print(": ");
            print_dec(allocations[i].size);
            print(" bytes at 0x");
            print_hex((u32)allocations[i].ptr);
            if (allocations[i].file && allocations[i].line) {
                print(" (");
                print(allocations[i].file);
                print(":");
                print_dec(allocations[i].line);
                print(")");
            }
            print("\n");
        }
    }
    
    if (active_leaks == 0) {
        print("No memory leaks detected\n");
    } else {
        print("Total leaks: ");
        print_dec(active_leaks);
        print("\n");
    }
}

// Print detailed heap map
void print_heap_map(void) {
    struct heap_block *curr = heap_start;
    int block_num = 0;
    
    print("Heap Map:\n");
    print("=========\n");
    
    while (curr) {
        print("Block ");
        print_dec(block_num++);
        print(": 0x");
        print_hex((u32)curr);
        print(" size=");
        print_dec(curr->size);
        print(" ");
        print(curr->used ? "USED" : "FREE");
        print("\n");
        
        curr = curr->next;
    }
}

void init_page_heap(void) {
    memset(page_zones, 0, sizeof(page_zones));
    print("heap   : page heap initialized with ");
    print_dec(PAGE_HEAP_ENTRIES);
    print(" zones, max ");
    print_dec(PAGE_HEAP_MAX);
    print(" pages\n");
}

void *get_page_from_heap(void) {
    for (int i = 0; i < PAGE_HEAP_ENTRIES; i++) {
        if (!page_zones[i].used) {
            page_zones[i].start = (u32)get_page_frame();
            if (page_zones[i].start == (u32)-1) {
                print("heap   : ERROR - No physical pages available!\n");
                return 0;
            }
            
            page_zones[i].size = PAGE_SIZE;
            page_zones[i].used = 1;
            
            // Map the page in kernel space
            u32 vaddr = PAGE_HEAP_START + (i * PAGE_SIZE);
            u32 *pt = (u32 *)0xFFC00000; // Recursive mapping
            
            pt[(vaddr >> 12) & 0x3FF] = page_zones[i].start | PAGE_PRESENT | PAGE_RW;
            asm("invlpg %0"::"m"(vaddr));
            
            return (void *)vaddr;
        }
    }
    
    print("heap   : ERROR - Page heap full!\n");
    return 0;
}

void release_page_from_heap(void *ptr) {
    u32 vaddr = (u32)ptr;
    if (vaddr < PAGE_HEAP_START || vaddr >= PAGE_HEAP_START + (PAGE_HEAP_ENTRIES * PAGE_SIZE)) {
        print("heap   : ERROR - Invalid page free!\n");
        return;
    }
    
    int idx = (vaddr - PAGE_HEAP_START) / PAGE_SIZE;
    if (!page_zones[idx].used) {
        print("heap   : ERROR - Page already free!\n");
        return;
    }
    
    // Unmap the page
    u32 *pt = (u32 *)0xFFC00000;
    pt[(vaddr >> 12) & 0x3FF] = 0;
    asm("invlpg %0"::"m"(vaddr));
    
    // Release physical page
    release_page_frame(page_zones[idx].start);
    page_zones[idx].used = 0;
}