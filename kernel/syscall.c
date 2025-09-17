#include "syscall.h"
#include "interrupt.h"
#include "memory.h"
#include "../drivers/vga/vga.h"
#include "../lib/string.h"
#include <stddef.h>

// 系统调用表
static syscall_entry_t syscall_table[MAX_SYSCALLS];
static uint32_t syscall_count = 0;

// 当前系统调用参数（用于调试）
static syscall_args_t current_syscall_args;

// 初始化系统调用
void syscall_init(void) {
    // 清零系统调用表
    memset(syscall_table, 0, sizeof(syscall_table));
    syscall_count = 0;
    
    // 注册进程相关系统调用
    syscall_register(SYS_EXIT, sys_exit, "exit", "Terminate current process");
    syscall_register(SYS_FORK, sys_fork, "fork", "Create new process");
    syscall_register(SYS_EXEC, sys_exec, "exec", "Execute program");
    syscall_register(SYS_WAIT, sys_wait, "wait", "Wait for child process");
    syscall_register(SYS_GETPID, sys_getpid, "getpid", "Get process ID");
    syscall_register(SYS_GETPPID, sys_getppid, "getppid", "Get parent process ID");
    syscall_register(SYS_KILL, sys_kill, "kill", "Send signal to process");
    syscall_register(SYS_YIELD, sys_yield, "yield", "Yield CPU to other process");
    
    // 注册文件系统相关系统调用
    syscall_register(SYS_OPEN, sys_open, "open", "Open file");
    syscall_register(SYS_CLOSE, sys_close, "close", "Close file");
    syscall_register(SYS_READ, sys_read, "read", "Read from file");
    syscall_register(SYS_WRITE, sys_write, "write", "Write to file");
    syscall_register(SYS_SEEK, sys_seek, "seek", "Seek file position");
    syscall_register(SYS_TELL, sys_tell, "tell", "Get file position");
    syscall_register(SYS_CREATE, sys_create, "create", "Create file");
    syscall_register(SYS_DELETE, sys_delete, "delete", "Delete file");
    syscall_register(SYS_RENAME, sys_rename, "rename", "Rename file");
    syscall_register(SYS_MKDIR, sys_mkdir, "mkdir", "Create directory");
    syscall_register(SYS_RMDIR, sys_rmdir, "rmdir", "Remove directory");
    syscall_register(SYS_CHDIR, sys_chdir, "chdir", "Change directory");
    syscall_register(SYS_GETCWD, sys_getcwd, "getcwd", "Get current directory");
    syscall_register(SYS_LISTDIR, sys_listdir, "listdir", "List directory contents");
    
    // 注册内存管理相关系统调用
    syscall_register(SYS_MALLOC, sys_malloc, "malloc", "Allocate memory");
    syscall_register(SYS_FREE, sys_free, "free", "Free memory");
    syscall_register(SYS_MMAP, sys_mmap, "mmap", "Map memory");
    syscall_register(SYS_MUNMAP, sys_munmap, "munmap", "Unmap memory");
    
    // 注册进程管理相关系统调用
    syscall_register(SYS_PS, sys_ps, "ps", "List processes");
    syscall_register(SYS_SETPRIORITY, sys_setpriority, "setpriority", "Set process priority");
    syscall_register(SYS_GETINFO, sys_getinfo, "getinfo", "Get system information");
    
    // 设置系统调用中断处理程序
    idt_set_entry(SYSCALL_INT_NUM, (uint32_t)syscall_handler, 0x08, IDT_ATTR_PRESENT | IDT_ATTR_DPL_3 | IDT_ATTR_32BIT_TRAP);
}

// 注册系统调用
int syscall_register(uint32_t syscall_num, syscall_handler_t handler, const char* name, const char* description) {
    if (syscall_num >= MAX_SYSCALLS || !handler) {
        return SYSCALL_ERROR;
    }
    
    if (syscall_table[syscall_num].handler != NULL) {
        return SYSCALL_ERROR; // 系统调用已存在
    }
    
    syscall_table[syscall_num].handler = handler;
    syscall_table[syscall_num].name = name;
    syscall_table[syscall_num].description = description;
    
    if (syscall_num >= syscall_count) {
        syscall_count = syscall_num + 1;
    }
    
    return SYSCALL_SUCCESS;
}

// 查找系统调用
syscall_entry_t* syscall_find(uint32_t syscall_num) {
    if (syscall_num >= MAX_SYSCALLS) {
        return NULL;
    }
    
    if (syscall_table[syscall_num].handler == NULL) {
        return NULL;
    }
    
    return &syscall_table[syscall_num];
}

