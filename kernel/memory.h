#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 内存管理常量
#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024
#define KERNEL_START 0x100000  // 1MB
#define KERNEL_HEAP_START 0x200000  // 2MB
#define KERNEL_HEAP_SIZE 0x100000   // 1MB heap
#define USER_SPACE_START 0x400000   // 4MB
#define USER_SPACE_SIZE 0x3C00000   // 60MB user space

// 页表项标志位
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4
#define PAGE_WRITE_THROUGH 0x8
#define PAGE_CACHE_DISABLE 0x10
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_SIZE_4MB 0x80
#define PAGE_GLOBAL 0x100

// 内存区域类型
typedef enum {
    MEMORY_FREE = 0,
    MEMORY_ALLOCATED,
    MEMORY_RESERVED,
    MEMORY_KERNEL
} memory_type_t;

// 内存块结构
typedef struct memory_block {
    uint32_t start_addr;
    uint32_t size;
    memory_type_t type;
    struct memory_block* next;
    struct memory_block* prev;
} memory_block_t;

// 页表项结构
typedef struct {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t pwt : 1;
    uint32_t pcd : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t size : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t address : 20;
} page_entry_t;

// 页表结构
typedef struct {
    page_entry_t entries[PAGE_TABLE_SIZE];
} page_table_t;

// 页目录结构
typedef struct {
    page_entry_t entries[PAGE_DIRECTORY_SIZE];
} page_directory_t;

// 虚拟内存区域
typedef struct vm_area {
    uint32_t start;
    uint32_t end;
    uint32_t flags;
    struct vm_area* next;
} vm_area_t;

// 进程内存上下文
typedef struct {
    page_directory_t* page_dir;
    vm_area_t* vm_areas;
    uint32_t heap_start;
    uint32_t heap_end;
} memory_context_t;

// 内存统计信息
typedef struct {
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t used_memory;
    uint32_t kernel_memory;
    uint32_t user_memory;
    uint32_t page_faults;
    uint32_t page_allocations;
    uint32_t page_deallocations;
} memory_stats_t;

// 函数声明

// 初始化函数
void memory_init(void);
void paging_init(void);
void heap_init(void);

// 分页管理
void enable_paging(void);
void disable_paging(void);
void set_page_directory(page_directory_t* pd);
page_directory_t* get_current_page_directory(void);
page_table_t* create_page_table(void);
void destroy_page_table(page_table_t* pt);
bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
bool unmap_page(uint32_t virtual_addr);
uint32_t get_physical_address(uint32_t virtual_addr);
void invalidate_tlb(void);
void invalidate_page(uint32_t virtual_addr);

// 内存分配
void* kmalloc(size_t size);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t size);
void kfree(void* ptr);
void* vmalloc(size_t size);
void vfree(void* ptr);

// 物理内存管理
uint32_t alloc_physical_page(void);
void free_physical_page(uint32_t page);
bool is_page_allocated(uint32_t page);
void mark_page_allocated(uint32_t page);
void mark_page_free(uint32_t page);

// 虚拟内存管理
memory_context_t* create_memory_context(void);
void destroy_memory_context(memory_context_t* ctx);
void switch_memory_context(memory_context_t* ctx);
bool map_memory_region(memory_context_t* ctx, uint32_t start, uint32_t end, uint32_t flags);
bool unmap_memory_region(memory_context_t* ctx, uint32_t start, uint32_t end);

// 内存信息
memory_stats_t* get_memory_stats(void);
void print_memory_info(void);
void print_page_directory(page_directory_t* pd);
void print_memory_map(void);

// 工具函数
uint32_t align_to_page(uint32_t addr);
uint32_t get_page_directory_index(uint32_t virtual_addr);
uint32_t get_page_table_index(uint32_t virtual_addr);
uint32_t get_page_offset(uint32_t virtual_addr);
bool is_page_aligned(uint32_t addr);

// 汇编函数声明
extern void load_page_directory(uint32_t page_dir);
extern void enable_paging_asm(void);
extern void disable_paging_asm(void);
extern void invalidate_tlb_asm(void);
extern void invalidate_page_asm(uint32_t virtual_addr);

#endif // MEMORY_H
