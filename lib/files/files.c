#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "files.h"
#include <stdlib.h>


// sizes used for erasing (memory supports erasing of 4KB, 32KB, 64KB or entire chip)
#define ERASE_DATA_SIZE (64 * 1024) // 64KB for 64 1KB files
#define ERASE_NAMES_SIZE FLASH_SECTOR_SIZE // 1KB for 64 16-byte file names (it's reserved as whole 4K sector because apparently SDK can only do sector-level erasing)

#define DATA_SECTOR_SIZE (ERASE_DATA_SIZE)
#define NAMES_SECTOR_SIZE (1024)

#define FLASH_DATA_OFFSET (2048 * 1024 - ERASE_DATA_SIZE)
#define FLASH_NAMES_OFFSET (2048 * 1024 - ERASE_DATA_SIZE - ERASE_NAMES_SIZE) 

const uint8_t *flash_data_contents = (const uint8_t *) (XIP_BASE + FLASH_DATA_OFFSET);
const uint8_t *flash_names_contents = (const uint8_t *) (XIP_BASE + FLASH_NAMES_OFFSET);

void GetFilesInfo(FilesInfo* files_info) {

    // for every file
    for (int i = 0; i < AMOUNT_OF_FILES; i++) {
        // Copy data into the struct
        memcpy(files_info->file_names[i],
            &flash_names_contents[i * LINE_SIZE],
            LINE_SIZE);

        int len = 0;
        // Increase len until it hits the name size or an empty memory byte appears
        while (len < LINE_SIZE)
            if (files_info->file_names[i][len] == 0xFF)
                break;
            else
                len++;
        // set file length inside struct
        files_info->name_lengths[i] = len;
    }}

void GetFileData(FileData* file_data, int pos) {
    for (int i = 0; i < AMOUNT_OF_LINES; i++) {
        memcpy(file_data->data[i],
            &flash_data_contents[pos*DATA_SIZE + i*LINE_SIZE],
            LINE_SIZE);
        
        int len = 0;
        while (len < LINE_SIZE) {
            if (file_data->data[i][len] == 0xFF)
                break;
            else
                len++;
        }
        file_data->line_lengths[i] = len;
    }
}

void WriteFilesInfo(FilesInfo* files_info) {
    uint8_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_NAMES_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_NAMES_OFFSET, (char*)files_info->file_names, NAMES_SECTOR_SIZE);
    restore_interrupts(ints);
}

void WriteFileData(FileData* file_data, int pos) {
    const int files_per_sector = FLASH_SECTOR_SIZE / DATA_SIZE;
    const int current_sector = pos / files_per_sector;
    const int file_in_sector = pos % files_per_sector;
    const int sector_offset = FLASH_DATA_OFFSET + current_sector*FLASH_SECTOR_SIZE;
    // get existing sector data from flash into a buffer
    char buf[FLASH_SECTOR_SIZE];
    memcpy(buf,
        &flash_data_contents[current_sector*FLASH_SECTOR_SIZE],
        FLASH_SECTOR_SIZE);
    // put new file data into the buffer
    char *data_pointer = (char*)file_data->data;
    memmove(&buf[file_in_sector*DATA_SIZE],
        data_pointer,
        DATA_SIZE);
    // write modified sector to flash
    uint8_t ints = save_and_disable_interrupts();
    flash_range_erase(sector_offset, FLASH_SECTOR_SIZE);
    flash_range_program(sector_offset, buf, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

void CreateFile(FilesInfo* files_info, int pos, char* name, int len) {
    files_info->name_lengths[pos] = len;
    memcpy(files_info->file_names[pos], name, len*sizeof(char));
    for (int i = len; i < LINE_SIZE; i++) {
        files_info->file_names[pos][i] = 255;
    }

    WriteFilesInfo(files_info);
}

void DeleteFile(FilesInfo* files_info, FileData* file_data, int pos) {
    for (int i = 0; i < LINE_SIZE; i++)
        files_info->file_names[pos][i] = 255;
    
    WriteFilesInfo(files_info);
    files_info->name_lengths[pos] = 0;
    
    for (int i = 0; i < AMOUNT_OF_LINES; i++)
        for (int j = 0; j < LINE_SIZE; j++)
            file_data->data[i][j] = 255;

    WriteFileData(file_data, pos);
}

void EraseAll() {
    const int pages = 68 * 4;
    uint8_t buf[256];
    for (int i = 0; i < 256; i++)
        buf[i] = 0xFF;

    for (int i = 0; i < pages; i++) {
        flash_range_erase(FLASH_NAMES_OFFSET + i*FLASH_PAGE_SIZE, FLASH_PAGE_SIZE);
        flash_range_program(FLASH_NAMES_OFFSET + i*FLASH_PAGE_SIZE, buf, FLASH_PAGE_SIZE);   
    }
    
}