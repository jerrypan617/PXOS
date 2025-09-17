#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

// Process state definitions
typedef enum {
    PROCESS_STATE_NEW,        // Newly created
    PROCESS_STATE_READY,      // Ready
    PROCESS_STATE_RUNNING,    // Running
    PROCESS_STATE_BLOCKED,    // Blocked
    PROCESS_STATE_TERMINATED  // Terminated
} process_state_t;

// Process priority
typedef enum {
    PROCESS_PRIORITY_LOW = 1,
    PROCESS_PRIORITY_NORMAL = 2,
    PROCESS_PRIORITY_HIGH = 3,
    PROCESS_PRIORITY_CRITICAL = 4
} process_priority_t;

// Process Control Block (PCB)
typedef struct process_control_block {
    uint32_t pid;                    // Process ID
    char name[32];                   // Process name
    process_state_t state;           // Process state
    process_priority_t priority;     // Process priority
    
    // Register context
    uint32_t eax, ebx, ecx, edx;     // General purpose registers
    uint32_t esi, edi;               // Index registers
    uint32_t esp, ebp;               // Stack pointers
    uint32_t eip;                    // Instruction pointer
    uint32_t eflags;                 // Flags register
    uint32_t cs, ds, es, fs, gs, ss; // Segment registers
    
    // Memory management
    uint32_t stack_base;             // Stack base address
    uint32_t stack_size;             // Stack size
    uint32_t heap_base;              // Heap base address
    uint32_t heap_size;              // Heap size
    
    // Time management
    uint32_t cpu_time;               // CPU time
    uint32_t creation_time;          // Creation time
    uint32_t last_run_time;          // Last run time
    
    // Scheduling information
    uint32_t time_slice;             // Time slice
    uint32_t remaining_slice;        // Remaining time slice
    
    // Linked list pointers
    struct process_control_block* next;
    struct process_control_block* prev;
    
    // Parent and child processes
    struct process_control_block* parent;
    struct process_control_block* children;
    struct process_control_block* sibling;
    
    // Process exit code
    int32_t exit_code;
    
    // Process resources
    uint32_t open_files[16];         // Open file descriptors
    uint32_t file_count;             // Number of open files
} pcb_t;

// Process manager state
typedef struct {
    pcb_t* running_process;          // Currently running process
    pcb_t* ready_queue;              // Ready queue
    pcb_t* blocked_queue;            // Blocked queue
    pcb_t* terminated_queue;         // Terminated queue
    
    uint32_t next_pid;               // Next process ID
    uint32_t process_count;          // Total process count
    uint32_t max_processes;          // Maximum processes
    
    // Scheduler state
    uint32_t time_slice_quantum;     // Time slice size
    uint32_t current_tick;           // Current clock tick
    uint32_t scheduler_ticks;        // Scheduler clock ticks
} process_manager_t;

// 进程管理函数声明

// 初始化进程管理器
int process_manager_init(void);

// 进程创建和销毁
int process_create(const char* name, void* entry_point, process_priority_t priority, uint32_t stack_size);
int process_terminate(uint32_t pid);
int process_kill(uint32_t pid);

// 进程调度
void process_scheduler(void);
void process_switch(pcb_t* new_process);
void process_yield(void);

// 进程状态管理
int process_block(uint32_t pid);
int process_unblock(uint32_t pid);
int process_suspend(uint32_t pid);
int process_resume(uint32_t pid);

// 进程查询
pcb_t* process_get_by_pid(uint32_t pid);
pcb_t* process_get_current(void);
int process_get_list(pcb_t* processes, uint32_t max_count, uint32_t* count);

// 进程信息
int process_get_info(uint32_t pid, pcb_t* info);
int process_set_priority(uint32_t pid, process_priority_t priority);
int process_get_stats(process_manager_t* stats);

// 进程间通信
int process_send_signal(uint32_t pid, uint32_t signal);
int process_wait(uint32_t pid, int32_t* exit_code);

// 上下文切换
void process_save_context(pcb_t* pcb);
void process_restore_context(pcb_t* pcb);

// 调度器
void scheduler_init(void);
void scheduler_tick(void);
void scheduler_add_process(pcb_t* process);
void scheduler_remove_process(pcb_t* process);

// 进程管理工具函数
const char* process_state_to_string(process_state_t state);
const char* process_priority_to_string(process_priority_t priority);
void process_print_info(pcb_t* pcb);

// 常量定义
#define MAX_PROCESSES 64
#define DEFAULT_STACK_SIZE 4096
#define DEFAULT_TIME_SLICE 10
#define PROCESS_NAME_MAX 31

// 错误代码
#define PROCESS_SUCCESS 0
#define PROCESS_ERROR_INVALID_PID -1
#define PROCESS_ERROR_NOT_FOUND -2
#define PROCESS_ERROR_ALREADY_EXISTS -3
#define PROCESS_ERROR_INVALID_STATE -4
#define PROCESS_ERROR_NO_MEMORY -5
#define PROCESS_ERROR_INVALID_PARAM -6
#define PROCESS_ERROR_QUEUE_FULL -7

#endif // PROCESS_H
