#include "process.h"
#include "../memory.h"
#include "../../drivers/vga/vga.h"
#include "../../lib/string.h"
#include <stddef.h>

// 全局进程管理器
static process_manager_t g_process_manager = {0};

// 进程控制块池
static pcb_t process_pool[MAX_PROCESSES];
static uint8_t process_pool_used[MAX_PROCESSES];

// 内部函数声明
static pcb_t* allocate_pcb(void);
static void deallocate_pcb(pcb_t* pcb);
static void add_to_queue(pcb_t** queue, pcb_t* process);
static void remove_from_queue(pcb_t** queue, pcb_t* process);
static pcb_t* find_process_in_queue(pcb_t* queue, uint32_t pid);
static void setup_process_stack(pcb_t* pcb, void* entry_point);
static void schedule_next_process(void);

// 初始化进程管理器
int process_manager_init(void) {
    // 清零进程管理器状态
    memset(&g_process_manager, 0, sizeof(process_manager_t));
    
    // 清零进程池
    memset(process_pool, 0, sizeof(process_pool));
    memset(process_pool_used, 0, sizeof(process_pool_used));
    
    // 初始化配置
    g_process_manager.next_pid = 1;
    g_process_manager.max_processes = MAX_PROCESSES;
    g_process_manager.time_slice_quantum = DEFAULT_TIME_SLICE;
    g_process_manager.current_tick = 0;
    g_process_manager.scheduler_ticks = 0;
    
    // 创建空闲进程（PID 0）
    pcb_t* idle_process = allocate_pcb();
    if (!idle_process) {
        return PROCESS_ERROR_NO_MEMORY;
    }
    
    idle_process->pid = 0;
    strcpy(idle_process->name, "idle");
    idle_process->state = PROCESS_STATE_RUNNING;
    idle_process->priority = PROCESS_PRIORITY_LOW;
    idle_process->time_slice = 0;
    idle_process->remaining_slice = 0;
    
    g_process_manager.running_process = idle_process;
    g_process_manager.process_count = 1;
    
    return PROCESS_SUCCESS;
}

// 分配进程控制块
static pcb_t* allocate_pcb(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_pool_used[i]) {
            process_pool_used[i] = 1;
            memset(&process_pool[i], 0, sizeof(pcb_t));
            return &process_pool[i];
        }
    }
    return NULL;
}

// 释放进程控制块
static void deallocate_pcb(pcb_t* pcb) {
    if (!pcb) return;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&process_pool[i] == pcb) {
            process_pool_used[i] = 0;
            break;
        }
    }
}

// 添加进程到队列
static void add_to_queue(pcb_t** queue, pcb_t* process) {
    if (!queue || !process) return;
    
    process->next = *queue;
    if (*queue) {
        (*queue)->prev = process;
    }
    process->prev = NULL;
    *queue = process;
}

// 从队列中移除进程
static void remove_from_queue(pcb_t** queue, pcb_t* process) {
    if (!queue || !process) return;
    
    if (process->prev) {
        process->prev->next = process->next;
    } else {
        *queue = process->next;
    }
    
    if (process->next) {
        process->next->prev = process->prev;
    }
    
    process->next = NULL;
    process->prev = NULL;
}