// 执行系统调用
int32_t syscall_execute(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    syscall_entry_t* entry = syscall_find(syscall_num);
    if (!entry) {
        return SYSCALL_INVALID;
    }
    
    // 保存当前系统调用参数（用于调试）
    current_syscall_args.syscall_num = syscall_num;
    current_syscall_args.arg1 = arg1;
    current_syscall_args.arg2 = arg2;
    current_syscall_args.arg3 = arg3;
    current_syscall_args.arg4 = arg4;
    current_syscall_args.arg5 = arg5;
    
    // 调用系统调用处理函数
    return entry->handler(arg1, arg2, arg3, arg4, arg5);
}

// 系统调用处理程序（汇编调用）
void syscall_handler(void) {
    // 这个函数会被汇编代码调用
    // 参数通过寄存器传递，需要汇编代码来获取
    // 这里只是占位符，实际的参数获取在汇编代码中完成
}

// 列出所有系统调用
void syscall_list(void) {
    vga_putstr("System Calls:\n");
    vga_putstr("Num | Name                | Description\n");
    vga_putstr("----|---------------------|----------------------------------------\n");
    
    for (uint32_t i = 0; i < syscall_count; i++) {
        if (syscall_table[i].handler != NULL) {
            vga_putstr(" ");
            vga_puthex(i);
            vga_putstr("  | ");
            
            // 打印系统调用名（左对齐，19字符宽度）
            char name[20];
            strncpy(name, syscall_table[i].name, 19);
            name[19] = '\0';
            vga_putstr(name);
            for (int j = strlen(name); j < 19; j++) {
                vga_putstr(" ");
            }
            vga_putstr(" | ");
            vga_putstr(syscall_table[i].description);
            vga_putstr("\n");
        }
    }
}

// ==================== 进程相关系统调用实现 ====================

int32_t sys_exit(uint32_t exit_code, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    pcb_t* current = process_get_current();
    if (current) {
        current->exit_code = (int32_t)exit_code;
        process_terminate(current->pid);
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_fork(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    // 简单的fork实现，创建当前进程的副本
    pcb_t* current = process_get_current();
    if (!current) {
        return SYSCALL_ERROR;
    }
    
    // 创建新进程（使用当前进程的入口点）
    int result = process_create("forked_process", (void*)current->eip, current->priority, DEFAULT_STACK_SIZE);
    if (result < 0) {
        return SYSCALL_ERROR;
    }
    
    return result; // 返回子进程PID
}

int32_t sys_exec(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, uint32_t arg4, uint32_t arg5) {
    (void)path_ptr; (void)argv_ptr; (void)envp_ptr; (void)arg4; (void)arg5;
    
    // exec实现需要加载新程序，这里暂时返回错误
    // 在实际实现中，需要解析可执行文件格式并加载到内存
    return SYSCALL_ERROR;
}

int32_t sys_wait(uint32_t pid_ptr, uint32_t status_ptr, uint32_t options, uint32_t arg4, uint32_t arg5) {
    (void)pid_ptr; (void)status_ptr; (void)options; (void)arg4; (void)arg5;
    
    // 简单的wait实现
    // 在实际实现中，需要等待子进程结束
    return SYSCALL_SUCCESS;
}

int32_t sys_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    pcb_t* current = process_get_current();
    if (!current) {
        return SYSCALL_ERROR;
    }
    
    return current->pid;
}

int32_t sys_getppid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    pcb_t* current = process_get_current();
    if (!current || !current->parent) {
        return SYSCALL_ERROR;
    }
    
    return current->parent->pid;
}

