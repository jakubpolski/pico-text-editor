#pragma once

#define AMOUNT_OF_FILES (64)
#define AMOUNT_OF_LINES (64)
#define LINE_SIZE (16)
#define DATA_SIZE (1024)

typedef struct FilesInfo {
    char file_names[AMOUNT_OF_FILES][LINE_SIZE];
    int name_lengths[AMOUNT_OF_FILES];
} FilesInfo;

typedef struct FileData {
    char data[AMOUNT_OF_LINES][LINE_SIZE];
    int line_lengths[AMOUNT_OF_LINES];
} FileData;

void GetFilesInfo(FilesInfo* files_info);
void GetFileData(FileData* file_data, int pos);
void WriteFilesInfo(FilesInfo* files_info);
void WriteFileData(FileData* file_data, int pos);
void CreateFile(FilesInfo* files_info, int pos, char* name, int len); 
void DeleteFile(FilesInfo* files_info, FileData* file_data, int pos);
void EraseAll();