// 在队列中查找进程
static pcb_t* find_process_in_queue(pcb_t* queue, uint32_t pid) {
    pcb_t* current = queue;
    while (current) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 设置进程栈
static void setup_process_stack(pcb_t* pcb, void* entry_point) {
    if (!pcb || !entry_point) return;
    
    // 分配栈空间
    pcb->stack_size = DEFAULT_STACK_SIZE;
    pcb->stack_base = (uint32_t)kmalloc(pcb->stack_size);
    
    if (!pcb->stack_base) {
        return;
    }
    
    // 设置栈指针（栈向下增长）
    pcb->esp = pcb->stack_base + pcb->stack_size - 16;
    pcb->ebp = pcb->esp;
    
    // 设置指令指针
    pcb->eip = (uint32_t)entry_point;
    
    // 设置段寄存器
    pcb->cs = 0x08;  // 内核代码段
    pcb->ds = 0x10;  // 内核数据段
    pcb->es = 0x10;
    pcb->fs = 0x10;
    pcb->gs = 0x10;
    pcb->ss = 0x10;
    
    // 设置标志寄存器
    pcb->eflags = 0x202;  // 中断使能
    
    // 在栈上设置返回地址（进程退出时调用process_terminate）
    uint32_t* stack = (uint32_t*)pcb->esp;
    stack[0] = 0;  // 返回地址（进程退出）
    stack[1] = (uint32_t)process_terminate;  // 退出函数
    stack[2] = pcb->pid;  // 参数
}

// 创建进程
int process_create(const char* name, void* entry_point, process_priority_t priority, uint32_t stack_size) {
    (void)stack_size;  // 暂时未使用，避免警告
    
    if (!name || !entry_point) {
        return PROCESS_ERROR_INVALID_PARAM;
    }
    
    if (g_process_manager.process_count >= g_process_manager.max_processes) {
        return PROCESS_ERROR_QUEUE_FULL;
    }
    
    // 分配进程控制块
    pcb_t* new_process = allocate_pcb();
    if (!new_process) {
        return PROCESS_ERROR_NO_MEMORY;
    }
    
    // 初始化进程信息
    new_process->pid = g_process_manager.next_pid++;
    strncpy(new_process->name, name, PROCESS_NAME_MAX);
    new_process->name[PROCESS_NAME_MAX] = '\0';
    new_process->state = PROCESS_STATE_NEW;
    new_process->priority = priority;
    new_process->time_slice = g_process_manager.time_slice_quantum;
    new_process->remaining_slice = new_process->time_slice;
    new_process->creation_time = g_process_manager.current_tick;
    new_process->last_run_time = 0;
    new_process->cpu_time = 0;
    new_process->exit_code = 0;
    new_process->file_count = 0;
    
    // 设置进程栈
    setup_process_stack(new_process, entry_point);
    
    // 添加到就绪队列
    new_process->state = PROCESS_STATE_READY;
    add_to_queue(&g_process_manager.ready_queue, new_process);
    
    g_process_manager.process_count++;
    
    return new_process->pid;
}

// 终止进程
int process_terminate(uint32_t pid) {
    pcb_t* process = process_get_by_pid(pid);
    if (!process) {
        return PROCESS_ERROR_NOT_FOUND;
    }
    
    // 从当前队列中移除
    if (process == g_process_manager.running_process) {
        g_process_manager.running_process = NULL;
    } else {
        remove_from_queue(&g_process_manager.ready_queue, process);
        remove_from_queue(&g_process_manager.blocked_queue, process);
    }
    
    // 添加到终止队列
    process->state = PROCESS_STATE_TERMINATED;
    add_to_queue(&g_process_manager.terminated_queue, process);
    
    // 释放资源
    if (process->stack_base) {
        kfree((void*)process->stack_base);
    }
    if (process->heap_base) {
        kfree((void*)process->heap_base);
    }
    
    // 释放进程控制块
    deallocate_pcb(process);
    
    g_process_manager.process_count--;
    
    // 如果当前进程被终止，调度下一个进程
    if (process == g_process_manager.running_process) {
        schedule_next_process();
    }
    
    return PROCESS_SUCCESS;
}

// 杀死进程
int process_kill(uint32_t pid) {
    return process_terminate(pid);
}

// 进程调度器
void process_scheduler(void) {
    g_process_manager.scheduler_ticks++;
    
    // 如果当前进程时间片用完，需要切换
    if (g_process_manager.running_process && 
        g_process_manager.running_process->remaining_slice <= 0) {
        
        // 保存当前进程上下文
        if (g_process_manager.running_process->state == PROCESS_STATE_RUNNING) {
            g_process_manager.running_process->state = PROCESS_STATE_READY;
            add_to_queue(&g_process_manager.ready_queue, g_process_manager.running_process);
        }
        
        // 调度下一个进程
        schedule_next_process();
    }
}

// 调度下一个进程
static void schedule_next_process(void) {
    pcb_t* next_process = NULL;
    
    // 从就绪队列中选择下一个进程（简单轮转调度）
    if (g_process_manager.ready_queue) {
        next_process = g_process_manager.ready_queue;
        remove_from_queue(&g_process_manager.ready_queue, next_process);
    }
    
    // 如果没有就绪进程，使用空闲进程
    if (!next_process) {
        next_process = process_get_by_pid(0);  // 空闲进程
    }
    
    if (next_process) {
        process_switch(next_process);
    }
}

// 进程切换
void process_switch(pcb_t* new_process) {
    if (!new_process) return;
    
    pcb_t* old_process = g_process_manager.running_process;
    
    // 保存旧进程上下文
    if (old_process && old_process->state == PROCESS_STATE_RUNNING) {
        process_save_context(old_process);
    }
    
    // 切换到新进程
    g_process_manager.running_process = new_process;
    new_process->state = PROCESS_STATE_RUNNING;
    new_process->remaining_slice = new_process->time_slice;
    new_process->last_run_time = g_process_manager.current_tick;
    
    // 恢复新进程上下文
    process_restore_context(new_process);
}

// 让出CPU
void process_yield(void) {
    if (g_process_manager.running_process) {
        g_process_manager.running_process->remaining_slice = 0;
        process_scheduler();
    }
}

// 根据PID获取进程
pcb_t* process_get_by_pid(uint32_t pid) {
    // 检查当前运行进程
    if (g_process_manager.running_process && g_process_manager.running_process->pid == pid) {
        return g_process_manager.running_process;
    }
    
    // 检查就绪队列
    pcb_t* process = find_process_in_queue(g_process_manager.ready_queue, pid);
    if (process) return process;
    
    // 检查阻塞队列
    process = find_process_in_queue(g_process_manager.blocked_queue, pid);
    if (process) return process;
    
    // 检查终止队列
    process = find_process_in_queue(g_process_manager.terminated_queue, pid);
    if (process) return process;
    
    return NULL;
}

// 获取当前进程
pcb_t* process_get_current(void) {
    return g_process_manager.running_process;
}

// 保存进程上下文
void process_save_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 这里应该保存当前CPU寄存器到PCB
    // 由于我们在C代码中，实际的寄存器保存需要在汇编中完成
    // 这里只是标记需要保存上下文
}

