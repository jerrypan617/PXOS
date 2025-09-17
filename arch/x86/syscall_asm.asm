; syscall_asm.asm - System call assembly interface
[bits 32]

; 系统调用入口点
; 用户程序通过 int 0x80 调用系统调用
; 参数通过寄存器传递：
; eax = 系统调用号
; ebx = 参数1
; ecx = 参数2
; edx = 参数3
; esi = 参数4
; edi = 参数5
; 返回值通过 eax 返回

global syscall_entry
extern syscall_execute

syscall_entry:
    ; 保存所有寄存器
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    
    ; 调用C语言系统调用处理函数
    ; 参数通过寄存器传递：eax=syscall_num, ebx=arg1, ecx=arg2, edx=arg3, esi=arg4, edi=arg5
    push edi    ; arg5
    push esi    ; arg4
    push edx    ; arg3
    push ecx    ; arg2
    push ebx    ; arg1
    push eax    ; syscall_num
    call syscall_execute
    add esp, 24 ; 清理栈（6个参数 * 4字节）
    
    ; 恢复所有寄存器
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    
    ; 返回用户态
    iret

; 系统调用包装函数（供C代码调用）
; 这些函数提供了从内核代码调用系统调用的接口

global syscall_exit
global syscall_fork
global syscall_exec
global syscall_wait
global syscall_getpid
global syscall_getppid
global syscall_kill
global syscall_yield

global syscall_open
global syscall_close
global syscall_read
global syscall_write
global syscall_seek
global syscall_tell
global syscall_create
global syscall_delete
global syscall_rename
global syscall_mkdir
global syscall_rmdir
global syscall_chdir
global syscall_getcwd
global syscall_listdir

global syscall_malloc
global syscall_free
global syscall_mmap
global syscall_munmap

global syscall_ps
global syscall_setpriority
global syscall_getinfo

; 进程相关系统调用包装函数
syscall_exit:
    mov eax, 1      ; SYS_EXIT
    int 0x80
    ret

syscall_fork:
    mov eax, 2      ; SYS_FORK
    int 0x80
    ret

syscall_exec:
    mov eax, 3      ; SYS_EXEC
    int 0x80
    ret

syscall_wait:
    mov eax, 4      ; SYS_WAIT
    int 0x80
    ret

syscall_getpid:
    mov eax, 5      ; SYS_GETPID
    int 0x80
    ret

syscall_getppid:
    mov eax, 6      ; SYS_GETPPID
    int 0x80
    ret

syscall_kill:
    mov eax, 7      ; SYS_KILL
    int 0x80
    ret

syscall_yield:
    mov eax, 8      ; SYS_YIELD
    int 0x80
    ret

; 文件系统相关系统调用包装函数
syscall_open:
    mov eax, 10     ; SYS_OPEN
    int 0x80
    ret

syscall_close:
    mov eax, 11     ; SYS_CLOSE
    int 0x80
    ret

syscall_read:
    mov eax, 12     ; SYS_READ
    int 0x80
    ret

syscall_write:
    mov eax, 13     ; SYS_WRITE
    int 0x80
    ret

syscall_seek:
    mov eax, 14     ; SYS_SEEK
    int 0x80
    ret

syscall_tell:
    mov eax, 15     ; SYS_TELL
    int 0x80
    ret

syscall_create:
    mov eax, 16     ; SYS_CREATE
    int 0x80
    ret

syscall_delete:
    mov eax, 17     ; SYS_DELETE
    int 0x80
    ret

syscall_rename:
    mov eax, 18     ; SYS_RENAME
    int 0x80
    ret

syscall_mkdir:
    mov eax, 19     ; SYS_MKDIR
    int 0x80
    ret

syscall_rmdir:
    mov eax, 20     ; SYS_RMDIR
    int 0x80
    ret

syscall_chdir:
    mov eax, 21     ; SYS_CHDIR
    int 0x80
    ret

syscall_getcwd:
    mov eax, 22     ; SYS_GETCWD
    int 0x80
    ret

syscall_listdir:
    mov eax, 23     ; SYS_LISTDIR
    int 0x80
    ret

; 内存管理相关系统调用包装函数
syscall_malloc:
    mov eax, 30     ; SYS_MALLOC
    int 0x80
    ret

syscall_free:
    mov eax, 31     ; SYS_FREE
    int 0x80
    ret

syscall_mmap:
    mov eax, 32     ; SYS_MMAP
    int 0x80
    ret

syscall_munmap:
    mov eax, 33     ; SYS_MUNMAP
    int 0x80
    ret

; 进程管理相关系统调用包装函数
syscall_ps:
    mov eax, 40     ; SYS_PS
    int 0x80
    ret

syscall_setpriority:
    mov eax, 41     ; SYS_SETPRIORITY
    int 0x80
    ret

syscall_getinfo:
    mov eax, 42     ; SYS_GETINFO
    int 0x80
    ret
