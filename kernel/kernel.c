// kernel.c - Enhanced OS Kernel with integrated shell
#include "../drivers/vga/vga.h"
#include "../lib/string.h"
#include "interrupt.h"
#include "../drivers/keyboard/keyboard.h"
#include "memory.h"
#include "../fs/filesystem.h"
#include "process/process.h"
#include "syscall.h"

// Shell constants
#define MAX_COMMAND_LENGTH 64
#define MAX_ARGS 8

// Command structure
typedef struct {
    char name[16];
    void (*func)(int argc, char* argv[]);
    char description[64];
} command_t;

// Forward declarations
void kmain(void);
void shell_init(void);
void shell_run(void);
void shell_prompt(void);
void shell_execute_command(const char* input);
void shell_read_input(char* buffer, int max_length);
void shell_help(int argc, char* argv[]);
void shell_clear(int argc, char* argv[]);
void shell_info(int argc, char* argv[]);
void shell_memory(int argc, char* argv[]);
void shell_malloc(int argc, char* argv[]);
void shell_free(int argc, char* argv[]);
void shell_paging(int argc, char* argv[]);
void shell_memmap(int argc, char* argv[]);
void shell_ls(int argc, char* argv[]);
void shell_cat(int argc, char* argv[]);
void shell_touch(int argc, char* argv[]);
void shell_rm(int argc, char* argv[]);
void shell_mkdir(int argc, char* argv[]);
void shell_rmdir(int argc, char* argv[]);
void shell_cd(int argc, char* argv[]);
void shell_pwd(int argc, char* argv[]);
void shell_fs_info(int argc, char* argv[]);
void shell_ps(int argc, char* argv[]);
void shell_kill(int argc, char* argv[]);
void shell_priority(int argc, char* argv[]);
void shell_syscall(int argc, char* argv[]);

// Entry point for the kernel
__attribute__((section(".text._start")))
void _start(void) {
    kmain();
}

