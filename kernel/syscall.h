#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include "../fs/filesystem.h"
#include "process/process.h"

// System call number definitions
#define SYS_EXIT            1
#define SYS_FORK            2
#define SYS_EXEC            3
#define SYS_WAIT            4
#define SYS_GETPID          5
#define SYS_GETPPID         6
#define SYS_KILL            7
#define SYS_YIELD           8

// Filesystem related system calls
#define SYS_OPEN            10
#define SYS_CLOSE           11
#define SYS_READ            12
#define SYS_WRITE           13
#define SYS_SEEK            14
#define SYS_TELL            15
#define SYS_CREATE          16
#define SYS_DELETE          17
#define SYS_RENAME          18
#define SYS_MKDIR           19
#define SYS_RMDIR           20
#define SYS_CHDIR           21
#define SYS_GETCWD          22
#define SYS_LISTDIR         23

// Memory management related system calls
#define SYS_MALLOC          30
#define SYS_FREE            31
#define SYS_MMAP            32
#define SYS_MUNMAP          33

// Process management related system calls
#define SYS_PS              40
#define SYS_SETPRIORITY     41
#define SYS_GETINFO         42

// System call error codes
#define SYSCALL_SUCCESS     0
#define SYSCALL_ERROR       -1
#define SYSCALL_INVALID     -2
#define SYSCALL_NOT_FOUND   -3
#define SYSCALL_ACCESS_DENIED -4
#define SYSCALL_NO_MEMORY   -5

// System call parameter structure
typedef struct {
    uint32_t syscall_num;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
} syscall_args_t;

// System call result structure
typedef struct {
    int32_t result;
    int32_t error_code;
} syscall_result_t;

// System call handler function type
typedef int32_t (*syscall_handler_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

// System call table entry
typedef struct {
    syscall_handler_t handler;
    const char* name;
    const char* description;
} syscall_entry_t;

// Function declarations

// System call initialization
void syscall_init(void);

// System call handler
void syscall_handler(void);

// System call registration
int syscall_register(uint32_t syscall_num, syscall_handler_t handler, const char* name, const char* description);

// System call lookup
syscall_entry_t* syscall_find(uint32_t syscall_num);

// System call execution
int32_t syscall_execute(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

// System call list
void syscall_list(void);

// System call handler functions

// Process related system calls
int32_t sys_exit(uint32_t exit_code, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_fork(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_exec(uint32_t path_ptr, uint32_t argv_ptr, uint32_t envp_ptr, uint32_t arg4, uint32_t arg5);
int32_t sys_wait(uint32_t pid_ptr, uint32_t status_ptr, uint32_t options, uint32_t arg4, uint32_t arg5);
int32_t sys_getpid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_getppid(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_kill(uint32_t pid, uint32_t signal, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_yield(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

// Filesystem related system calls
int32_t sys_open(uint32_t path_ptr, uint32_t flags, uint32_t mode, uint32_t arg4, uint32_t arg5);
int32_t sys_close(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_read(uint32_t fd, uint32_t buf_ptr, uint32_t count, uint32_t arg4, uint32_t arg5);
int32_t sys_write(uint32_t fd, uint32_t buf_ptr, uint32_t count, uint32_t arg4, uint32_t arg5);
int32_t sys_seek(uint32_t fd, uint32_t offset, uint32_t whence, uint32_t arg4, uint32_t arg5);
int32_t sys_tell(uint32_t fd, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_create(uint32_t path_ptr, uint32_t mode, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_delete(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_rename(uint32_t old_path_ptr, uint32_t new_path_ptr, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_mkdir(uint32_t path_ptr, uint32_t mode, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_rmdir(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_chdir(uint32_t path_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_getcwd(uint32_t buf_ptr, uint32_t size, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_listdir(uint32_t path_ptr, uint32_t entries_ptr, uint32_t max_entries, uint32_t count_ptr, uint32_t arg5);

// Memory management related system calls
int32_t sys_malloc(uint32_t size, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_free(uint32_t ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t fd);
int32_t sys_munmap(uint32_t addr, uint32_t length, uint32_t arg3, uint32_t arg4, uint32_t arg5);

// Process management related system calls
int32_t sys_ps(uint32_t processes_ptr, uint32_t max_count, uint32_t count_ptr, uint32_t arg4, uint32_t arg5);
int32_t sys_setpriority(uint32_t pid, uint32_t priority, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int32_t sys_getinfo(uint32_t info_ptr, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

// System call interrupt number
#define SYSCALL_INT_NUM 0x80

// Maximum number of system calls
#define MAX_SYSCALLS 64

#endif // SYSCALL_H
