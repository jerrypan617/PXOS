#include "memory.h"
#include "../drivers/vga/vga.h"
#include "../lib/string.h"
#include "interrupt.h"

// 全局变量
static page_directory_t* current_page_directory = NULL;
static memory_block_t* memory_map = NULL;
static memory_stats_t memory_stats = {0};
static uint32_t* physical_page_bitmap = NULL;
static uint32_t bitmap_size = 0;
static uint32_t heap_start = 0;
static uint32_t heap_end = 0;
static uint32_t heap_current = 0;

// 内部函数声明
static void setup_identity_paging(void);
static void setup_kernel_paging(void);
static memory_block_t* find_free_block(size_t size);
static memory_block_t* split_block(memory_block_t* block, size_t size);
static void merge_adjacent_blocks(void);
static void update_memory_stats(void);
static void init_physical_page_bitmap(void);
static uint32_t find_free_physical_page(void);
static void set_bitmap_bit(uint32_t page, bool set);
static bool get_bitmap_bit(uint32_t page);

// 初始化内存管理系统
void memory_init(void) {
    vga_putstr("Initializing memory management...\n");
    
    // 初始化内存统计
    memset(&memory_stats, 0, sizeof(memory_stats_t));
    memory_stats.total_memory = 0x10000000; // 假设256MB内存
    memory_stats.free_memory = memory_stats.total_memory;
    
    // 初始化物理页面位图
    init_physical_page_bitmap();
    
    // 初始化分页
    paging_init();
    
    // 初始化堆
    heap_init();
    
    vga_putstr("Memory management initialized successfully!\n");
}

// 初始化分页系统
void paging_init(void) {
    vga_putstr("Setting up paging...\n");
    
    // 使用更安全的内存地址
    current_page_directory = (page_directory_t*)0x1000000; // 使用16MB处的内存
    
    // 清零页目录
    memset(current_page_directory, 0, sizeof(page_directory_t));
    
    // 设置身份映射（前4MB）
    setup_identity_paging();
    
    // 设置内核分页
    setup_kernel_paging();
    
    // 暂时不启用分页，避免地址映射问题
    // enable_paging();
    
    vga_putstr("Paging setup completed (not enabled)\n");
}