void kmain(void) {
    // Initialize VGA display
    vga_init();
    
    // Display startup information
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_putstr("=== PXOS Kernel v1.0 ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_putstr("Initializing Kernel...\n");
    
    // Initialize interrupt system
    vga_putstr("Step 1: Initializing interrupts...\n");
    idt_init();
    vga_putstr("Step 1: Interrupts initialized.\n");
    
    // Initialize keyboard
    vga_putstr("Step 2: Initializing keyboard...\n");
    keyboard_init();
    vga_putstr("Step 2: Keyboard initialized.\n");
    
    // Initialize memory management
    vga_putstr("Step 3: Initializing memory...\n");
    memory_init();
    vga_putstr("Step 3: Memory initialized.\n");
    
    // Initialize filesystem
    vga_putstr("Step 4: Initializing filesystem...\n");
    int fs_result = fs_init();
    if (fs_result == FS_SUCCESS) {
        vga_putstr("Step 4: Filesystem initialized successfully\n");
    } else {
        vga_putstr("Step 4: Filesystem initialization failed with code: ");
        vga_puthex(fs_result);
        vga_putstr("\n");
    }
    
    // Initialize process manager
    vga_putstr("Step 5: Initializing process manager...\n");
    int process_result = process_manager_init();
    if (process_result == PROCESS_SUCCESS) {
        vga_putstr("Step 5: Process manager initialized successfully\n");
    } else {
        vga_putstr("Step 5: Process manager initialization failed with code: ");
        vga_puthex(process_result);
        vga_putstr("\n");
    }
    
    // Initialize system calls
    vga_putstr("Step 6: Initializing system calls...\n");
    syscall_init();
    vga_putstr("Step 6: System calls initialized successfully\n");
    
    // Initialize and run Shell
    vga_putstr("Step 7: Initializing shell...\n");
    shell_init();
    vga_putstr("Step 7: Shell initialized.\n");
    
    vga_putstr("Enabling interrupts...\n");
    __asm__ volatile("sti");
    vga_putstr("Interrupts enabled.\n");

    // Run interactive shell
    vga_putstr("Kernel initialized successfully!\n");
    vga_clear();
    shell_run();
}

// ==================== SHELL IMPLEMENTATION ====================

// Command table
static command_t commands[] = {
    {"help", shell_help, "Show all available commands."},
    {"clear", shell_clear, "Clear the screen."},
    {"info", shell_info, "Show System information."},
    {"memory", shell_memory, "Show memory information and statistics."},
    {"malloc", shell_malloc, "Allocate memory (usage: malloc <size>)."},
    {"free", shell_free, "Free allocated memory (usage: free <address>)."},
    {"paging", shell_paging, "Show paging information."},
    {"memmap", shell_memmap, "Show memory map."},
    {"ls", shell_ls, "List directory contents."},
    {"cat", shell_cat, "Display file contents (usage: cat <filename>)."},
    {"touch", shell_touch, "Create empty file (usage: touch <filename>)."},
    {"rm", shell_rm, "Remove file (usage: rm <filename>)."},
    {"mkdir", shell_mkdir, "Create directory (usage: mkdir <dirname>)."},
    {"rmdir", shell_rmdir, "Remove directory (usage: rmdir <dirname>)."},
    {"cd", shell_cd, "Change directory (usage: cd <dirname>)."},
    {"pwd", shell_pwd, "Print current working directory."},
    {"fsinfo", shell_fs_info, "Show filesystem information."},
    {"ps", shell_ps, "List all processes."},
    {"kill", shell_kill, "Kill a process (usage: kill <pid>)."},
    {"priority", shell_priority, "Set process priority (usage: priority <pid> <level>)."},
    {"syscall", shell_syscall, "System call interface (usage: syscall <num> [args...])."},
    {"", NULL, ""} // End marker
};

// Initialize shell
void shell_init(void) {
    vga_putstr("OS Kernel Shell v1.0\n");
    vga_putstr("Type 'help' to see available commands.\n");
    vga_putstr("Keyboard input enabled!\n\n");
    shell_prompt();
}

// Run shell main loop
void shell_run(void) {
    char command[MAX_COMMAND_LENGTH];
    
    // Display initial prompt
    shell_prompt();
    
    while (1) {
        shell_read_input(command, MAX_COMMAND_LENGTH);
        shell_execute_command(command);
    }
}

// Color output functions
void print_success(const char* message) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_putstr(message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void print_error(const char* message) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_putstr(message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void print_warning(const char* message) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_putstr(message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void print_info(const char* message) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_putstr(message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// Display prompt
void shell_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_putstr("PXOS@os:");
    
    // Display current directory
    char cwd[FS_MAX_PATH];
    if (fs_get_cwd(cwd, sizeof(cwd)) == FS_SUCCESS) {
        if (strcmp(cwd, "/") == 0) {
            vga_putstr("~");
        } else {
            vga_putstr(cwd);
        }
    } else {
        vga_putstr("/");
    }
    
    vga_putstr("$ ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);  // User input in white
}

// Read user input
void shell_read_input(char* buffer, int max_length) {
    int pos = 0;
    char ch;
    
    // Clear buffer
    memset(buffer, 0, max_length);
    
    while (pos < max_length - 1) {
        if (keyboard_has_input()) {
            ch = keyboard_get_char();
            
            // Handle enter key
            if (ch == '\r' || ch == '\n') {
                vga_putchar('\n');
                break;
            }
            // Handle backspace key
            else if (ch == '\b' || ch == 127) {
                if (pos > 0) {
                    pos--;
                    buffer[pos] = '\0';
                    vga_putchar('\b');
                }
            }
            // Handle printable characters
            else if (ch >= 32 && ch <= 126) {
                buffer[pos] = ch;
                pos++;
                vga_putchar(ch);
            }
            // Ignore other control characters
        } else {
            __asm__ volatile("hlt");
        }
    }
    
    buffer[pos] = '\0';
}

// Execute command
void shell_execute_command(const char* input) {
    char args[MAX_ARGS][MAX_COMMAND_LENGTH];
    int argc = 0;
    int i = 0;
    
    // 跳过开头的空白字符
    while (input[i] == ' ' || input[i] == '\t') {
        i++;
    }
    
    // 如果输入为空或只有空白字符，直接返回
    if (input[i] == '\0') {
        shell_prompt();
        return;
    }
    
    // 解析命令和参数
    size_t input_len = strlen(input);
    
    while (i < (int)input_len && argc < MAX_ARGS) {
        // 跳过空白字符
        while (i < (int)input_len && (input[i] == ' ' || input[i] == '\t')) {
            i++;
        }
        if (i >= (int)input_len) break;
        
        // 读取一个参数
        int j = 0;
        while (i < (int)input_len && input[i] != ' ' && input[i] != '\t' && j < MAX_COMMAND_LENGTH - 1) {
            args[argc][j++] = input[i++];
        }
        args[argc][j] = '\0';
        argc++;
    }
    
    // 如果没有解析到任何参数
    if (argc == 0) {
        shell_prompt();
        return;
    }
    
    // 查找并执行命令
    for (int cmd_idx = 0; commands[cmd_idx].func != NULL; cmd_idx++) {
        if (strcmp(args[0], commands[cmd_idx].name) == 0) {
            // 创建正确的参数指针数组
            char* argv[MAX_ARGS];
            for (int i = 0; i < argc; i++) {
                argv[i] = args[i];
            }
            commands[cmd_idx].func(argc, argv);
            vga_putchar('\n');
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);  // 系统信息为灰色
            shell_prompt();
            return;
        }
    }
    
    // 命令未找到
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);  // 错误信息为红色
    vga_putstr("Command not found: ");
    vga_putstr(args[0]);
    vga_putchar('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    shell_prompt();
}

// help command
void shell_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_putstr("Available commands:\n");
    for (int i = 0; commands[i].func != NULL; i++) {
        vga_putstr("  ");
        vga_putstr(commands[i].name);
        vga_putstr(" - ");
        vga_putstr(commands[i].description);
        vga_putchar('\n');
    }
}

// clear command
void shell_clear(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_clear();
    // No need to call shell_prompt(), shell_execute_command will call it automatically
}

// info command
void shell_info(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_putstr("=== System Information ===\n");
    vga_putstr("Kernel Version: PXOS Kernel v1.0\n");
    vga_putstr("Architecture: x86-32\n");
    vga_putstr("Memory: Virtual Memory Management Enabled\n");
    vga_putstr("Display: VGA text mode 80x25\n");
    vga_putstr("Shell: Enhanced command line interface\n");
}

// memory command
void shell_memory(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_memory_info();
}

// malloc command
void shell_malloc(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: malloc <size_in_bytes>\n");
        return;
    }
    
    
    // Simple string to number conversion
    uint32_t size = 0;
    char* str = argv[1];
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            size = size * 10 + (*str - '0');
        } else {
            vga_putstr("Invalid size format!\n");
            return;
        }
        str++;
    }
    
    
    if (size == 0) {
        vga_putstr("Size must be greater than 0!\n");
        return;
    }
    
    void* ptr = kmalloc(size);
    if (ptr) {
        vga_putstr("Allocated ");
        vga_puthex(size);
        vga_putstr(" bytes at address: ");
        vga_puthex((uint32_t)ptr);
        vga_putstr("\n");
    } else {
        vga_putstr("Memory allocation failed!\n");
    }
}

