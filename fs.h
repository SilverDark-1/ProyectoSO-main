#ifndef FS_H_
#define FS_H_

#include "types.h"

#define MAX_FILES 32
#define MAX_FILENAME 32
#define SECTOR_SIZE 512

/* Tipos de archivos */
#define FILE_TYPE_REGULAR   1
#define FILE_TYPE_DIRECTORY 2

/* Estructura de entrada de archivo */
struct file_entry {
    char name[MAX_FILENAME];
    u32 size;
    u32 start_sector;
    u8 type;
    u8 used;
} __attribute__((packed));

/* Estructura de directorio */
struct directory {
    struct file_entry files[MAX_FILES];
    u32 count;
} __attribute__((packed));

/* Descriptor de archivo */
struct file_descriptor {
    struct file_entry *entry;
    u32 position;
    u8 used;
};

/* File system statistics */
struct fs_stats {
    u32 total_files;
    u32 free_files;
    u32 total_size;
    u32 next_free_sector;
};

/* Variables globales */
extern struct directory root_dir;
extern struct file_descriptor open_files[MAX_FILES];

/* Funciones del sistema de archivos */
void fs_init(void);
int fs_create_file(const char *name, u32 size);
int fs_delete_file(const char *name);
int fs_open_file(const char *name);
void fs_close_file(int fd);
int fs_read_file(int fd, void *buffer, u32 size);
int fs_stream_read(int fd, void (*output_func)(char), u32 chunk_size);
int fs_write_file(int fd, const void *buffer, u32 size);
int fs_list_files(void);
struct file_entry *fs_find_file(const char *name);
void fs_get_stats(struct fs_stats *stats);
void fs_print_stats(void);

#endif