// 设置身份映射
static void setup_identity_paging(void) {
    // 为前4MB创建身份映射
    for (uint32_t i = 0; i < 1024; i++) {
        uint32_t physical_addr = i * PAGE_SIZE;
        uint32_t virtual_addr = physical_addr;
        
        map_page(virtual_addr, physical_addr, 
                PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
    }
}

// 设置内核分页
static void setup_kernel_paging(void) {
    // 映射内核代码和数据段
    uint32_t kernel_start = 0x100000;  // 1MB
    uint32_t kernel_size = 0x100000;   // 1MB
    
    for (uint32_t i = 0; i < kernel_size / PAGE_SIZE; i++) {
        uint32_t physical_addr = kernel_start + (i * PAGE_SIZE);
        uint32_t virtual_addr = physical_addr;
        
        map_page(virtual_addr, physical_addr,
                PAGE_PRESENT | PAGE_WRITABLE);
    }
}

// 初始化堆
void heap_init(void) {
    heap_start = KERNEL_HEAP_START;
    heap_end = heap_start + KERNEL_HEAP_SIZE;
    heap_current = heap_start;
    
    // 创建初始内存块
    memory_block_t* initial_block = (memory_block_t*)heap_start;
    initial_block->start_addr = heap_start + sizeof(memory_block_t);
    initial_block->size = KERNEL_HEAP_SIZE - sizeof(memory_block_t);
    initial_block->type = MEMORY_FREE;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    
    memory_map = initial_block;
    
    vga_putstr("Heap initialized at 0x");
    vga_puthex(heap_start);
    vga_putstr("\n");
}

// 初始化物理页面位图
static void init_physical_page_bitmap(void) {
    // 计算需要的位图大小（假设最大256MB内存）
    uint32_t max_pages = 0x10000000 / PAGE_SIZE; // 256MB / 4KB
    bitmap_size = (max_pages + 31) / 32; // 每个uint32_t包含32位
    
    // 分配位图内存
    physical_page_bitmap = (uint32_t*)0x500000; // 在5MB处分配位图
    
    // 清零位图
    memset(physical_page_bitmap, 0, bitmap_size * sizeof(uint32_t));
    
    // 标记已使用的页面（前6MB）
    uint32_t used_pages = 0x600000 / PAGE_SIZE;
    for (uint32_t i = 0; i < used_pages; i++) {
        set_bitmap_bit(i, true);
    }
}

// 分配物理页面
uint32_t alloc_physical_page(void) {
    uint32_t page = find_free_physical_page();
    if (page != 0) {
        set_bitmap_bit(page, true);
        memory_stats.page_allocations++;
        update_memory_stats();
    }
    return page * PAGE_SIZE;
}

// 释放物理页面
void free_physical_page(uint32_t page) {
    uint32_t page_num = page / PAGE_SIZE;
    if (page_num < (bitmap_size * 32)) {
        set_bitmap_bit(page_num, false);
        memory_stats.page_deallocations++;
        update_memory_stats();
    }
}

// 查找空闲物理页面
static uint32_t find_free_physical_page(void) {
    for (uint32_t i = 0; i < bitmap_size; i++) {
        if (physical_page_bitmap[i] != 0xFFFFFFFF) {
            // 在这个uint32_t中找到第一个0位
            for (uint32_t j = 0; j < 32; j++) {
                if (!(physical_page_bitmap[i] & (1 << j))) {
                    return i * 32 + j;
                }
            }
        }
    }
    return 0; // 没有找到空闲页面
}

// 设置位图位
static void set_bitmap_bit(uint32_t page, bool set) {
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    
    if (set) {
        physical_page_bitmap[index] |= (1 << bit);
    } else {
        physical_page_bitmap[index] &= ~(1 << bit);
    }
}

// 获取位图位
static bool get_bitmap_bit(uint32_t page) {
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    return (physical_page_bitmap[index] & (1 << bit)) != 0;
}

// 映射页面
bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = get_page_directory_index(virtual_addr);
    uint32_t pt_index = get_page_table_index(virtual_addr);
    
    // 检查页目录项是否存在
    if (!(current_page_directory->entries[pd_index].present)) {
        // 创建页表
        page_table_t* pt = create_page_table();
        if (!pt) {
            return false;
        }
        
        // 设置页目录项
        current_page_directory->entries[pd_index].address = ((uint32_t)pt) >> 12;
        current_page_directory->entries[pd_index].present = 1;
        current_page_directory->entries[pd_index].rw = 1;
        current_page_directory->entries[pd_index].user = 1;
    }
    
    // 获取页表
    page_table_t* pt = (page_table_t*)(current_page_directory->entries[pd_index].address << 12);
    
    // 设置页表项
    pt->entries[pt_index].address = physical_addr >> 12;
    pt->entries[pt_index].present = (flags & PAGE_PRESENT) ? 1 : 0;
    pt->entries[pt_index].rw = (flags & PAGE_WRITABLE) ? 1 : 0;
    pt->entries[pt_index].user = (flags & PAGE_USER) ? 1 : 0;
    pt->entries[pt_index].global = (flags & PAGE_GLOBAL) ? 1 : 0;
    
    return true;
}

// 取消映射页面
bool unmap_page(uint32_t virtual_addr) {
    uint32_t pd_index = get_page_directory_index(virtual_addr);
    uint32_t pt_index = get_page_table_index(virtual_addr);
    
    if (!current_page_directory->entries[pd_index].present) {
        return false;
    }
    
    page_table_t* pt = (page_table_t*)(current_page_directory->entries[pd_index].address << 12);
    pt->entries[pt_index].present = 0;
    
    return true;
}

// 创建页表
page_table_t* create_page_table(void) {
    // 在分页启用前，使用简单的内存分配
    static uint32_t next_page_table_addr = 0x700000; // 从7MB开始分配页表
    uint32_t page_addr = next_page_table_addr;
    next_page_table_addr += PAGE_SIZE;
    
    page_table_t* pt = (page_table_t*)page_addr;
    memset(pt, 0, sizeof(page_table_t));
    return pt;
}

// 销毁页表
void destroy_page_table(page_table_t* pt) {
    if (pt) {
        free_physical_page((uint32_t)pt);
    }
}

// 内核内存分配
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // 对齐到4字节边界
    size = (size + 3) & ~3;
    
    memory_block_t* block = find_free_block(size);
    if (!block) {
        return NULL;
    }
    
    if (block->size > size + sizeof(memory_block_t)) {
        block = split_block(block, size);
    }
    
    block->type = MEMORY_ALLOCATED;
    update_memory_stats();
    
    return (void*)(block->start_addr);
}

