#include "fs.h"
#include "lib.h"
#include "screen.h"
#include "mm.h"
#include "ide.h"

/* Variables globales */
struct directory root_dir;
struct file_descriptor open_files[MAX_FILES];
u32 next_free_sector = 100;  // Empezar después del sector 100

/* Shared sector buffer to reduce memory allocations */
static char sector_buffer[SECTOR_SIZE];
static u8 sector_buffer_in_use = 0;

/* Inicializar el sistema de archivos */
void fs_init(void) {
    int i;
    
    // Limpiar la estructura del directorio raíz
    memset(&root_dir, 0, sizeof(struct directory));
    
    // Limpiar descriptores de archivos
    for (i = 0; i < MAX_FILES; i++) {
        open_files[i].used = 0;
        open_files[i].entry = NULL;
        open_files[i].position = 0;
    }
    
    // Initialize shared buffer
    sector_buffer_in_use = 0;
    
    // Intentar cargar el directorio raíz desde el disco
    if (ide_read_sectors(IDE_MASTER, 1, 1, &root_dir) != 0) {
        print("fs     : Creating new filesystem\n");
        
        // Crear algunos archivos de ejemplo
        fs_create_file("readme.txt", 100);
        fs_create_file("welcome.txt", 200);
        
        // Guardar el directorio raíz en el disco
        ide_write_sectors(IDE_MASTER, 1, 1, &root_dir);
    } else {
        print("fs     : Loaded existing filesystem\n");
    }
}

/* Helper function to get sector buffer */
static char *get_sector_buffer(void) {
    if (sector_buffer_in_use) {
        // Fallback to dynamic allocation if buffer is in use
        return (char *)kmalloc(SECTOR_SIZE);
    }
    sector_buffer_in_use = 1;
    return sector_buffer;
}

/* Helper function to release sector buffer */
static void release_sector_buffer(char *buffer) {
    if (buffer == sector_buffer) {
        sector_buffer_in_use = 0;
    } else {
        kfree(buffer);
    }
}

/* Crear un archivo */
int fs_create_file(const char *name, u32 size) {
    int i;
    
    // Buscar si el archivo ya existe
    if (fs_find_file(name) != NULL) {
        return -1; // El archivo ya existe
    }
    
    // Buscar una entrada libre
    for (i = 0; i < MAX_FILES; i++) {
        if (!root_dir.files[i].used) {
            // Copiar el nombre
            u32 name_len = strlen(name);
            if (name_len >= MAX_FILENAME) {
                name_len = MAX_FILENAME - 1;
            }
            memcpy(root_dir.files[i].name, name, name_len);
            root_dir.files[i].name[name_len] = '\0';
            
            // Configurar el archivo
            root_dir.files[i].size = size;
            root_dir.files[i].start_sector = next_free_sector;
            root_dir.files[i].type = FILE_TYPE_REGULAR;
            root_dir.files[i].used = 1;
            
            // Actualizar el sector libre
            next_free_sector += (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
            
            root_dir.count++;
            
            // Guardar el directorio actualizado
            ide_write_sectors(IDE_MASTER, 1, 1, &root_dir);
            
            return i;
        }
    }
    
    return -1; // No hay espacio
}

/* Eliminar un archivo */
int fs_delete_file(const char *name) {
    struct file_entry *file = fs_find_file(name);
    
    if (file == NULL) {
        return -1; // Archivo no encontrado
    }
    
    // Marcar como no usado
    file->used = 0;
    root_dir.count--;
    
    // Guardar el directorio actualizado
    ide_write_sectors(IDE_MASTER, 1, 1, &root_dir);
    
    return 0;
}

/* Abrir un archivo */
int fs_open_file(const char *name) {
    struct file_entry *file = fs_find_file(name);
    int i;
    
    if (file == NULL) {
        return -1; // Archivo no encontrado
    }
    
    // Buscar un descriptor libre
    for (i = 0; i < MAX_FILES; i++) {
        if (!open_files[i].used) {
            open_files[i].entry = file;
            open_files[i].position = 0;
            open_files[i].used = 1;
            return i;
        }
    }
    
    return -1; // No hay descriptores disponibles
}

/* Cerrar un archivo */
void fs_close_file(int fd) {
    if (fd >= 0 && fd < MAX_FILES && open_files[fd].used) {
        open_files[fd].used = 0;
        open_files[fd].entry = NULL;
        open_files[fd].position = 0;
    }
}

/* Optimized read function that uses less memory */
int fs_read_file(int fd, void *buffer, u32 size) {
    if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used) {
        return -1;
    }
    
    struct file_descriptor *file_desc = &open_files[fd];
    struct file_entry *file = file_desc->entry;
    
    // Verificar límites
    if (file_desc->position >= file->size) {
        return 0; // EOF
    }
    
    if (file_desc->position + size > file->size) {
        size = file->size - file_desc->position;
    }
    
    // Leer desde el disco usando shared buffer
    u32 sector = file->start_sector + (file_desc->position / SECTOR_SIZE);
    u32 offset = file_desc->position % SECTOR_SIZE;
    
    char *temp_buffer = get_sector_buffer();
    if (temp_buffer == NULL) {
        print("fs     : ERROR - Cannot allocate sector buffer\n");
        return -1;
    }
    
    if (ide_read_sectors(IDE_MASTER, sector, 1, temp_buffer) != 0) {
        release_sector_buffer(temp_buffer);
        return -1;
    }
    
    memcpy(buffer, temp_buffer + offset, size);
    release_sector_buffer(temp_buffer);
    
    file_desc->position += size;
    return size;
}