// free command
void shell_free(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: free <address>\n");
        return;
    }
    
    // Simple hex string to number conversion
    uint32_t addr = 0;
    char* str = argv[1];
    
    // Skip 0x prefix
    if (str[0] == '0' && str[1] == 'x') {
        str += 2;
    }
    
    while (*str) {
        addr <<= 4;
        if (*str >= '0' && *str <= '9') {
            addr += (*str - '0');
        } else if (*str >= 'a' && *str <= 'f') {
            addr += (*str - 'a' + 10);
        } else if (*str >= 'A' && *str <= 'F') {
            addr += (*str - 'A' + 10);
        } else {
            vga_putstr("Invalid address format!\n");
            return;
        }
        str++;
    }
    
    kfree((void*)addr);
    vga_putstr("Freed memory at address: 0x");
    vga_puthex(addr);
    vga_putstr("\n");
}

// paging command
void shell_paging(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    vga_putstr("=== Paging Information ===\n");
    vga_putstr("Page Size: 4KB\n");
    vga_putstr("Page Directory Entries: 1024\n");
    vga_putstr("Page Table Entries: 1024\n");
    vga_putstr("Paging system initialized.\n");
    // Temporarily not displaying page directory info to avoid calling potentially problematic functions
}

// memmap command
void shell_memmap(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_memory_map();
}

