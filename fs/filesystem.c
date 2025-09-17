#include "filesystem.h"
#include "../drivers/vga/vga.h"
#include "../lib/string.h"
#include "../kernel/memory.h"

// 全局文件系统状态
static fs_state_t fs_state = {0};
static char current_directory[FS_MAX_PATH] = "/";

// 内部函数声明
static int write_fat_sector(uint32_t sector, const void* buffer);
static int find_directory_entry(const char* name, fs_dirent_t* entry);
static int add_directory_entry(const char* name, uint8_t attr, uint16_t cluster, uint32_t size);
static int add_directory_entry_to_dir(const char* name, uint8_t attr, uint16_t cluster, uint32_t size, const char* target_dir);
static int remove_directory_entry(const char* name);
static int update_directory_entry(const char* name, const fs_dirent_t* entry);
static void convert_filename(const char* filename, uint8_t* name, uint8_t* ext);
static void convert_filename_back(const uint8_t* name, const uint8_t* ext, char* filename);
static bool is_valid_filename(const char* filename);
static int validate_path(const char* path);
static int parse_path(const char* path, char* dir, char* filename);
static int join_path(const char* dir, const char* filename, char* result);

// 初始化文件系统
int fs_init(void) {
    vga_putstr("Initializing filesystem...\n");
    
    // 清零状态
    memset(&fs_state, 0, sizeof(fs_state_t));
    
    // 使用静态内存分配，避免动态分配问题
    static uint16_t fat_table_static[FS_FAT_SIZE * FS_SECTOR_SIZE / sizeof(uint16_t)];
    static fs_dirent_t root_dir_static[FS_ROOT_ENTRIES];
    
    fs_state.fat_table = fat_table_static;
    fs_state.root_dir = root_dir_static;
    
    // 读取引导扇区
    uint8_t boot_sector[FS_SECTOR_SIZE];
    memset(boot_sector, 0, FS_SECTOR_SIZE); // 初始化为0
    
    if (fs_read_sector(0, boot_sector) != FS_SUCCESS) {
        vga_putstr("Failed to read boot sector\n");
        return FS_ERROR_IO_ERROR;
    }
    
    // 验证FAT12文件系统（简化验证，因为当前是模拟实现）
    // 在实际实现中，这里应该检查引导扇区的FAT12标识
    vga_putstr("Filesystem validation skipped (simulation mode)\n");
    
    // 初始化FAT表（模拟实现）
    memset(fs_state.fat_table, 0, FS_FAT_SIZE * FS_SECTOR_SIZE);
    // 标记保留簇为已使用（簇0和簇1是保留的）
    fs_write_fat(0, 0xFF8);  // 簇0：保留
    fs_write_fat(1, 0xFFF);  // 簇1：保留
    vga_putstr("FAT table initialized (simulation mode)\n");
    
    // 初始化根目录（模拟实现）
    memset(fs_state.root_dir, 0, FS_ROOT_ENTRIES * sizeof(fs_dirent_t));
    vga_putstr("Root directory initialized (simulation mode)\n");
    
    // 计算统计信息
    fs_state.stats.total_sectors = 2880; // 1.44MB软盘
    fs_state.stats.free_sectors = 0;
    fs_state.stats.used_sectors = 0;
    fs_state.stats.total_files = 0;
    fs_state.stats.total_dirs = 0;
    
    // 计算空闲扇区数
    for (int i = 2; i < 2880; i++) {
        uint16_t cluster = fs_read_fat(i, NULL);
        if (cluster == 0) {
            fs_state.stats.free_sectors++;
        } else {
            fs_state.stats.used_sectors++;
        }
    }
    
    // 计算文件和目录数
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
            if (fs_state.root_dir[i].attr & FS_ATTR_DIRECTORY) {
                fs_state.stats.total_dirs++;
            } else {
                fs_state.stats.total_files++;
            }
        }
    }
    
    fs_state.initialized = true;
    vga_putstr("Filesystem initialized successfully\n");
    return FS_SUCCESS;
}

// 清理文件系统
void fs_cleanup(void) {
    // 使用静态内存，不需要释放
    fs_state.fat_table = NULL;
    fs_state.root_dir = NULL;
    fs_state.initialized = false;
}

