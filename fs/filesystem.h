#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 文件系统常量
#define FS_SECTOR_SIZE 512
#define FS_ROOT_ENTRIES 224
#define FS_FAT_SIZE 9  // FAT12需要9个扇区
#define FS_ROOT_SECTOR 19
#define FS_DATA_SECTOR 33
#define FS_MAX_FILENAME 11
#define FS_MAX_PATH 256

// 文件属性
#define FS_ATTR_READ_ONLY 0x01
#define FS_ATTR_HIDDEN 0x02
#define FS_ATTR_SYSTEM 0x04
#define FS_ATTR_VOLUME_ID 0x08
#define FS_ATTR_DIRECTORY 0x10
#define FS_ATTR_ARCHIVE 0x20

// 文件打开模式
#define FS_MODE_READ 0x01
#define FS_MODE_WRITE 0x02
#define FS_MODE_APPEND 0x04
#define FS_MODE_CREATE 0x08

// 文件系统错误码
#define FS_SUCCESS 0
#define FS_ERROR_NOT_FOUND -1
#define FS_ERROR_ACCESS_DENIED -2
#define FS_ERROR_DISK_FULL -3
#define FS_ERROR_INVALID_PATH -4
#define FS_ERROR_FILE_EXISTS -5
#define FS_ERROR_INVALID_MODE -6
#define FS_ERROR_IO_ERROR -7
#define FS_ERROR_NOT_DIRECTORY -8

// 文件类型
typedef enum {
    FS_TYPE_FILE = 0,
    FS_TYPE_DIRECTORY,
    FS_TYPE_VOLUME
} fs_file_type_t;

// 文件属性结构
typedef struct {
    uint8_t name[8];        // 文件名（不含扩展名）
    uint8_t ext[3];         // 文件扩展名
    uint8_t attr;           // 文件属性
    uint8_t reserved[10];   // 保留字段
    uint16_t time;          // 时间
    uint16_t date;          // 日期
    uint16_t cluster;       // 起始簇号
    uint32_t size;          // 文件大小
} __attribute__((packed)) fs_dirent_t;

// 文件句柄结构
typedef struct {
    uint32_t cluster;       // 当前簇号
    uint32_t offset;        // 在文件中的偏移
    uint32_t size;          // 文件大小
    uint8_t mode;           // 打开模式
    bool valid;             // 句柄是否有效
    char name[FS_MAX_FILENAME + 1]; // 文件名
} fs_file_t;

// 目录项结构
typedef struct {
    char name[FS_MAX_FILENAME + 1];
    fs_file_type_t type;
    uint32_t size;
    uint32_t cluster;
    uint8_t attr;
} fs_dirent_info_t;

// 文件系统统计信息
typedef struct {
    uint32_t total_sectors;
    uint32_t free_sectors;
    uint32_t used_sectors;
    uint32_t total_files;
    uint32_t total_dirs;
} fs_stats_t;

// 文件系统全局状态
typedef struct {
    bool initialized;
    uint32_t boot_sector;
    uint16_t* fat_table;
    fs_dirent_t* root_dir;
    fs_stats_t stats;
} fs_state_t;

// 函数声明

// 初始化函数
int fs_init(void);
void fs_cleanup(void);

// 文件操作
int fs_open(const char* path, uint8_t mode, fs_file_t* file);
int fs_close(fs_file_t* file);
int fs_read(fs_file_t* file, void* buffer, size_t size);
int fs_write(fs_file_t* file, const void* buffer, size_t size);
int fs_seek(fs_file_t* file, int32_t offset, int whence);
int fs_tell(fs_file_t* file);

// 目录操作
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);
int fs_listdir(const char* path, fs_dirent_info_t* entries, size_t max_entries, size_t* count);
int fs_chdir(const char* path);

// 文件管理
int fs_create(const char* path);
int fs_delete(const char* path);
int fs_rename(const char* old_path, const char* new_path);
int fs_exists(const char* path);
int fs_get_size(const char* path);

// 文件系统信息
int fs_get_stats(fs_stats_t* stats);
int fs_get_free_space(uint32_t* free_bytes);
int fs_format(void);

// 路径操作
int fs_parse_path(const char* path, char* dir, char* filename);
int fs_join_path(const char* dir, const char* filename, char* result);
int fs_get_cwd(char* buffer, size_t size);
int fs_set_cwd(const char* path);

// 内部函数
int fs_read_sector(uint32_t sector, void* buffer);
int fs_write_sector(uint32_t sector, const void* buffer);
int fs_read_fat(uint16_t cluster, uint16_t* value);
int fs_write_fat(uint16_t cluster, uint16_t value);
int fs_find_free_cluster(void);
int fs_allocate_cluster(uint16_t* cluster);
int fs_free_cluster_chain(uint16_t cluster);
int fs_find_file(const char* name, fs_dirent_t* entry);
int fs_add_directory_entry(const char* name, uint8_t attr, uint16_t cluster, uint32_t size);
int fs_remove_directory_entry(const char* name);
int fs_update_directory_entry(const char* name, const fs_dirent_t* entry);
fs_dirent_t* find_entry(const char* path);

// 工具函数
void fs_convert_name(const char* filename, uint8_t* name, uint8_t* ext);
void fs_convert_name_back(const uint8_t* name, const uint8_t* ext, char* filename);
bool fs_is_valid_filename(const char* filename);
int fs_validate_path(const char* path);

#endif // FILESYSTEM_H