// ls command
void shell_ls(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    fs_dirent_info_t entries[32];
    size_t count;
    
    // 获取当前目录
    char current_path[FS_MAX_PATH];
    fs_get_cwd(current_path, sizeof(current_path));
    
    int result = fs_listdir(current_path, entries, 32, &count);
    if (result != FS_SUCCESS) {
        print_error("Failed to list directory\n");
        return;
    }
    
    print_info("Directory listing:\n");
    print_info("Name\t\tType\t\tSize\n");
    print_info("----------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        vga_putstr(entries[i].name);
        vga_putstr("\t\t");
        
        if (entries[i].type == FS_TYPE_DIRECTORY) {
            vga_putstr("DIR\t\t");
        } else {
            vga_putstr("FILE\t\t");
        }
        
        vga_puthex(entries[i].size);
        vga_putstr("\n");
    }
}

// cat command
void shell_cat(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: cat <filename>\n");
        return;
    }
    
    // 构建完整路径
    char full_path[FS_MAX_PATH];
    strcpy(full_path, "/");
    strcat(full_path, argv[1]);
    
    fs_file_t file;
    int result = fs_open(full_path, FS_MODE_READ, &file);
    if (result != FS_SUCCESS) {
        print_error("Failed to open file\n");
        return;
    }
    
    char buffer[256];
    int bytes_read;
    while ((bytes_read = fs_read(&file, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        vga_putstr(buffer);
    }
    
    fs_close(&file);
}

// touch command
void shell_touch(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: touch <filename>\n");
        return;
    }
    
    // 构建完整路径
    char full_path[FS_MAX_PATH];
    strcpy(full_path, "/");
    strcat(full_path, argv[1]);
    
    int result = fs_create(full_path);
    
    if (result == FS_SUCCESS) {
        print_success("File created successfully\n");
    } else {
        print_error("Failed to create file\n");
    }
}

// rm command
void shell_rm(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: rm <filename>\n");
        return;
    }
    
    // 构建完整路径
    char full_path[FS_MAX_PATH];
    strcpy(full_path, "/");
    strcat(full_path, argv[1]);
    
    int result = fs_delete(full_path);
    if (result == FS_SUCCESS) {
        print_success("File deleted successfully\n");
    } else {
        print_error("Failed to delete file\n");
    }
}

// mkdir command
void shell_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: mkdir <dirname>\n");
        return;
    }
    
    // 构建完整路径
    char full_path[FS_MAX_PATH];
    strcpy(full_path, "/");
    strcat(full_path, argv[1]);
    
    int result = fs_mkdir(full_path);
    if (result == FS_SUCCESS) {
        print_success("Directory created successfully\n");
    } else {
        print_error("Failed to create directory\n");
    }
}

// rmdir command
void shell_rmdir(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: rmdir <dirname>\n");
        return;
    }
    
    // 构建完整路径
    char full_path[FS_MAX_PATH];
    strcpy(full_path, "/");
    strcat(full_path, argv[1]);
    
    int result = fs_rmdir(full_path);
    if (result == FS_SUCCESS) {
        print_success("Directory removed successfully\n");
    } else {
        print_error("Failed to remove directory\n");
    }
}