// 内核内存释放
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // 找到对应的内存块
    memory_block_t* block = (memory_block_t*)((uint32_t)ptr - sizeof(memory_block_t));
    
    if (block->type == MEMORY_ALLOCATED) {
        block->type = MEMORY_FREE;
        merge_adjacent_blocks();
        update_memory_stats();
    }
}

// 查找空闲内存块
static memory_block_t* find_free_block(size_t size) {
    memory_block_t* current = memory_map;
    
    while (current) {
        if (current->type == MEMORY_FREE && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// 分割内存块
static memory_block_t* split_block(memory_block_t* block, size_t size) {
    if (block->size <= size + sizeof(memory_block_t)) {
        return block;
    }
    
    // 创建新块
    memory_block_t* new_block = (memory_block_t*)(block->start_addr + size);
    new_block->start_addr = block->start_addr + size + sizeof(memory_block_t);
    new_block->size = block->size - size - sizeof(memory_block_t);
    new_block->type = MEMORY_FREE;
    new_block->next = block->next;
    new_block->prev = block;
    
    // 更新原块
    block->size = size;
    block->next = new_block;
    
    // 更新下一个块的prev指针
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
    
    return block;
}

// 合并相邻的空闲块
static void merge_adjacent_blocks(void) {
    memory_block_t* current = memory_map;
    
    while (current && current->next) {
        if (current->type == MEMORY_FREE && 
            current->next->type == MEMORY_FREE &&
            current->start_addr + current->size + sizeof(memory_block_t) == current->next->start_addr) {
            
            // 合并块
            current->size += current->next->size + sizeof(memory_block_t);
            current->next = current->next->next;
            
            if (current->next) {
                current->next->prev = current;
            }
        } else {
            current = current->next;
        }
    }
}

// 更新内存统计
static void update_memory_stats(void) {
    memory_stats.used_memory = 0;
    memory_stats.free_memory = 0;
    
    memory_block_t* current = memory_map;
    while (current) {
        if (current->type == MEMORY_ALLOCATED) {
            memory_stats.used_memory += current->size;
        } else if (current->type == MEMORY_FREE) {
            memory_stats.free_memory += current->size;
        }
        current = current->next;
    }
}

// 获取物理地址
uint32_t get_physical_address(uint32_t virtual_addr) {
    uint32_t pd_index = get_page_directory_index(virtual_addr);
    uint32_t pt_index = get_page_table_index(virtual_addr);
    
    if (!current_page_directory->entries[pd_index].present) {
        return 0;
    }
    
    page_table_t* pt = (page_table_t*)(current_page_directory->entries[pd_index].address << 12);
    if (!pt->entries[pt_index].present) {
        return 0;
    }
    
    return (pt->entries[pt_index].address << 12) | get_page_offset(virtual_addr);
}

// 工具函数
uint32_t align_to_page(uint32_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

uint32_t get_page_directory_index(uint32_t virtual_addr) {
    return (virtual_addr >> 22) & 0x3FF;
}

uint32_t get_page_table_index(uint32_t virtual_addr) {
    return (virtual_addr >> 12) & 0x3FF;
}

uint32_t get_page_offset(uint32_t virtual_addr) {
    return virtual_addr & 0xFFF;
}

bool is_page_aligned(uint32_t addr) {
    return (addr & (PAGE_SIZE - 1)) == 0;
}

// 获取内存统计信息
memory_stats_t* get_memory_stats(void) {
    return &memory_stats;
}

// 打印内存信息
void print_memory_info(void) {
    vga_putstr("=== Memory Information ===\n");
    vga_putstr("Total Memory: ");
    vga_puthex(memory_stats.total_memory);
    vga_putstr(" bytes\n");
    
    vga_putstr("Used Memory: ");
    vga_puthex(memory_stats.used_memory);
    vga_putstr(" bytes\n");
    
    vga_putstr("Free Memory: ");
    vga_puthex(memory_stats.free_memory);
    vga_putstr(" bytes\n");
    
    vga_putstr("Page Allocations: ");
    vga_puthex(memory_stats.page_allocations);
    vga_putstr("\n");
    
    vga_putstr("Page Deallocations: ");
    vga_puthex(memory_stats.page_deallocations);
    vga_putstr("\n");
    
    vga_putstr("Page Faults: ");
    vga_puthex(memory_stats.page_faults);
    vga_putstr("\n");
}

// 打印内存映射
void print_memory_map(void) {
    vga_putstr("=== Memory Map ===\n");
    vga_putstr("Address\t\tSize\t\tType\n");
    vga_putstr("----------------------------------------\n");
    
    memory_block_t* current = memory_map;
    while (current) {
        vga_puthex(current->start_addr);
        vga_putstr("\t");
        vga_puthex(current->size);
        vga_putstr("\t\t");
        
        switch (current->type) {
            case MEMORY_FREE:
                vga_putstr("FREE");
                break;
            case MEMORY_ALLOCATED:
                vga_putstr("ALLOCATED");
                break;
            case MEMORY_RESERVED:
                vga_putstr("RESERVED");
                break;
            case MEMORY_KERNEL:
                vga_putstr("KERNEL");
                break;
        }
        vga_putstr("\n");
        
        current = current->next;
    }
}

// 汇编函数包装
void enable_paging(void) {
    load_page_directory((uint32_t)current_page_directory);
    enable_paging_asm();
}

void disable_paging(void) {
    disable_paging_asm();
}

void invalidate_tlb(void) {
    invalidate_tlb_asm();
}

void invalidate_page(uint32_t virtual_addr) {
    invalidate_page_asm(virtual_addr);
}

// 其他函数的简单实现
void* kcalloc(size_t num, size_t size) {
    void* ptr = kmalloc(num * size);
    if (ptr) {
        memset(ptr, 0, num * size);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // 简单实现：分配新内存并复制数据
    void* new_ptr = kmalloc(size);
    if (new_ptr) {
        // 这里需要知道原块的大小，简化处理
        memcpy(new_ptr, ptr, size);
        kfree(ptr);
    }
    
    return new_ptr;
}

void* vmalloc(size_t size) {
    // 虚拟内存分配，简化实现
    return kmalloc(size);
}

void vfree(void* ptr) {
    kfree(ptr);
}

bool is_page_allocated(uint32_t page) {
    return get_bitmap_bit(page / PAGE_SIZE);
}

void mark_page_allocated(uint32_t page) {
    set_bitmap_bit(page / PAGE_SIZE, true);
}

void mark_page_free(uint32_t page) {
    set_bitmap_bit(page / PAGE_SIZE, false);
}

// 内存上下文相关函数（简化实现）
memory_context_t* create_memory_context(void) {
    memory_context_t* ctx = (memory_context_t*)kmalloc(sizeof(memory_context_t));
    if (ctx) {
        ctx->page_dir = current_page_directory;
        ctx->vm_areas = NULL;
        ctx->heap_start = USER_SPACE_START;
        ctx->heap_end = USER_SPACE_START + USER_SPACE_SIZE;
    }
    return ctx;
}

void destroy_memory_context(memory_context_t* ctx) {
    if (ctx) {
        kfree(ctx);
    }
}

void switch_memory_context(memory_context_t* ctx) {
    if (ctx && ctx->page_dir) {
        current_page_directory = ctx->page_dir;
        load_page_directory((uint32_t)current_page_directory);
    }
}

bool map_memory_region(memory_context_t* ctx, uint32_t start, uint32_t end, uint32_t flags) {
    // 简化实现
    (void)ctx;
    (void)start;
    (void)end;
    (void)flags;
    return true;
}

bool unmap_memory_region(memory_context_t* ctx, uint32_t start, uint32_t end) {
    // 简化实现
    (void)ctx;
    (void)start;
    (void)end;
    return true;
}

// 获取当前页目录
page_directory_t* get_current_page_directory(void) {
    return current_page_directory;
}

void print_page_directory(page_directory_t* pd) {
    vga_putstr("=== Page Directory ===\n");
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if (pd->entries[i].present) {
            vga_putstr("PD[");
            vga_puthex(i);
            vga_putstr("]: ");
            vga_puthex(pd->entries[i].address << 12);
            vga_putstr(" (");
            vga_putstr(pd->entries[i].user ? "U" : "K");
            vga_putstr(pd->entries[i].rw ? "W" : "R");
            vga_putstr(")\n");
        }
    }
}