/* Optimized streaming read function for large files */
int fs_stream_read(int fd, void (*output_func)(char), u32 chunk_size) {
    if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used) {
        return -1;
    }
    
    struct file_descriptor *file_desc = &open_files[fd];
    struct file_entry *file = file_desc->entry;
    
    if (file_desc->position >= file->size) {
        return 0; // EOF
    }
    
    char *temp_buffer = get_sector_buffer();
    if (temp_buffer == NULL) {
        print("fs     : ERROR - Cannot allocate sector buffer\n");
        return -1;
    }
    
    u32 total_read = 0;
    u32 remaining = file->size - file_desc->position;
    
    while (remaining > 0 && total_read < chunk_size) {
        u32 sector = file->start_sector + (file_desc->position / SECTOR_SIZE);
        u32 offset = file_desc->position % SECTOR_SIZE;
        
        if (ide_read_sectors(IDE_MASTER, sector, 1, temp_buffer) != 0) {
            release_sector_buffer(temp_buffer);
            return -1;
        }
        
        u32 bytes_in_sector = SECTOR_SIZE - offset;
        u32 bytes_to_read = (remaining < bytes_in_sector) ? remaining : bytes_in_sector;
        
        for (u32 i = 0; i < bytes_to_read; i++) {
            output_func(temp_buffer[offset + i]);
        }
        
        file_desc->position += bytes_to_read;
        total_read += bytes_to_read;
        remaining -= bytes_to_read;
    }
    
    release_sector_buffer(temp_buffer);
    return total_read;
}

/* Escribir a un archivo */
int fs_write_file(int fd, const void *buffer, u32 size) {
    if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used) {
        return -1;
    }
    
    struct file_descriptor *file_desc = &open_files[fd];
    struct file_entry *file = file_desc->entry;
    
    // Verificar límites
    if (file_desc->position + size > file->size) {
        return -1; // Archivo demasiado pequeño
    }
    
    // Escribir al disco usando shared buffer
    u32 sector = file->start_sector + (file_desc->position / SECTOR_SIZE);
    u32 offset = file_desc->position % SECTOR_SIZE;
    
    char *temp_buffer = get_sector_buffer();
    if (temp_buffer == NULL) {
        print("fs     : ERROR - Cannot allocate sector buffer\n");
        return -1;
    }
    
    // Leer el sector actual
    if (ide_read_sectors(IDE_MASTER, sector, 1, temp_buffer) != 0) {
        release_sector_buffer(temp_buffer);
        return -1;
    }
    
    // Modificar el buffer
    memcpy(temp_buffer + offset, buffer, size);
    
    // Escribir de vuelta
    if (ide_write_sectors(IDE_MASTER, sector, 1, temp_buffer) != 0) {
        release_sector_buffer(temp_buffer);
        return -1;
    }
    
    release_sector_buffer(temp_buffer);
    file_desc->position += size;
    return size;
}

/* Listar archivos */
int fs_list_files(void) {
    int i;
    int count = 0;
    
    print("Files in root directory:\n");
    print("Name                Size     Type\n");
    print("----                ----     ----\n");
    
    for (i = 0; i < MAX_FILES; i++) {
        if (root_dir.files[i].used) {
            print(root_dir.files[i].name);
            
            // Pad with spaces
            u32 name_len = strlen(root_dir.files[i].name);
            for (u32 j = name_len; j < 20; j++) {
                print(" ");
            }
            
            print_dec(root_dir.files[i].size);
            print("     ");
            
            if (root_dir.files[i].type == FILE_TYPE_REGULAR) {
                print("FILE");
            } else if (root_dir.files[i].type == FILE_TYPE_DIRECTORY) {
                print("DIR");
            }
            
            print("\n");
            count++;
        }
    }
    
    print("Total files: ");
    print_dec(count);
    print("\n");
    
    return count;
}

/* Buscar un archivo */
struct file_entry *fs_find_file(const char *name) {
    int i;
    
    for (i = 0; i < MAX_FILES; i++) {
        if (root_dir.files[i].used) {
            // Comparar nombres
            char *file_name = root_dir.files[i].name;
            const char *search_name = name;
            
            u8 match = 1;
            while (*file_name && *search_name) {
                if (*file_name != *search_name) {
                    match = 0;
                    break;
                }
                file_name++;
                search_name++;
            }
            
            if (match && *file_name == '\0' && *search_name == '\0') {
                return &root_dir.files[i];
            }
        }
    }
    
    return NULL;
}

/* Get file system statistics */
void fs_get_stats(struct fs_stats *stats) {
    stats->total_files = 0;
    stats->total_size = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (root_dir.files[i].used) {
            stats->total_files++;
            stats->total_size += root_dir.files[i].size;
        }
    }
    
    stats->free_files = MAX_FILES - stats->total_files;
    stats->next_free_sector = next_free_sector;
}

/* Print file system statistics */
void fs_print_stats(void) {
    struct fs_stats stats;
    fs_get_stats(&stats);
    
    print("File System Statistics:\n");
    print("======================\n");
    print("Total files: ");
    print_dec(stats.total_files);
    print("/");
    print_dec(MAX_FILES);
    print("\n");
    
    print("Total size: ");
    print_dec(stats.total_size);
    print(" bytes\n");
    
    print("Next free sector: ");
    print_dec(stats.next_free_sector);
    print("\n");
}