// cd command
void shell_cd(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: cd <dirname>\n");
        return;
    }
    
    // 特殊处理 cd .. 在根目录的情况
    if (strcmp(argv[1], "..") == 0) {
        char cwd[FS_MAX_PATH];
        if (fs_get_cwd(cwd, sizeof(cwd)) == FS_SUCCESS && strcmp(cwd, "/") == 0) {
            print_warning("Already at root directory.\n");
            return;
        }
    }
    
    char full_path[FS_MAX_PATH];
    
    // 特殊处理 .. 和 . 以及根目录
    if (strcmp(argv[1], "..") == 0 || strcmp(argv[1], ".") == 0 || strcmp(argv[1], "/") == 0) {
        strcpy(full_path, argv[1]);
    } else {
        // 构建完整路径
        strcpy(full_path, "/");
        strcat(full_path, argv[1]);
    }
    
    int result = fs_chdir(full_path);
    if (result == FS_SUCCESS) {
        print_success("Changed directory successfully\n");
    } else {
        print_error("Failed to change directory\n");
    }
}

// pwd command
void shell_pwd(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    char cwd[FS_MAX_PATH];
    int result = fs_get_cwd(cwd, sizeof(cwd));
    if (result == FS_SUCCESS) {
        vga_putstr("Current directory: ");
        if (strcmp(cwd, "/") == 0) {
            vga_putstr("~");
        } else {
            vga_putstr(cwd);
        }
        vga_putstr("\n");
    } else {
        vga_putstr("Failed to get current directory\n");
    }
}

// fsinfo command
void shell_fs_info(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    fs_stats_t stats;
    int result = fs_get_stats(&stats);
    if (result != FS_SUCCESS) {
        vga_putstr("Failed to get filesystem information\n");
        return;
    }
    
    vga_putstr("=== Filesystem Information ===\n");
    vga_putstr("Total Sectors: ");
    vga_puthex(stats.total_sectors);
    vga_putstr("\n");
    
    vga_putstr("Free Sectors: ");
    vga_puthex(stats.free_sectors);
    vga_putstr("\n");
    
    vga_putstr("Used Sectors: ");
    vga_puthex(stats.used_sectors);
    vga_putstr("\n");
    
    vga_putstr("Total Files: ");
    vga_puthex(stats.total_files);
    vga_putstr("\n");
    
    vga_putstr("Total Directories: ");
    vga_puthex(stats.total_dirs);
    vga_putstr("\n");
    
    uint32_t free_bytes;
    if (fs_get_free_space(&free_bytes) == FS_SUCCESS) {
        vga_putstr("Free Space: ");
        vga_puthex(free_bytes);
        vga_putstr(" bytes\n");
    }
}

// ps command - list all processes
void shell_ps(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    pcb_t processes[MAX_PROCESSES];
    uint32_t count;
    
    if (process_get_list(processes, MAX_PROCESSES, &count) != PROCESS_SUCCESS) {
        print_error("Failed to get process list\n");
        return;
    }
    
    print_info("Process List:\n");
    vga_putstr("PID | Name                | State     | Priority  | CPU Time\n");
    vga_putstr("----|---------------------|-----------|-----------|---------\n");
    
    for (uint32_t i = 0; i < count; i++) {
        vga_putstr(" ");
        vga_puthex(processes[i].pid);
        vga_putstr("  | ");
        
        // 打印进程名（左对齐，20字符宽度）
        char name[21];
        strncpy(name, processes[i].name, 20);
        name[20] = '\0';
        vga_putstr(name);
        for (int j = strlen(name); j < 20; j++) {
            vga_putstr(" ");
        }
        vga_putstr(" | ");
        
        // 打印状态（左对齐，9字符宽度）
        const char* state = process_state_to_string(processes[i].state);
        vga_putstr(state);
        for (int j = strlen(state); j < 9; j++) {
            vga_putstr(" ");
        }
        vga_putstr(" | ");
        
        // 打印优先级（左对齐，9字符宽度）
        const char* priority = process_priority_to_string(processes[i].priority);
        vga_putstr(priority);
        for (int j = strlen(priority); j < 9; j++) {
            vga_putstr(" ");
        }
        vga_putstr(" | ");
        
        // 打印CPU时间
        vga_puthex(processes[i].cpu_time);
        vga_putstr("\n");
    }
    
    vga_putstr("Total processes: ");
    vga_puthex(count);
    vga_putstr("\n");
}