// 恢复进程上下文
void process_restore_context(pcb_t* pcb) {
    if (!pcb) return;
    
    // 这里应该从PCB恢复CPU寄存器
    // 由于我们在C代码中，实际的寄存器恢复需要在汇编中完成
    // 这里只是标记需要恢复上下文
}

// 获取进程信息
int process_get_info(uint32_t pid, pcb_t* info) {
    pcb_t* process = process_get_by_pid(pid);
    if (!process || !info) {
        return PROCESS_ERROR_NOT_FOUND;
    }
    
    *info = *process;
    return PROCESS_SUCCESS;
}

// 设置进程优先级
int process_set_priority(uint32_t pid, process_priority_t priority) {
    pcb_t* process = process_get_by_pid(pid);
    if (!process) {
        return PROCESS_ERROR_NOT_FOUND;
    }
    
    process->priority = priority;
    return PROCESS_SUCCESS;
}

// 获取进程管理器统计信息
int process_get_stats(process_manager_t* stats) {
    if (!stats) {
        return PROCESS_ERROR_INVALID_PARAM;
    }
    
    *stats = g_process_manager;
    return PROCESS_SUCCESS;
}

// 状态转字符串
const char* process_state_to_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_NEW: return "NEW";
        case PROCESS_STATE_READY: return "READY";
        case PROCESS_STATE_RUNNING: return "RUNNING";
        case PROCESS_STATE_BLOCKED: return "BLOCKED";
        case PROCESS_STATE_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

// 优先级转字符串
const char* process_priority_to_string(process_priority_t priority) {
    switch (priority) {
        case PROCESS_PRIORITY_LOW: return "LOW";
        case PROCESS_PRIORITY_NORMAL: return "NORMAL";
        case PROCESS_PRIORITY_HIGH: return "HIGH";
        case PROCESS_PRIORITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// 打印进程信息
void process_print_info(pcb_t* pcb) {
    if (!pcb) return;
    
    vga_putstr("PID: ");
    vga_puthex(pcb->pid);
    vga_putstr(" | Name: ");
    vga_putstr(pcb->name);
    vga_putstr(" | State: ");
    vga_putstr(process_state_to_string(pcb->state));
    vga_putstr(" | Priority: ");
    vga_putstr(process_priority_to_string(pcb->priority));
    vga_putstr(" | CPU Time: ");
    vga_puthex(pcb->cpu_time);
    vga_putstr("\n");
}

// 获取进程列表
int process_get_list(pcb_t* processes, uint32_t max_count, uint32_t* count) {
    if (!processes || !count) {
        return PROCESS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // 添加当前运行进程
    if (g_process_manager.running_process && *count < max_count) {
        processes[*count] = *g_process_manager.running_process;
        (*count)++;
    }
    
    // 添加就绪队列中的进程
    pcb_t* current = g_process_manager.ready_queue;
    while (current && *count < max_count) {
        processes[*count] = *current;
        (*count)++;
        current = current->next;
    }
    
    // 添加阻塞队列中的进程
    current = g_process_manager.blocked_queue;
    while (current && *count < max_count) {
        processes[*count] = *current;
        (*count)++;
        current = current->next;
    }
    
    return PROCESS_SUCCESS;
}