int32_t sys_kill(uint32_t pid, uint32_t signal, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)signal; (void)arg3; (void)arg4; (void)arg5;
    
    int result = process_kill(pid);
    if (result != PROCESS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_yield(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    process_yield();
    return SYSCALL_SUCCESS;
}

// ==================== 文件系统相关系统调用实现 ====================

int32_t sys_open(uint32_t path_ptr, uint32_t flags, uint32_t mode, uint32_t arg4, uint32_t arg5) {
    (void)mode; (void)arg4; (void)arg5;
    
    // 从用户空间复制路径字符串
    char path[FS_MAX_PATH];
    // 这里需要从用户空间复制数据，简化实现直接使用指针
    char* user_path = (char*)path_ptr;
    
    // 简单的路径验证
    if (!user_path || strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    fs_file_t file;
    int result = fs_open(path, (uint8_t)flags, &file);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    // 返回文件描述符（简化实现，直接返回文件句柄地址）
    return (int32_t)(uintptr_t)&file;
}

int32_t sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    fs_file_t* file = (fs_file_t*)(uintptr_t)fd;
    if (!file) {
        return SYSCALL_ERROR;
    }
    
    int result = fs_close(file);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_read(uint32_t fd, uint32_t buf_ptr, uint32_t count, uint32_t arg4, uint32_t arg5) {
    (void)arg4; (void)arg5;
    
    fs_file_t* file = (fs_file_t*)(uintptr_t)fd;
    if (!file || !buf_ptr) {
        return SYSCALL_ERROR;
    }
    
    // 从用户空间读取数据到内核缓冲区
    char* user_buf = (char*)buf_ptr;
    char kernel_buf[1024];
    size_t bytes_to_read = (count > sizeof(kernel_buf)) ? sizeof(kernel_buf) : count;
    
    int result = fs_read(file, kernel_buf, bytes_to_read);
    if (result < 0) {
        return SYSCALL_ERROR;
    }
    
    // 将数据复制回用户空间
    for (int i = 0; i < result; i++) {
        user_buf[i] = kernel_buf[i];
    }
    
    return result;
}

int32_t sys_write(uint32_t fd, uint32_t buf_ptr, uint32_t count, uint32_t arg4, uint32_t arg5) {
    (void)arg4; (void)arg5;
    
    fs_file_t* file = (fs_file_t*)(uintptr_t)fd;
    if (!file || !buf_ptr) {
        return SYSCALL_ERROR;
    }
    
    // 从用户空间复制数据到内核缓冲区
    char* user_buf = (char*)buf_ptr;
    char kernel_buf[1024];
    size_t bytes_to_write = (count > sizeof(kernel_buf)) ? sizeof(kernel_buf) : count;
    
    for (size_t i = 0; i < bytes_to_write; i++) {
        kernel_buf[i] = user_buf[i];
    }
    
    int result = fs_write(file, kernel_buf, bytes_to_write);
    if (result < 0) {
        return SYSCALL_ERROR;
    }
    
    return result;
}

int32_t sys_seek(uint32_t fd, uint32_t offset, uint32_t whence, uint32_t arg4, uint32_t arg5) {
    (void)arg4; (void)arg5;
    
    fs_file_t* file = (fs_file_t*)(uintptr_t)fd;
    if (!file) {
        return SYSCALL_ERROR;
    }
    
    int result = fs_seek(file, (int32_t)offset, (int)whence);
    if (result < 0) {
        return SYSCALL_ERROR;
    }
    
    return result;
}

int32_t sys_tell(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    fs_file_t* file = (fs_file_t*)(uintptr_t)fd;
    if (!file) {
        return SYSCALL_ERROR;
    }
    
    int result = fs_tell(file);
    if (result < 0) {
        return SYSCALL_ERROR;
    }
    
    return result;
}

int32_t sys_create(uint32_t path_ptr, uint32_t mode, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)mode; (void)arg3; (void)arg4; (void)arg5;
    
    char* user_path = (char*)path_ptr;
    if (!user_path) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    int result = fs_create(path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_delete(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    char* user_path = (char*)path_ptr;
    if (!user_path) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    int result = fs_delete(path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_rename(uint32_t old_path_ptr, uint32_t new_path_ptr, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    
    char* user_old_path = (char*)old_path_ptr;
    char* user_new_path = (char*)new_path_ptr;
    
    if (!user_old_path || !user_new_path) {
        return SYSCALL_ERROR;
    }
    
    char old_path[FS_MAX_PATH];
    char new_path[FS_MAX_PATH];
    
    if (strlen(user_old_path) >= FS_MAX_PATH || strlen(user_new_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(old_path, user_old_path);
    strcpy(new_path, user_new_path);
    
    int result = fs_rename(old_path, new_path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_mkdir(uint32_t path_ptr, uint32_t mode, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)mode; (void)arg3; (void)arg4; (void)arg5;
    
    char* user_path = (char*)path_ptr;
    if (!user_path) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    int result = fs_mkdir(path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_rmdir(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    char* user_path = (char*)path_ptr;
    if (!user_path) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    int result = fs_rmdir(path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_chdir(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    char* user_path = (char*)path_ptr;
    if (!user_path) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    int result = fs_chdir(path);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_getcwd(uint32_t buf_ptr, uint32_t size, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    
    char* user_buf = (char*)buf_ptr;
    if (!user_buf || size == 0) {
        return SYSCALL_ERROR;
    }
    
    char kernel_buf[FS_MAX_PATH];
    int result = fs_get_cwd(kernel_buf, sizeof(kernel_buf));
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    size_t len = strlen(kernel_buf);
    if (len >= size) {
        return SYSCALL_ERROR;
    }
    
    strcpy(user_buf, kernel_buf);
    return len;
}

int32_t sys_listdir(uint32_t path_ptr, uint32_t entries_ptr, uint32_t max_entries, uint32_t count_ptr, uint32_t arg5) {
    (void)arg5;
    
    char* user_path = (char*)path_ptr;
    fs_dirent_info_t* user_entries = (fs_dirent_info_t*)entries_ptr;
    uint32_t* user_count = (uint32_t*)count_ptr;
    
    if (!user_path || !user_entries || !user_count) {
        return SYSCALL_ERROR;
    }
    
    char path[FS_MAX_PATH];
    if (strlen(user_path) >= FS_MAX_PATH) {
        return SYSCALL_ERROR;
    }
    
    strcpy(path, user_path);
    
    fs_dirent_info_t kernel_entries[32];
    size_t count;
    
    int result = fs_listdir(path, kernel_entries, 32, &count);
    if (result != FS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    size_t copy_count = (count > max_entries) ? max_entries : count;
    for (size_t i = 0; i < copy_count; i++) {
        user_entries[i] = kernel_entries[i];
    }
    
    *user_count = copy_count;
    return copy_count;
}

// ==================== 内存管理相关系统调用实现 ====================

int32_t sys_malloc(uint32_t size, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    if (size == 0) {
        return SYSCALL_ERROR;
    }
    
    void* ptr = kmalloc(size);
    if (!ptr) {
        return SYSCALL_ERROR;
    }
    
    return (int32_t)(uintptr_t)ptr;
}

int32_t sys_free(uint32_t ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    if (!ptr) {
        return SYSCALL_ERROR;
    }
    
    kfree((void*)(uintptr_t)ptr);
    return SYSCALL_SUCCESS;
}

int32_t sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t fd) {
    (void)addr; (void)length; (void)prot; (void)flags; (void)fd;
    
    // mmap实现需要虚拟内存管理，这里暂时返回错误
    return SYSCALL_ERROR;
}

int32_t sys_munmap(uint32_t addr, uint32_t length, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)addr; (void)length; (void)arg3; (void)arg4; (void)arg5;
    
    // munmap实现需要虚拟内存管理，这里暂时返回错误
    return SYSCALL_ERROR;
}

// ==================== 进程管理相关系统调用实现 ====================

int32_t sys_ps(uint32_t processes_ptr, uint32_t max_count, uint32_t count_ptr, uint32_t arg4, uint32_t arg5) {
    (void)arg4; (void)arg5;
    
    pcb_t* user_processes = (pcb_t*)processes_ptr;
    uint32_t* user_count = (uint32_t*)count_ptr;
    
    if (!user_processes || !user_count) {
        return SYSCALL_ERROR;
    }
    
    pcb_t kernel_processes[MAX_PROCESSES];
    uint32_t count;
    
    int result = process_get_list(kernel_processes, MAX_PROCESSES, &count);
    if (result != PROCESS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    size_t copy_count = (count > max_count) ? max_count : count;
    for (size_t i = 0; i < copy_count; i++) {
        user_processes[i] = kernel_processes[i];
    }
    
    *user_count = copy_count;
    return copy_count;
}

int32_t sys_setpriority(uint32_t pid, uint32_t priority, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg3; (void)arg4; (void)arg5;
    
    if (priority < 1 || priority > 4) {
        return SYSCALL_ERROR;
    }
    
    int result = process_set_priority(pid, (process_priority_t)priority);
    if (result != PROCESS_SUCCESS) {
        return SYSCALL_ERROR;
    }
    
    return SYSCALL_SUCCESS;
}

int32_t sys_getinfo(uint32_t info_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5;
    
    // 返回系统信息，这里简化实现
    if (!info_ptr) {
        return SYSCALL_ERROR;
    }
    
    // 返回当前进程信息
    pcb_t* current = process_get_current();
    if (!current) {
        return SYSCALL_ERROR;
    }
    
    pcb_t* user_info = (pcb_t*)info_ptr;
    *user_info = *current;
    
    return SYSCALL_SUCCESS;
}