// kill command - kill process
void shell_kill(int argc, char* argv[]) {
    if (argc < 2) {
        print_error("Usage: kill <pid>\n");
        return;
    }
    
    uint32_t pid = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            pid = pid * 10 + (argv[1][i] - '0');
        } else {
            print_error("Invalid PID format\n");
            return;
        }
    }
    
    if (pid == 0) {
        print_error("Cannot kill idle process (PID 0)\n");
        return;
    }
    
    int result = process_kill(pid);
    if (result == PROCESS_SUCCESS) {
        print_success("Process killed successfully\n");
    } else if (result == PROCESS_ERROR_NOT_FOUND) {
        print_error("Process not found\n");
    } else {
        print_error("Failed to kill process\n");
    }
}

// priority command - set process priority
void shell_priority(int argc, char* argv[]) {
    if (argc < 3) {
        print_error("Usage: priority <pid> <level>\n");
        print_info("Levels: 1=Low, 2=Normal, 3=High, 4=Critical\n");
        return;
    }
    
    // 解析PID
    uint32_t pid = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            pid = pid * 10 + (argv[1][i] - '0');
        } else {
            print_error("Invalid PID format\n");
            return;
        }
    }
    
    // 解析优先级
    uint32_t priority_level = 0;
    for (int i = 0; argv[2][i] != '\0'; i++) {
        if (argv[2][i] >= '0' && argv[2][i] <= '9') {
            priority_level = priority_level * 10 + (argv[2][i] - '0');
        } else {
            print_error("Invalid priority level format\n");
            return;
        }
    }
    
    if (priority_level < 1 || priority_level > 4) {
        print_error("Priority level must be 1-4\n");
        return;
    }
    
    process_priority_t priority = (process_priority_t)priority_level;
    int result = process_set_priority(pid, priority);
    
    if (result == PROCESS_SUCCESS) {
        print_success("Process priority updated successfully\n");
    } else if (result == PROCESS_ERROR_NOT_FOUND) {
        print_error("Process not found\n");
    } else {
        print_error("Failed to update process priority\n");
    }
}

// syscall command - system call interface
void shell_syscall(int argc, char* argv[]) {
    if (argc < 2) {
        vga_putstr("Usage: syscall <num> [arg1] [arg2] [arg3] [arg4] [arg5]\n");
        vga_putstr("Use 'syscall list' to see available system calls.\n");
        return;
    }
    
    if (strcmp(argv[1], "list") == 0) {
        syscall_list();
        return;
    }
    
    // 解析系统调用号
    uint32_t syscall_num = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            syscall_num = syscall_num * 10 + (argv[1][i] - '0');
        } else {
            print_error("Invalid system call number format\n");
            return;
        }
    }
    
    // 解析参数
    uint32_t args[5] = {0, 0, 0, 0, 0};
    for (int i = 2; i < argc && i < 7; i++) {
        // 简单的十六进制解析
        if (argv[i][0] == '0' && argv[i][1] == 'x') {
            // 十六进制
            char* str = argv[i] + 2;
            while (*str) {
                args[i-2] <<= 4;
                if (*str >= '0' && *str <= '9') {
                    args[i-2] += (*str - '0');
                } else if (*str >= 'a' && *str <= 'f') {
                    args[i-2] += (*str - 'a' + 10);
                } else if (*str >= 'A' && *str <= 'F') {
                    args[i-2] += (*str - 'A' + 10);
                } else {
                    print_error("Invalid hex format\n");
                    return;
                }
                str++;
            }
        } else {
            // 十进制
            for (int j = 0; argv[i][j] != '\0'; j++) {
                if (argv[i][j] >= '0' && argv[i][j] <= '9') {
                    args[i-2] = args[i-2] * 10 + (argv[i][j] - '0');
                } else {
                    print_error("Invalid decimal format\n");
                    return;
                }
            }
        }
    }
    
    // 执行系统调用
    int32_t result = syscall_execute(syscall_num, args[0], args[1], args[2], args[3], args[4]);
    
    vga_putstr("System call result: ");
    vga_puthex(result);
    vga_putstr("\n");
}