// 打开文件
int fs_open(const char* path, uint8_t mode, fs_file_t* file) {
    if (!fs_state.initialized || !file) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 验证路径
    if (validate_path(path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 查找文件
    fs_dirent_t entry;
    if (find_directory_entry(path, &entry) != FS_SUCCESS) {
        if (!(mode & FS_MODE_CREATE)) {
            vga_putstr("fs_open: file not found and not creating\n");
            return FS_ERROR_NOT_FOUND;
        }
        // 创建新文件
        uint16_t cluster;
        if (fs_allocate_cluster(&cluster) != FS_SUCCESS) {
            return FS_ERROR_DISK_FULL;
        }
        if (add_directory_entry_to_dir(path, FS_ATTR_ARCHIVE, cluster, 0, current_directory) != FS_SUCCESS) {
            fs_free_cluster_chain(cluster);
            return FS_ERROR_IO_ERROR;
        }
        if (find_directory_entry(path, &entry) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
    }
    
    // 检查权限
    if ((mode & FS_MODE_WRITE) && (entry.attr & FS_ATTR_READ_ONLY)) {
        return FS_ERROR_ACCESS_DENIED;
    }
    
    // 初始化文件句柄
    memset(file, 0, sizeof(fs_file_t));
    file->cluster = entry.cluster;
    file->size = entry.size;
    file->mode = mode;
    file->valid = true;
    strcpy(file->name, path);
    
    return FS_SUCCESS;
}

// 关闭文件
int fs_close(fs_file_t* file) {
    if (!file || !file->valid) {
        return FS_ERROR_IO_ERROR;
    }
    
    file->valid = false;
    return FS_SUCCESS;
}

// 读取文件
int fs_read(fs_file_t* file, void* buffer, size_t size) {
    if (!file || !file->valid || !(file->mode & FS_MODE_READ)) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (file->offset >= file->size) {
        return 0; // EOF
    }
    
    size_t bytes_to_read = size;
    if (file->offset + bytes_to_read > file->size) {
        bytes_to_read = file->size - file->offset;
    }
    
    uint8_t* buf = (uint8_t*)buffer;
    size_t bytes_read = 0;
    uint32_t current_cluster = file->cluster;
    uint32_t sector_offset = file->offset / FS_SECTOR_SIZE;
    
    // 计算起始扇区
    uint32_t current_sector = FS_DATA_SECTOR + (current_cluster - 2) * 2 + sector_offset;
    
    // 读取数据
    while (bytes_read < bytes_to_read) {
        uint8_t sector_data[FS_SECTOR_SIZE];
        if (fs_read_sector(current_sector, sector_data) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        size_t sector_bytes = FS_SECTOR_SIZE - (file->offset % FS_SECTOR_SIZE);
        if (sector_bytes > bytes_to_read - bytes_read) {
            sector_bytes = bytes_to_read - bytes_read;
        }
        
        memcpy(buf + bytes_read, sector_data + (file->offset % FS_SECTOR_SIZE), sector_bytes);
        bytes_read += sector_bytes;
        file->offset += sector_bytes;
        
        // 移动到下一个扇区
        if (file->offset % FS_SECTOR_SIZE == 0) {
            current_sector++;
            if (current_sector % 2 == 0) {
                // 移动到下一个簇
                uint16_t next_cluster;
                if (fs_read_fat(current_cluster, &next_cluster) != FS_SUCCESS) {
                    return FS_ERROR_IO_ERROR;
                }
                if (next_cluster >= 0xFF8) { // 文件结束
                    break;
                }
                current_cluster = next_cluster;
                current_sector = FS_DATA_SECTOR + (current_cluster - 2) * 2;
            }
        }
    }
    
    return bytes_read;
}

// 写入文件
int fs_write(fs_file_t* file, const void* buffer, size_t size) {
    if (!file || !file->valid || !(file->mode & FS_MODE_WRITE)) {
        return FS_ERROR_IO_ERROR;
    }
    
    const uint8_t* buf = (const uint8_t*)buffer;
    size_t bytes_written = 0;
    uint32_t current_cluster = file->cluster;
    uint32_t sector_offset = file->offset / FS_SECTOR_SIZE;
    
    // 计算起始扇区
    uint32_t current_sector = FS_DATA_SECTOR + (current_cluster - 2) * 2 + sector_offset;
    
    // 写入数据
    while (bytes_written < size) {
        uint8_t sector_data[FS_SECTOR_SIZE];
        
        // 读取现有扇区数据
        if (fs_read_sector(current_sector, sector_data) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        size_t sector_bytes = FS_SECTOR_SIZE - (file->offset % FS_SECTOR_SIZE);
        if (sector_bytes > size - bytes_written) {
            sector_bytes = size - bytes_written;
        }
        
        memcpy(sector_data + (file->offset % FS_SECTOR_SIZE), buf + bytes_written, sector_bytes);
        
        // 写回扇区
        if (fs_write_sector(current_sector, sector_data) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        bytes_written += sector_bytes;
        file->offset += sector_bytes;
        
        // 更新文件大小
        if (file->offset > file->size) {
            file->size = file->offset;
        }
        
        // 移动到下一个扇区
        if (file->offset % FS_SECTOR_SIZE == 0) {
            current_sector++;
            if (current_sector % 2 == 0) {
                // 检查是否需要分配新簇
                uint16_t next_cluster;
                if (fs_read_fat(current_cluster, &next_cluster) != FS_SUCCESS) {
                    return FS_ERROR_IO_ERROR;
                }
                if (next_cluster >= 0xFF8) { // 需要新簇
                    uint16_t new_cluster;
                    if (fs_allocate_cluster(&new_cluster) != FS_SUCCESS) {
                        return FS_ERROR_DISK_FULL;
                    }
                    if (fs_write_fat(current_cluster, new_cluster) != FS_SUCCESS) {
                        fs_free_cluster_chain(new_cluster);
                        return FS_ERROR_IO_ERROR;
                    }
                    current_cluster = new_cluster;
                } else {
                    current_cluster = next_cluster;
                }
                current_sector = FS_DATA_SECTOR + (current_cluster - 2) * 2;
            }
        }
    }
    
    return bytes_written;
}

// 文件定位
int fs_seek(fs_file_t* file, int32_t offset, int whence) {
    if (!file || !file->valid) {
        return FS_ERROR_IO_ERROR;
    }
    
    uint32_t new_offset;
    switch (whence) {
        case 0: // SEEK_SET
            new_offset = offset;
            break;
        case 1: // SEEK_CUR
            new_offset = file->offset + offset;
            break;
        case 2: // SEEK_END
            new_offset = file->size + offset;
            break;
        default:
            return FS_ERROR_IO_ERROR;
    }
    
    if (new_offset > file->size) {
        return FS_ERROR_IO_ERROR;
    }
    
    file->offset = new_offset;
    return FS_SUCCESS;
}

// 获取文件位置
int fs_tell(fs_file_t* file) {
    if (!file || !file->valid) {
        return -1;
    }
    return file->offset;
}

// 创建目录
int fs_mkdir(const char* path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (validate_path(path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 检查目录是否已存在
    fs_dirent_t entry;
    if (find_directory_entry(path, &entry) == FS_SUCCESS) {
        return FS_ERROR_FILE_EXISTS;
    }
    
    // 分配新簇
    uint16_t cluster;
    if (fs_allocate_cluster(&cluster) != FS_SUCCESS) {
        return FS_ERROR_DISK_FULL;
    }
    
    // 添加目录项
    if (add_directory_entry(path, FS_ATTR_DIRECTORY, cluster, 0) != FS_SUCCESS) {
        fs_free_cluster_chain(cluster);
        return FS_ERROR_IO_ERROR;
    }
    
    // 初始化目录簇（写入两个空目录项：. 和 ..）
    uint8_t sector_data[FS_SECTOR_SIZE];
    memset(sector_data, 0, FS_SECTOR_SIZE);
    
    // 创建 . 目录项
    fs_dirent_t* dot_entry = (fs_dirent_t*)sector_data;
    memcpy(dot_entry->name, ".          ", 11);
    dot_entry->attr = FS_ATTR_DIRECTORY;
    dot_entry->cluster = cluster;
    
    // 创建 .. 目录项
    fs_dirent_t* dotdot_entry = (fs_dirent_t*)(sector_data + 32);
    memcpy(dotdot_entry->name, "..         ", 11);
    dotdot_entry->attr = FS_ATTR_DIRECTORY;
    dotdot_entry->cluster = 0; // 根目录
    
    // 写入目录簇
    uint32_t sector = FS_DATA_SECTOR + (cluster - 2) * 2;
    if (fs_write_sector(sector, sector_data) != FS_SUCCESS) {
        fs_free_cluster_chain(cluster);
        return FS_ERROR_IO_ERROR;
    }
    
    return FS_SUCCESS;
}

// 删除目录
int fs_rmdir(const char* path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (validate_path(path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 查找目录
    fs_dirent_t entry;
    if (find_directory_entry(path, &entry) != FS_SUCCESS) {
        return FS_ERROR_NOT_FOUND;
    }
    
    if (!(entry.attr & FS_ATTR_DIRECTORY)) {
        return FS_ERROR_ACCESS_DENIED;
    }
    
    // 检查目录是否为空
    // 这里简化处理，实际应该检查目录内容
    
    // 释放簇链
    if (entry.cluster != 0) {
        fs_free_cluster_chain(entry.cluster);
    }
    
    // 删除目录项
    if (remove_directory_entry(path) != FS_SUCCESS) {
        return FS_ERROR_IO_ERROR;
    }
    
    return FS_SUCCESS;
}

// 列出目录内容
int fs_listdir(const char* path, fs_dirent_info_t* entries, size_t max_entries, size_t* count) {
    if (!fs_state.initialized || !entries || !count) {
        return FS_ERROR_IO_ERROR;
    }
    
    *count = 0;
    
    // 如果路径为空或为根目录，列出根目录
    if (!path || strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
        // 列出根目录
        for (int i = 0; i < FS_ROOT_ENTRIES && *count < max_entries; i++) {
            if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
                fs_dirent_info_t* entry = &entries[*count];
                
                // 转换文件名
                convert_filename_back(fs_state.root_dir[i].name, fs_state.root_dir[i].ext, entry->name);
                
                // 设置类型
                if (fs_state.root_dir[i].attr & FS_ATTR_DIRECTORY) {
                    entry->type = FS_TYPE_DIRECTORY;
                } else {
                    entry->type = FS_TYPE_FILE;
                }
                
                entry->size = fs_state.root_dir[i].size;
                entry->cluster = fs_state.root_dir[i].cluster;
                entry->attr = fs_state.root_dir[i].attr;
                
                (*count)++;
            }
        }
    } else {
        // 查找目录
        fs_dirent_t dir_entry;
        if (find_directory_entry(path, &dir_entry) != FS_SUCCESS) {
            return FS_ERROR_NOT_FOUND;
        }
        
        if (!(dir_entry.attr & FS_ATTR_DIRECTORY)) {
            return FS_ERROR_ACCESS_DENIED;
        }
        
        // 读取目录簇内容
        if (dir_entry.cluster == 0) {
            // 根目录已经在上面处理了
            *count = 0;
            return FS_SUCCESS;
        }
        
        // 读取目录簇
        uint8_t sector_data[FS_SECTOR_SIZE];
        uint32_t sector = FS_DATA_SECTOR + (dir_entry.cluster - 2) * 2;
        
        if (fs_read_sector(sector, sector_data) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        // 解析目录项（跳过 . 和 .. 项）
        fs_dirent_t* dir_entries = (fs_dirent_t*)sector_data;
        for (size_t i = 2; i < FS_SECTOR_SIZE / sizeof(fs_dirent_t) && *count < max_entries; i++) {
            if (dir_entries[i].name[0] != 0 && dir_entries[i].name[0] != 0xE5) {
                fs_dirent_info_t* entry = &entries[*count];
                
                // 转换文件名
                convert_filename_back(dir_entries[i].name, dir_entries[i].ext, entry->name);
                
                // 设置类型
                if (dir_entries[i].attr & FS_ATTR_DIRECTORY) {
                    entry->type = FS_TYPE_DIRECTORY;
                } else {
                    entry->type = FS_TYPE_FILE;
                }
                
                entry->size = dir_entries[i].size;
                entry->cluster = dir_entries[i].cluster;
                entry->attr = dir_entries[i].attr;
                
                (*count)++;
            }
        }
    }
    
    return FS_SUCCESS;
}

// 改变当前目录
int fs_chdir(const char* path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (validate_path(path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 支持根目录
    if (strcmp(path, "/") == 0) {
        strcpy(current_directory, "/");
        return FS_SUCCESS;
    }
    
    // 支持 .. 操作
    if (strcmp(path, "..") == 0) {
        // 如果当前在根目录，.. 操作无效
        if (strcmp(current_directory, "/") == 0) {
            return FS_ERROR_ACCESS_DENIED;
        }
        
        // 简化实现：总是回到根目录
        // 在实际实现中，这里应该解析路径并回到父目录
        strcpy(current_directory, "/");
        return FS_SUCCESS;
    }
    
    // 检查目录是否存在
    fs_dirent_t* entry = find_entry(path);
    if (entry == NULL) {
        return FS_ERROR_NOT_FOUND;
    }
    
    // 检查是否为目录
    if (!(entry->attr & FS_ATTR_DIRECTORY)) {
        return FS_ERROR_NOT_DIRECTORY;
    }
    
    // 更新当前目录
    strcpy(current_directory, path);
    return FS_SUCCESS;
}

// 创建文件
int fs_create(const char* path) {
    fs_file_t file;
    int result = fs_open(path, FS_MODE_CREATE | FS_MODE_WRITE, &file);
    if (result == FS_SUCCESS) {
        fs_close(&file);
    }
    return result;
}

// 删除文件
int fs_delete(const char* path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (validate_path(path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 查找文件
    fs_dirent_t entry;
    if (find_directory_entry(path, &entry) != FS_SUCCESS) {
        return FS_ERROR_NOT_FOUND;
    }
    
    if (entry.attr & FS_ATTR_DIRECTORY) {
        return FS_ERROR_ACCESS_DENIED;
    }
    
    // 释放簇链
    if (entry.cluster != 0) {
        fs_free_cluster_chain(entry.cluster);
    }
    
    // 删除目录项
    if (remove_directory_entry(path) != FS_SUCCESS) {
        return FS_ERROR_IO_ERROR;
    }
    
    return FS_SUCCESS;
}

// 重命名文件
int fs_rename(const char* old_path, const char* new_path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    if (validate_path(old_path) != FS_SUCCESS || validate_path(new_path) != FS_SUCCESS) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 查找原文件
    fs_dirent_t entry;
    if (find_directory_entry(old_path, &entry) != FS_SUCCESS) {
        return FS_ERROR_NOT_FOUND;
    }
    
    // 检查新文件是否已存在
    fs_dirent_t new_entry;
    if (find_directory_entry(new_path, &new_entry) == FS_SUCCESS) {
        return FS_ERROR_FILE_EXISTS;
    }
    
    // 更新目录项
    char new_name[FS_MAX_FILENAME + 1];
    char new_ext[4];
    convert_filename(new_path, (uint8_t*)new_name, (uint8_t*)new_ext);
    
    memcpy(entry.name, new_name, 8);
    memcpy(entry.ext, new_ext, 3);
    
    if (update_directory_entry(old_path, &entry) != FS_SUCCESS) {
        return FS_ERROR_IO_ERROR;
    }
    
    return FS_SUCCESS;
}

// 检查文件是否存在
int fs_exists(const char* path) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    fs_dirent_t entry;
    return find_directory_entry(path, &entry) == FS_SUCCESS ? FS_SUCCESS : FS_ERROR_NOT_FOUND;
}

// 获取文件大小
int fs_get_size(const char* path) {
    if (!fs_state.initialized) {
        return -1;
    }
    
    fs_dirent_t entry;
    if (find_directory_entry(path, &entry) != FS_SUCCESS) {
        return -1;
    }
    
    return entry.size;
}

// 更新文件系统统计信息
static void update_fs_stats(void) {
    if (!fs_state.initialized) {
        return;
    }
    
    // 重置统计信息
    fs_state.stats.free_sectors = 0;
    fs_state.stats.used_sectors = 0;
    fs_state.stats.total_files = 0;
    fs_state.stats.total_dirs = 0;
    
    // 计算空闲扇区数
    for (int i = 2; i < 2880; i++) {
        uint16_t cluster = fs_read_fat(i, NULL);
        if (cluster == 0) {
            fs_state.stats.free_sectors++;
        } else {
            fs_state.stats.used_sectors++;
        }
    }
    
    // 计算文件和目录数
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
            if (fs_state.root_dir[i].attr & FS_ATTR_DIRECTORY) {
                fs_state.stats.total_dirs++;
            } else {
                fs_state.stats.total_files++;
            }
        }
    }
}

// 获取文件系统统计信息
int fs_get_stats(fs_stats_t* stats) {
    if (!fs_state.initialized || !stats) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 更新统计信息
    update_fs_stats();
    
    *stats = fs_state.stats;
    return FS_SUCCESS;
}

// 获取空闲空间
int fs_get_free_space(uint32_t* free_bytes) {
    if (!fs_state.initialized || !free_bytes) {
        return FS_ERROR_IO_ERROR;
    }
    
    *free_bytes = fs_state.stats.free_sectors * FS_SECTOR_SIZE;
    return FS_SUCCESS;
}

// 格式化文件系统
int fs_format(void) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 清零FAT表
    memset(fs_state.fat_table, 0, FS_FAT_SIZE * FS_SECTOR_SIZE);
    
    // 清零根目录
    memset(fs_state.root_dir, 0, FS_ROOT_ENTRIES * sizeof(fs_dirent_t));
    
    // 写回磁盘
    for (int i = 0; i < FS_FAT_SIZE; i++) {
        if (write_fat_sector(1 + i, (uint8_t*)fs_state.fat_table + i * FS_SECTOR_SIZE) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
    }
    
    if (fs_write_sector(FS_ROOT_SECTOR, fs_state.root_dir) != FS_SUCCESS) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 更新统计信息
    fs_state.stats.free_sectors = 2880 - 33; // 总扇区数 - 系统扇区数
    fs_state.stats.used_sectors = 33;
    fs_state.stats.total_files = 0;
    fs_state.stats.total_dirs = 0;
    
    return FS_SUCCESS;
}

// 读取扇区
int fs_read_sector(uint32_t sector, void* buffer) {
    // 这里需要实现实际的磁盘读取
    // 简化实现：返回成功但不实际读取
    (void)sector;
    (void)buffer;
    return FS_SUCCESS;
}

// 写入扇区
int fs_write_sector(uint32_t sector, const void* buffer) {
    // 这里需要实现实际的磁盘写入
    // 简化实现：返回成功但不实际写入
    (void)sector;
    (void)buffer;
    return FS_SUCCESS;
}

// 读取FAT表项
int fs_read_fat(uint16_t cluster, uint16_t* value) {
    if (!fs_state.initialized || cluster >= 2880) {
        return FS_ERROR_IO_ERROR;
    }
    
    // FAT12编码：每3个字节存储2个12位值
    uint32_t fat_offset = cluster * 3 / 2;
    uint16_t fat_entry = *(uint16_t*)((uint8_t*)fs_state.fat_table + fat_offset);
    
    if (cluster & 1) {
        // 奇数簇：取高12位
        fat_entry >>= 4;
    } else {
        // 偶数簇：取低12位
        fat_entry &= 0x0FFF;
    }
    
    if (value) {
        *value = fat_entry;
    }
    
    return FS_SUCCESS;
}

// 写入FAT表项
int fs_write_fat(uint16_t cluster, uint16_t value) {
    if (!fs_state.initialized || cluster >= 2880) {
        return FS_ERROR_IO_ERROR;
    }
    
    // FAT12编码：每3个字节存储2个12位值
    uint32_t fat_offset = cluster * 3 / 2;
    uint16_t* fat_entry = (uint16_t*)((uint8_t*)fs_state.fat_table + fat_offset);
    
    if (cluster & 1) {
        // 奇数簇：更新高12位
        *fat_entry = (*fat_entry & 0x000F) | (value << 4);
    } else {
        // 偶数簇：更新低12位
        *fat_entry = (*fat_entry & 0xF000) | (value & 0x0FFF);
    }
    
    return FS_SUCCESS;
}

// 查找空闲簇
int fs_find_free_cluster(void) {
    if (!fs_state.initialized) {
        return -1;
    }
    
    for (uint16_t i = 2; i < 2880; i++) {
        uint16_t value;
        if (fs_read_fat(i, &value) == FS_SUCCESS && value == 0) {
            return i;
        }
    }
    return -1;
}

// 分配簇
int fs_allocate_cluster(uint16_t* cluster) {
    if (!fs_state.initialized || !cluster) {
        return FS_ERROR_IO_ERROR;
    }
    
    int free_cluster = fs_find_free_cluster();
    if (free_cluster == -1) {
        return FS_ERROR_DISK_FULL;
    }
    
    if (fs_write_fat(free_cluster, 0xFFF) != FS_SUCCESS) {
        return FS_ERROR_IO_ERROR;
    }
    
    *cluster = free_cluster;
    return FS_SUCCESS;
}

// 释放簇链
int fs_free_cluster_chain(uint16_t cluster) {
    if (!fs_state.initialized) {
        return FS_ERROR_IO_ERROR;
    }
    
    while (cluster < 0xFF8) {
        uint16_t next_cluster;
        if (fs_read_fat(cluster, &next_cluster) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        if (fs_write_fat(cluster, 0) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        cluster = next_cluster;
    }
    
    return FS_SUCCESS;
}

// 查找目录项
int find_directory_entry(const char* name, fs_dirent_t* entry) {
    if (!fs_state.initialized || !name || !entry) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 去掉路径前缀，只保留文件名
    const char* filename = name;
    if (filename[0] == '/') {
        filename++;
    }
    
    char search_name[8], search_ext[3];
    convert_filename(filename, (uint8_t*)search_name, (uint8_t*)search_ext);
    
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
            if (memcmp(fs_state.root_dir[i].name, search_name, 8) == 0 &&
                memcmp(fs_state.root_dir[i].ext, search_ext, 3) == 0) {
                *entry = fs_state.root_dir[i];
                return FS_SUCCESS;
            }
        }
    }
    
    return FS_ERROR_NOT_FOUND;
}

// 添加目录项到指定目录
static int add_directory_entry_to_dir(const char* name, uint8_t attr, uint16_t cluster, uint32_t size, const char* target_dir) {
    if (!fs_state.initialized || !name) {
        return FS_ERROR_IO_ERROR;
    }
    
    // 去掉路径前缀，只保留文件名
    const char* filename = name;
    if (filename[0] == '/') {
        filename++;
    }
    
    char entry_name[8], entry_ext[3];
    convert_filename(filename, (uint8_t*)entry_name, (uint8_t*)entry_ext);
    
    // 如果目标目录是根目录，添加到根目录
    if (!target_dir || strcmp(target_dir, "/") == 0) {
        // 查找空闲目录项
        for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
            if (fs_state.root_dir[i].name[0] == 0 || fs_state.root_dir[i].name[0] == 0xE5) {
                memcpy(fs_state.root_dir[i].name, entry_name, 8);
                memcpy(fs_state.root_dir[i].ext, entry_ext, 3);
                fs_state.root_dir[i].attr = attr;
                fs_state.root_dir[i].cluster = cluster;
                fs_state.root_dir[i].size = size;
                fs_state.root_dir[i].time = 0;
                fs_state.root_dir[i].date = 0;
                memset(fs_state.root_dir[i].reserved, 0, 10);
                return FS_SUCCESS;
            }
        }
        return FS_ERROR_DISK_FULL;
    } else {
        // 添加到子目录
        fs_dirent_t dir_entry;
        if (find_directory_entry(target_dir, &dir_entry) != FS_SUCCESS) {
            return FS_ERROR_NOT_FOUND;
        }
        
        if (!(dir_entry.attr & FS_ATTR_DIRECTORY)) {
            return FS_ERROR_ACCESS_DENIED;
        }
        
        // 读取目录簇
        uint8_t sector_data[FS_SECTOR_SIZE];
        uint32_t sector = FS_DATA_SECTOR + (dir_entry.cluster - 2) * 2;
        
        if (fs_read_sector(sector, sector_data) != FS_SUCCESS) {
            return FS_ERROR_IO_ERROR;
        }
        
        // 查找空闲目录项
        fs_dirent_t* dir_entries = (fs_dirent_t*)sector_data;
        for (size_t i = 2; i < FS_SECTOR_SIZE / sizeof(fs_dirent_t); i++) {
            if (dir_entries[i].name[0] == 0 || dir_entries[i].name[0] == 0xE5) {
                memcpy(dir_entries[i].name, entry_name, 8);
                memcpy(dir_entries[i].ext, entry_ext, 3);
                dir_entries[i].attr = attr;
                dir_entries[i].cluster = cluster;
                dir_entries[i].size = size;
                dir_entries[i].time = 0;
                dir_entries[i].date = 0;
                memset(dir_entries[i].reserved, 0, 10);
                
                // 写回目录簇
                if (fs_write_sector(sector, sector_data) != FS_SUCCESS) {
                    return FS_ERROR_IO_ERROR;
                }
                
                return FS_SUCCESS;
            }
        }
        return FS_ERROR_DISK_FULL;
    }
}

// 添加目录项（保持向后兼容）
int add_directory_entry(const char* name, uint8_t attr, uint16_t cluster, uint32_t size) {
    return add_directory_entry_to_dir(name, attr, cluster, size, "/");
}

// 删除目录项
int remove_directory_entry(const char* name) {
    if (!fs_state.initialized || !name) {
        return FS_ERROR_IO_ERROR;
    }
    
    char search_name[8], search_ext[3];
    convert_filename(name, (uint8_t*)search_name, (uint8_t*)search_ext);
    
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
            if (memcmp(fs_state.root_dir[i].name, search_name, 8) == 0 &&
                memcmp(fs_state.root_dir[i].ext, search_ext, 3) == 0) {
                fs_state.root_dir[i].name[0] = 0xE5; // 标记为已删除
                return FS_SUCCESS;
            }
        }
    }
    
    return FS_ERROR_NOT_FOUND;
}

// 更新目录项
int update_directory_entry(const char* name, const fs_dirent_t* entry) {
    if (!fs_state.initialized || !name || !entry) {
        return FS_ERROR_IO_ERROR;
    }
    
    char search_name[8], search_ext[3];
    convert_filename(name, (uint8_t*)search_name, (uint8_t*)search_ext);
    
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] != 0 && fs_state.root_dir[i].name[0] != 0xE5) {
            if (memcmp(fs_state.root_dir[i].name, search_name, 8) == 0 &&
                memcmp(fs_state.root_dir[i].ext, search_ext, 3) == 0) {
                fs_state.root_dir[i] = *entry;
                return FS_SUCCESS;
            }
        }
    }
    
    return FS_ERROR_NOT_FOUND;
}

// 转换文件名
void convert_filename(const char* filename, uint8_t* name, uint8_t* ext) {
    memset(name, ' ', 8);
    memset(ext, ' ', 3);
    
    const char* dot = strchr(filename, '.');
    if (dot) {
        // 有扩展名
        size_t name_len = dot - filename;
        if (name_len > 8) name_len = 8;
        memcpy(name, filename, name_len);
        
        const char* ext_start = dot + 1;
        size_t ext_len = strlen(ext_start);
        if (ext_len > 3) ext_len = 3;
        memcpy(ext, ext_start, ext_len);
    } else {
        // 无扩展名
        size_t name_len = strlen(filename);
        if (name_len > 8) name_len = 8;
        memcpy(name, filename, name_len);
    }
    
    // 转换为大写
    for (int i = 0; i < 8; i++) {
        if (name[i] >= 'a' && name[i] <= 'z') {
            name[i] = name[i] - 'a' + 'A';
        }
    }
    for (int i = 0; i < 3; i++) {
        if (ext[i] >= 'a' && ext[i] <= 'z') {
            ext[i] = ext[i] - 'a' + 'A';
        }
    }
}

// 转换文件名回来
void convert_filename_back(const uint8_t* name, const uint8_t* ext, char* filename) {
    int pos = 0;
    
    // 复制文件名
    for (int i = 0; i < 8 && name[i] != ' '; i++) {
        filename[pos++] = name[i];
    }
    
    // 复制扩展名
    if (ext[0] != ' ') {
        filename[pos++] = '.';
        for (int i = 0; i < 3 && ext[i] != ' '; i++) {
            filename[pos++] = ext[i];
        }
    }
    
    filename[pos] = '\0';
}

// 验证文件名
bool is_valid_filename(const char* filename) {
    if (!filename || strlen(filename) == 0) {
        return false;
    }
    
    const char* dot = strchr(filename, '.');
    if (dot) {
        // 检查文件名部分
        size_t name_len = dot - filename;
        if (name_len == 0 || name_len > 8) {
            return false;
        }
        
        // 检查扩展名部分
        size_t ext_len = strlen(dot + 1);
        if (ext_len > 3) {
            return false;
        }
    } else {
        // 无扩展名
        if (strlen(filename) > 8) {
            return false;
        }
    }
    
    return true;
}

// 验证路径
int validate_path(const char* path) {
    if (!path || strlen(path) == 0) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 支持特殊路径
    if (strcmp(path, "..") == 0 || strcmp(path, ".") == 0 || strcmp(path, "/") == 0) {
        return FS_SUCCESS;
    }
    
    // 简化实现：只支持根目录下的文件
    if (path[0] != '/') {
        return FS_ERROR_INVALID_PATH;
    }
    
    const char* filename = path + 1;
    if (strlen(filename) == 0) {
        return FS_ERROR_INVALID_PATH;
    }
    
    if (!is_valid_filename(filename)) {
        return FS_ERROR_INVALID_PATH;
    }
    
    return FS_SUCCESS;
}

// 解析路径
int parse_path(const char* path, char* dir, char* filename) {
    if (!path || !dir || !filename) {
        return FS_ERROR_INVALID_PATH;
    }
    
    const char* last_slash = strrchr(path, '/');
    if (!last_slash) {
        return FS_ERROR_INVALID_PATH;
    }
    
    // 复制目录部分
    size_t dir_len = last_slash - path;
    if (dir_len >= FS_MAX_PATH) {
        return FS_ERROR_INVALID_PATH;
    }
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';
    
    // 复制文件名部分
    strcpy(filename, last_slash + 1);
    
    return FS_SUCCESS;
}

// 连接路径
int join_path(const char* dir, const char* filename, char* result) {
    if (!dir || !filename || !result) {
        return FS_ERROR_INVALID_PATH;
    }
    
    strcpy(result, dir);
    
    // 确保目录以/结尾
    size_t dir_len = strlen(result);
    if (dir_len > 0 && result[dir_len - 1] != '/') {
        result[dir_len] = '/';
        result[dir_len + 1] = '\0';
    }
    
    // 添加文件名
    strcat(result, filename);
    
    return FS_SUCCESS;
}

// 获取当前工作目录
int fs_get_cwd(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return FS_ERROR_INVALID_PATH;
    }
    
    size_t len = strlen(current_directory);
    if (len >= size) {
        return FS_ERROR_INVALID_PATH;
    }
    
    strcpy(buffer, current_directory);
    return FS_SUCCESS;
}

// 设置当前工作目录
int fs_set_cwd(const char* path) {
    return fs_chdir(path);
}

// 内部函数实现

// 查找目录项
fs_dirent_t* find_entry(const char* path) {
    if (!fs_state.initialized || !path) {
        return NULL;
    }
    
    // 简化实现：只在根目录中查找
    if (strcmp(path, "/") == 0) {
        return NULL; // 根目录本身不是目录项
    }
    
    // 去掉路径前缀 "/"
    const char* name = path;
    if (name[0] == '/') {
        name++;
    }
    
    // 在根目录中查找
    for (int i = 0; i < FS_ROOT_ENTRIES; i++) {
        if (fs_state.root_dir[i].name[0] == 0) {
            continue; // 空目录项
        }
        
        // 构建文件名进行比较
        char entry_name[FS_MAX_FILENAME + 1];
        fs_convert_name_back(fs_state.root_dir[i].name, fs_state.root_dir[i].ext, entry_name);
        
        
        // 进行大小写不敏感的比较
        if (strcasecmp(entry_name, name) == 0) {
            return &fs_state.root_dir[i];
        }
    }
    return NULL;
}

int write_fat_sector(uint32_t sector, const void* buffer) {
    return fs_write_sector(sector, buffer);
}

// 为兼容性提供的别名函数
void fs_convert_name(const char* filename, uint8_t* name, uint8_t* ext) {
    convert_filename(filename, name, ext);
}

void fs_convert_name_back(const uint8_t* name, const uint8_t* ext, char* filename) {
    convert_filename_back(name, ext, filename);
}

bool fs_is_valid_filename(const char* filename) {
    return is_valid_filename(filename);
}

int fs_validate_path(const char* path) {
    return validate_path(path);
}

int fs_parse_path(const char* path, char* dir, char* filename) {
    return parse_path(path, dir, filename);
}

int fs_join_path(const char* dir, const char* filename, char* result) {
    return join_path(dir, filename, result);
}
