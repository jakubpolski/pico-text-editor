#include "lcd.h"
#include "files.h"
#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include "tusb.h"
#include "bsp/board.h"
#include "pico/stdlib.h"

typedef enum CurrentMenu {
    FileSelection,
    ExistingFileOperations,
    NewFileOperations,
    FileNameSelection,
    TextEditor,
    EditorExitPrompt
} CurrentMenu;

typedef enum SelectedOperation {
    FileOpen,
    FileCreate,
    FileSave,
    Discard,
    FileRename,
    FileDelete,
    GoBack
} SelectedOperation;

// Stores current position on display
int lcd_row, lcd_col;
// Stores selected file and current line in editor
int current_file, current_line;
// Stores whether line indexes should be shown (controlled by 'tab' on keyboard)
int show_indexes = 0;
// Stores whether we are adding or replacing chars (controlled by 'insert' on keyboard)
int insert_mode = 0;
// Stores whether keyboard was mounted on first boot
int device_mounted = 0;

// Stores current name when in new file/renaming menu
char new_name_buf[16];
// Stores length of current name when in new file/renaming menu 
int new_name_len = 0;

// Stores all file names and their lengths 
FilesInfo files_info;
// Stores currently opened file's lines and their lengths 
FileData file_data;
// Stores which menu the program is currently in
CurrentMenu current_menu;
// Stores which operation is currently selected
SelectedOperation selected_operation;

// internal functions
inline int DistanceBetweenCursorAndLineEnd(int len);

void FileSelectionAt(int pos);
void FileNameSelectionDefaults();
void ExistingFileOperationsDefaults();
void NewFileOperationsDefaults();
void FileRenameDefaults();
void TextEditorDefaults();
void EditorExitPromptDefaults();

int LineAddChar(char chr, char* line, int* len);
void LineDelete(char* line, int* len);
void LineBackspace(char* line, int* len);

void EditorAddChar(char chr);
void EditorDelete();
void EditorBackspace();
void EditorEnter();

void PrintFileName(int pos, int row);
void PrintDataLine(int pos, int row);

// ----------------------------------------------------
// functions exposed in the header file
// ----------------------------------------------------

// sets mounted flag to true (only needed on first boot)
void ProcessMount() {
    device_mounted = 1;
}

// depending on current menu, redirects char input into proper functions
void ProcessChar(char chr) {
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
    case FileNameSelection:
        // '<' and '>' are reserved for showing <Empty slot> on empty file
        if (chr != '<' && chr != '>')
            LineAddChar(chr, new_name_buf, &new_name_len);
        break;
    case TextEditor:
        EditorAddChar(chr);
        break;
    default:
        break;
    }
}

// depending on current menu, executes left arrow operations
void ProcessArrowLeft() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
    case ExistingFileOperations:
        // select between "Open Rename Delete Back"
        switch (selected_operation) {
        case FileRename:
            selected_operation = FileOpen;
            SetCursor(0, BOTTOM_ROW);
            Print("      Open     >");
            break;
        case FileDelete:
            selected_operation = FileRename;
            SetCursor(0, BOTTOM_ROW);
            Print("<    Rename    >");
            break;
        case GoBack:
            selected_operation = FileDelete;
            SetCursor(0, BOTTOM_ROW);
            Print("<    Delete    >");
            break;
        }
        break;
    case NewFileOperations:
        // select between "Create Back"
        if (selected_operation == GoBack) {
            selected_operation = FileCreate;
            SetCursor(0, BOTTOM_ROW);
            Print("     Create    >");
        }
        break;

    case FileNameSelection:
        // move left if not at line beginning
        if (lcd_col > 0)
            SetCursor(--lcd_col, lcd_row);
        break;

    case TextEditor:
        // if cursor is not at line beginning
        if (lcd_col > 0) {
            // move it to the left
            SetCursor(--lcd_col, lcd_row);
        // if cursor is at (not first) line beginning
        } else if (current_line > 0) {
            // move it at the end of previous line
            lcd_col = LINE_SIZE;
            ProcessArrowUp();
        }
        break;

    case EditorExitPrompt:
        // switch between "Save Discard Back"
        switch (selected_operation) {
            case Discard:
                selected_operation = FileSave;
                SetCursor(0, BOTTOM_ROW);
                Print("      Save     >");
                break;
            case GoBack:
                selected_operation = Discard;
                SetCursor(0, BOTTOM_ROW);
                Print("<    Discard   >");
                break;
        }
        break;

    default:
        break;
    }
}

// depending on current menu, executes right arrow operations
void ProcessArrowRight() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
    case ExistingFileOperations:
        // select between "Open Rename Delete Back"
        switch (selected_operation) {
        case FileOpen:
            selected_operation = FileRename;
            SetCursor(0, BOTTOM_ROW);
            Print("<    Rename    >");
            break;

        case FileRename:
            selected_operation = FileDelete;
            SetCursor(0, BOTTOM_ROW);
            Print("<    Delete    >");
            break;

        case FileDelete:
            selected_operation = GoBack;
            SetCursor(0, BOTTOM_ROW);
            Print("<     Back      ");
            break;
        }
        break;

    case NewFileOperations:
        // select between "Create Back"
        if (selected_operation == FileCreate) {
            selected_operation = GoBack;
            SetCursor(0, BOTTOM_ROW);
            Print("<     Back      ");
        }
        break;

    case FileNameSelection:
        // if cursor is before the end of the name
        if (lcd_col < new_name_len)
            // move it forwards by 1
            SetCursor(++lcd_col, lcd_row);
        break;

    case TextEditor:
        // if cursor is before the end of the line
        if (lcd_col < file_data.line_lengths[current_line]) {
            // move it forwards by 1
            SetCursor(++lcd_col, lcd_row);
        // if cursor is at the end of the line, that is not the last one
        } else if (current_line < AMOUNT_OF_LINES-1) {
            // move it at the beginning of next line
            lcd_col = 0;
            ProcessArrowDown();
        }
        break;

    case EditorExitPrompt:
        // switch between "Save Discard Back"
        switch (selected_operation) {
            case FileSave:
                selected_operation = Discard;
                SetCursor(0, BOTTOM_ROW);
                Print("<    Discard   >");
                break;
            case Discard:
                selected_operation = GoBack;
                SetCursor(0, BOTTOM_ROW);
                Print("<     Back      ");
                break;
        }
        break;

    default:
        break;
    }
}

// depending on current menu, executes up arrow operations
void ProcessArrowUp() {
    switch (current_menu) {
    case FileSelection:
        // if not at the first file
        if (current_file > 0) {
            if (lcd_row == BOTTOM_ROW) {
                // only change the selected file and current row
                current_file--;
                lcd_row = TOP_ROW;
            // if cursor is at the top row
            } else {
                // print previous file at the bottom
                PrintFileName(current_file--, BOTTOM_ROW);
                // print current file at the top
                PrintFileName(current_file, TOP_ROW);
            }
            // place cursor at the beginning of selected file name
            SetCursor(0, lcd_row);
        }
        break;

    case TextEditor:
        // if not at the first line
        if (current_line > 0) {
            if (lcd_row == BOTTOM_ROW) {
                // only change the selected line and current row
                current_line--;
                lcd_row = TOP_ROW;
            // if cursor is at the top row
            } else {
                // print previous line at the bottom
                PrintDataLine(current_line--, BOTTOM_ROW);
                // print current line at the top
                PrintDataLine(current_line, TOP_ROW);
            }
            // if after changing lines cursor is after the end of current line
            if (lcd_col > file_data.line_lengths[current_line])
                // move it at the end of current line
                lcd_col = file_data.line_lengths[current_line];
            // set final cursor postion on display
            SetCursor(lcd_col, lcd_row);
        }
        break;

    default:
        break;
    }
}

// depending on current menu, executes arrow down operations
void ProcessArrowDown(){
    switch (current_menu) {
    case FileSelection:
        // if not at the last file
        if (current_file < AMOUNT_OF_FILES-1) {
            if (lcd_row == BOTTOM_ROW) {
                // print previous file name at top row
                PrintFileName(current_file, TOP_ROW);
                // print current file name at bottom row
                PrintFileName(++current_file, BOTTOM_ROW);
            // if cursor is at the top row
            } else {
                // only change selected file and lcd row
                current_file++;
                lcd_row = BOTTOM_ROW;
            }
            // place the cursor at the beginning of selected file name
            SetCursor(0, lcd_row);
        }
        break;

    case TextEditor:
        // if not at the last line
        if (current_line < AMOUNT_OF_LINES-1) {
            if (lcd_row == BOTTOM_ROW) {
                // print previous line at top row
                PrintDataLine(current_line, TOP_ROW);
                // print current line at bottom row
                PrintDataLine(++current_line, BOTTOM_ROW);
            // if cursor is at top row
            } else {
                // only increase current line at move cursor at the bottom row
                current_line++;
                lcd_row = BOTTOM_ROW;
            }
        }
        // if after changing lines cursor is after the end of current line
        if (lcd_col > file_data.line_lengths[current_line])
            // move it at the end of current line
            lcd_col = file_data.line_lengths[current_line];
        // set final cursor postion on display
        SetCursor(lcd_col, lcd_row);
        break;

    default:
        break;
    }
}

// depending on current menu, executes escape operations
void ProcessEscape() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
	switch (current_menu) {
    case FileSelection:
        // go back to the first file
        FileSelectionAt(0);
        break;
    case ExistingFileOperations:
    case NewFileOperations:
    case FileNameSelection:
        // return to selected file
        FileSelectionAt(current_file);
        break;
    case TextEditor:
        EditorExitPromptDefaults();
        break;
    case EditorExitPrompt:
        TextEditorDefaults();
        break;
    default:
        break;
    }
}

// depending on current menu, executes enter operations
void ProcessEnter() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
    case FileSelection:
        // if file exists
        if (files_info.name_lengths[current_file] > 0)
            ExistingFileOperationsDefaults();
        // if it doesen't
        else
            NewFileOperationsDefaults();
        break;

    case ExistingFileOperations:
        switch (selected_operation) {
        case FileOpen:
            ClearDisplay();
            Print("Opening file...");
            sleep_ms(1000);

            GetFileData(&file_data, current_file);
            TextEditorDefaults();
            break;
        case FileRename:
            FileRenameDefaults();
            break;
        case FileDelete:
            DeleteFile(&files_info, &file_data, current_file);
            FileSelectionAt(current_file);
            break;
        case GoBack:
            FileSelectionAt(current_file);
            break;
        default:
            break;
        }
        break;

    case NewFileOperations:
        switch (selected_operation) {
        case FileCreate:
            FileNameSelectionDefaults();
            break;
        case GoBack:
            FileSelectionAt(0);
            break;
        default: 
            break;
        }
        break;

    case FileNameSelection:
        if (new_name_len > 0) {
            CreateFile(&files_info, current_file, new_name_buf, new_name_len);

            CursorOff();
            BlinkingOff();
            ClearDisplay();
            if (selected_operation == FileCreate)
                Print("File created");
            else
                Print("File renamed");
            sleep_ms(1000);
            FileSelectionAt(current_file);
        }
    break;

    case TextEditor:
        EditorEnter();
        break;

    case EditorExitPrompt:
        switch (selected_operation) {
            case FileSave:
                WriteFileData(&file_data, current_file);
                FileSelectionAt(current_file);
                break;
            case Discard:
                FileSelectionAt(current_file);
                break;
            case GoBack:
                TextEditorDefaults();
                break;
            default:
                break;
        }
        break;

    default:
        break;
    }
}

// depending on current menu, executes delete operations
void ProcessDelete() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
	switch (current_menu) {
        case FileNameSelection:
            LineDelete(new_name_buf, &new_name_len);
            break;
        case TextEditor:
            EditorDelete();
            break;
        default:
            break;
    }
}

// depending on current menu, executes backspace operations
void ProcessBackspace() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
        case FileNameSelection:
            LineBackspace(new_name_buf, &new_name_len);
            break;
        case TextEditor:
            EditorBackspace();
            break;
        default:
            break;
    }
}

// depending on current menu, executes tab operations
void ProcessTab() {
    switch (current_menu) {
        case FileSelection:
            // show/hide indexes in file selection
            show_indexes = !show_indexes;
            if (show_indexes || insert_mode) {
                CursorOff();
                BlinkingOn();
            } else {
                CursorOn();
                BlinkingOff();
            }
            PrintFileName(current_file, lcd_row);
            if (lcd_row == TOP_ROW)
                PrintFileName(current_file+1, BOTTOM_ROW);
            else
                PrintFileName(current_file-1, TOP_ROW);
            SetCursor(lcd_col, lcd_row);
            break;

        case TextEditor:
            // show/hide indexes in text editor
            show_indexes = !show_indexes;
            if (show_indexes || insert_mode) {
                CursorOff();
                BlinkingOn();
            } else {
                CursorOn();
                BlinkingOff();
            }
            PrintDataLine(current_line, lcd_row);
            if (lcd_row == TOP_ROW)
                PrintDataLine(current_line+1, BOTTOM_ROW);
            else
                PrintDataLine(current_line-1, TOP_ROW);
            SetCursor(lcd_col, lcd_row);
            break;

        default:
            break;
    }
}

// depending on current menu, executes insert operations
void ProcessInsert() {
    // hide indexes if they are currently shown and stop
    if (show_indexes) {
        ProcessTab();
        return;
    }
    switch (current_menu) {
    case FileNameSelection:
    case TextEditor:
        // enter/leave insert mode and switch between blinking and cursor-only
        insert_mode = !insert_mode;
        if (insert_mode) {
            CursorOff();
            BlinkingOn();
        } else {
            CursorOn();
            BlinkingOff();
        }
        break;
    default:
        break;
    }
}

void ProcessPageDown() {
    switch (current_menu) {
    case FileSelection:
        if ((lcd_row == BOTTOM_ROW && current_file < AMOUNT_OF_FILES-2) || 
            (lcd_row == TOP_ROW && current_file == AMOUNT_OF_FILES-3)) {
            current_file += 2;
            PrintFileName(current_file-1, TOP_ROW);
            PrintFileName(current_file, BOTTOM_ROW);
            lcd_row = BOTTOM_ROW;
        } else if (lcd_row == TOP_ROW && current_file == AMOUNT_OF_FILES-2) {
            current_file++;
            PrintFileName(current_file-1, TOP_ROW);
            PrintFileName(current_file, BOTTOM_ROW);
            lcd_row = BOTTOM_ROW;
        } else if (lcd_row == TOP_ROW && current_file < AMOUNT_OF_FILES-3) {
            current_file += 2;
            PrintFileName(current_file, TOP_ROW);
            PrintFileName(current_file+1, BOTTOM_ROW);
        }
        SetCursor(lcd_col, lcd_row);
        break;
    
    case TextEditor:
        if ((lcd_row == BOTTOM_ROW && current_line < AMOUNT_OF_FILES-2) || 
            (lcd_row == TOP_ROW && current_line == AMOUNT_OF_FILES-3)) {
            current_line += 2;
            PrintDataLine(current_line-1, TOP_ROW);
            PrintDataLine(current_line, BOTTOM_ROW);
            lcd_row = BOTTOM_ROW;
        } else if (lcd_row == TOP_ROW && current_line == AMOUNT_OF_FILES-2) {
            current_line++;
            PrintDataLine(current_line-1, TOP_ROW);
            PrintDataLine(current_line, BOTTOM_ROW);
            lcd_row = BOTTOM_ROW;
        } else if (lcd_row == TOP_ROW && current_line < AMOUNT_OF_FILES-3) {
            current_line += 2;
            PrintDataLine(current_line, TOP_ROW);
            PrintDataLine(current_line+1, BOTTOM_ROW);
        }
        if (lcd_col > file_data.line_lengths[current_line])
            lcd_col = file_data.line_lengths[current_line];
        SetCursor(lcd_col, lcd_row);
        break;

    default:
        break;
    }
}

void ProcessPageUp() {
    switch (current_menu) {
    case FileSelection:
        if ((lcd_row == TOP_ROW && current_file > 1) ||
            (lcd_row == BOTTOM_ROW && current_file == 2)) {
            current_file -= 2;
            PrintFileName(current_file, TOP_ROW);
            PrintFileName(current_file+1, BOTTOM_ROW);
            lcd_row = TOP_ROW;
        } else if (lcd_row == BOTTOM_ROW && current_file == 1) {
            current_file--;
            PrintFileName(current_file, TOP_ROW);
            PrintFileName(current_file+1, BOTTOM_ROW);
            lcd_row = TOP_ROW;
        } else if (lcd_row == BOTTOM_ROW && current_file > 2) {
            current_file -= 2;
            PrintFileName(current_file-1, TOP_ROW);
            PrintFileName(current_file, BOTTOM_ROW);
        }
        SetCursor(lcd_col, lcd_row);
        break;

    case TextEditor:
        if ((lcd_row == TOP_ROW && current_line > 1) ||
            (lcd_row == BOTTOM_ROW && current_line == 2)) {
            current_line -= 2;
            PrintDataLine(current_line, TOP_ROW);
            PrintDataLine(current_line+1, BOTTOM_ROW);
            lcd_row = TOP_ROW;
        } else if (lcd_row == BOTTOM_ROW && current_line == 1) {
            current_line--;
            PrintDataLine(current_line, TOP_ROW);
            PrintDataLine(current_line+1, BOTTOM_ROW);
            lcd_row = TOP_ROW;
        } else if (lcd_row == BOTTOM_ROW && current_line > 2) {
            current_line -= 2;
            PrintDataLine(current_line-1, TOP_ROW);
            PrintDataLine(current_line, BOTTOM_ROW);
        }
        if (lcd_col > file_data.line_lengths[current_line])
            lcd_col = file_data.line_lengths[current_line];
        SetCursor(lcd_col, lcd_row);
        break;

    default:
        break;
    }
}

void ProcessHome() {
    switch (current_menu) {
    case FileSelection:
        FileSelectionAt(0);
        break;

    case FileNameSelection:
        lcd_col = 0;
        SetCursor(lcd_col, lcd_row);
        break;
    case TextEditor:
        if (show_indexes) {
            current_line = 0;
            PrintDataLine(current_line, TOP_ROW);
            PrintDataLine(current_line+1, BOTTOM_ROW);
            lcd_row = TOP_ROW;
        }
        lcd_col = 0;
        SetCursor(lcd_col, lcd_row);
        break;

    default:
        break;
    }
}

void ProcessEnd() {
    switch (current_menu) {
    case FileSelection:
        FileSelectionAt(AMOUNT_OF_FILES-1);
        break;
    
    case FileNameSelection:
        lcd_col = new_name_len;
        SetCursor(lcd_col, lcd_row);
    case TextEditor:
        if (show_indexes) {
            current_line = AMOUNT_OF_LINES-1;
            PrintDataLine(current_line-1, TOP_ROW);
            PrintDataLine(current_line, BOTTOM_ROW);
            lcd_row = BOTTOM_ROW;
            lcd_col = 0;
        } else {
            lcd_col = file_data.line_lengths[current_line];
        }
        SetCursor(lcd_col, lcd_row);
        break;

    default:
        break;
    }
}

// starts the editor
void EditorInitialize() {
    // initialize onboard led
    board_init();
    // initialize usb stack
    tusb_init();
    // show title screen
    InitializeDisplay();
    Print("Pico Editor v1.0");
    SetCursor(0, BOTTOM_ROW);
    Print("Connect keyboard");
    // Wait until keyboard is connected
    while (!device_mounted)
        tuh_task();
    sleep_ms(100);
    SetCursor(0, BOTTOM_ROW);
    Print("Connected!      ");
    sleep_ms(500);
    // Show prompt "Select file"
    ClearDisplay();
    Print("Select file");
    sleep_ms(1000);
    // Get file names from flash
    GetFilesInfo(&files_info);
    // Enter file selection
    FileSelectionAt(0);
    // Program loop
    while (1)
        tuh_task();
}


// ----------------------------------------------------
// internal functions
// ----------------------------------------------------

/*
    ---
    Compares distance between current lcd_col position and provided length
    ---
    positive value means cursor is before last character
    negative value means cursor is after last character
*/
inline int DistanceBetweenCursorAndLineEnd(int len) {
    return len-1 - lcd_col;
}

/*
    ---
    Prints data from files_info on display
    ---
    First "pos" parameter is file index
    Second "row" parameter is the row to be printed in
*/
void PrintFileName(int pos, int row) {
    SetCursor(0, row);
    // length of current line
    int len = files_info.name_lengths[pos];
    // Posistion from where display should be cleared
    int empty_space_start = MAX_CHARS;
    if (show_indexes) {
        // Indexes can have max 2 digits and a sepataror between name
        enum {index_length = 3};
        // Print current line index
        char buf[index_length];
        Print(itoa(pos, buf, 10));
        // Print additional space to align single digit numbers 
        if (pos < 10)
            Write(' ');
        // Print the sepatator
        Write(255);
        
        if (len == 0) {
            Print("<Empty slot> ");
        } else {
            // Remaining space on display
            const int remaining_space = MAX_CHARS - index_length;
            // don't print too many characters if name is long enough
            if (len > remaining_space) {
                len = remaining_space;
            // clear data after name end if it's short enough
            } else {
                empty_space_start = len + index_length;
            }
            // Print name
            PrintN(files_info.file_names[pos], len);
        }
    // If not showing indexes
    } else {
        if (len == 0) {
            Print("<Empty slot>    ");
        } else {
            PrintN(files_info.file_names[pos], len);
            empty_space_start = len;
        }
    }
    // clear everyting after name end
    for (int i = empty_space_start; i < MAX_CHARS; i++)
        Write(' ');
}


void PrintDataLine(int pos, int row) {
    SetCursor(0, row);
    // length of current line
    int len = file_data.line_lengths[pos];
    int empty_space_start;
    if (show_indexes) {
        // Indexes can have max 2 digits and a sepataror between name
        enum {index_length = 3};
        // Print current line index
        char buf[index_length];
        Print(itoa(pos, buf, 10));
        // Print additional space to align single digit numbers 
        if (pos < 10)
            Write(' ');
        // Print the sepatator
        Write(255);
        // Remaining space on display
        const int remaining_space = MAX_CHARS - index_length;
        // don't clear anything and don't print too many characters if name is long enough
        if (len > remaining_space) {
            empty_space_start = MAX_CHARS;
            len = remaining_space;
        } else {
            empty_space_start = len + index_length;
        }
        // Print line
        PrintN(file_data.data[pos], len);
    // If not showing indexes
    } else {
        empty_space_start = len;
        PrintN(file_data.data[pos], len);
    }
    // clear space after line end
    for (int i = empty_space_start; i < MAX_CHARS; i++)
        Write(' ');
}

void FileSelectionAt(int pos) {
    CursorOff();
    BlinkingOn();
    ClearDisplay();

    current_menu = FileSelection;
    lcd_col = 0;
    current_file = pos;
    
    // if at last line
    if (pos == AMOUNT_OF_FILES-1) {
        // print previous line at the top
        PrintFileName(pos-1, TOP_ROW);
        // print current line at the bottom
        PrintFileName(pos, BOTTOM_ROW);
        lcd_row = BOTTOM_ROW;
    // if before last line
    } else {
        // print current line at the top
        PrintFileName(pos, TOP_ROW);
        // print next line at the bottom
        PrintFileName(pos+1, BOTTOM_ROW);
        lcd_row = TOP_ROW;
    }
    // set final cursor position
    SetCursor(lcd_col, lcd_row);
}



void ExistingFileOperationsDefaults() {
    CursorOff();
    BlinkingOff();
    ClearDisplay();

    current_menu = ExistingFileOperations;
    selected_operation = FileOpen;
    show_indexes = 0;

    Print("Choose action:");
    lcd_col = 0;
    lcd_row = BOTTOM_ROW;
    SetCursor(lcd_col, lcd_row);
    Print("      Open     >");
}

void NewFileOperationsDefaults() {
    CursorOff();
    BlinkingOff();
    
    current_menu = NewFileOperations;
    selected_operation = FileCreate;
    show_indexes = 0;

    ClearDisplay();
    Print("Choose action:");
    lcd_col = 0;
    lcd_row = BOTTOM_ROW;
    SetCursor(lcd_col, lcd_row);
    Print("     Create    >");
}

void FileNameSelectionDefaults() {
    CursorOn();
    BlinkingOff();

    current_menu = FileNameSelection;
    show_indexes = 0;
    insert_mode = 0;
    // clear current name buffer
    for (int i = 0; i < LINE_SIZE; i++)
        new_name_buf[i] = 0xFF;
    new_name_len = 0;

    ClearDisplay();
    Print("Enter file name:");
    lcd_col = 0;
    lcd_row = BOTTOM_ROW;
    SetCursor(lcd_col, lcd_row);
}

void FileRenameDefaults() {
    CursorOn();
    BlinkingOff();

    current_menu = FileNameSelection;
    show_indexes = 0;
    // load current file name into the buffer
    memcpy(new_name_buf,
        &files_info.file_names[current_file],
        LINE_SIZE);
    new_name_len = files_info.name_lengths[current_file];
    
    ClearDisplay();
    Print("Enter file name:");
    SetCursor(0, BOTTOM_ROW);
    PrintN(new_name_buf, new_name_len);

    lcd_col = new_name_len;
    lcd_row = BOTTOM_ROW;
}

void TextEditorDefaults() {
    BlinkingOff();
    CursorOn();

    current_line = 0;
    current_menu = TextEditor;
    show_indexes = 0;
    insert_mode = 0;
    PrintDataLine(0, TOP_ROW);
    PrintDataLine(1, BOTTOM_ROW);

    lcd_col = 0;
    lcd_row = TOP_ROW;
    SetCursor(lcd_col, lcd_row);
}

void EditorExitPromptDefaults() {
    CursorOff();
    BlinkingOff();
    show_indexes = 0;
    current_menu = EditorExitPrompt;
    selected_operation = FileSave;

    ClearDisplay();
    Print("Choose action:");
    lcd_col = 0;
    lcd_row = BOTTOM_ROW;
    SetCursor(lcd_col, lcd_row);
    Print("      Save     >");
}

/*
    ---
    Adds a character to the line, does not handle multi-line operations
    ---
    first parameter 'chr' is the character to add
    second parameter 'line' is the line to be added into
    third parameter 'len' is pointer to line's length variable

    returns -1 or error and 0 on success
*/
int LineAddChar(char chr, char* line, int* len) {
    const int distance = DistanceBetweenCursorAndLineEnd(*len);

    // prevent creating gaps in memory and going outside of the buffer
    if (distance < -1 || (*len) >= LINE_SIZE)
        return -1;
     
    // if cursor is on a character
    if (distance > -1) {
        if (insert_mode == 0) {
            // shift data from the cursor forward by 1
            memmove(&line[lcd_col+1],
                &line[lcd_col],
                distance+1);
            // put new char at cursor position
            line[lcd_col] = chr;
            // print modified characters
            PrintN(&line[lcd_col], distance+2);
            SetCursor(lcd_col+1, lcd_row);
            (*len)++;
        } else {
            // put new char at cursor position
            line[lcd_col] = chr;
            Write(chr);
        }
    // if cursor is after last character
    } else {
        line[*len] = chr;
        Write(chr);
        (*len)++;
        
    }
    lcd_col++;
    

    return 0;
}

/*
    ---
    Removes a character at cursor's position, does not handle multi-line operationss
    ---
    first parameter 'line' is the line to be deleted from
    second parameter 'len' is pointer to line's length variable
*/
void LineDelete(char* line, int* len) {
    const int distance = DistanceBetweenCursorAndLineEnd(*len);
    // nothing to delete if cursor isn't pointing at name
    if (distance < 0)
        return;

    // if cursor is before last character
    if (distance > 0) {
        // shift characters after the cursor by 1 backwards (thus overwriting the char to remove)
        memmove(&line[lcd_col], &line[lcd_col+1], distance);
        // print modified characters
        PrintN(&line[lcd_col], distance);
    }
    // clear deleted (or shifted) character
    // note that this also covers distance = 0 scenario
    line[--(*len)] = 255;
    Write(' ');
    
    // move cursor to original position
    SetCursor(lcd_col, lcd_row);
}

/*
    ---
    Removes a character before cursor's position, does not handle multi-line operationss
    ---
    first parameter 'line' is the line to be deleted from
    second parameter 'len' is pointer to line's length variable
*/
void LineBackspace(char* line, int* len) {
    // it's the same as moving the cursor to the left by one and deleting that character
    if (lcd_col > 0) {
        lcd_col--;
        SetCursor(lcd_col, lcd_row);
        LineDelete(line, len);
    }
}

/*
    ---
    Adds a character to file_data, handles multi-line operations
    ---
    first parameter 'chr' is the character to add
*/
void EditorAddChar(char chr) {
    // When in insert mode
    if (insert_mode == 1) {
        // if at line end
        if (lcd_col == LINE_SIZE)
            // stop if inserting after file end
            if (current_line >= AMOUNT_OF_LINES-1)
                return;
            // go the the next line otherwise
            else
                ProcessArrowRight();
        // Let the single line function handle it, because inserting before line end won't create more lines
        LineAddChar(chr, file_data.data[current_line], &file_data.line_lengths[current_line]);
    // When not in insert mode
    } else {
        // check if there are any non-full lines
        int first_not_full_line = -1;
        for (int i = current_line; i < AMOUNT_OF_LINES; i++) {
            if (file_data.line_lengths[i] < LINE_SIZE) {
                first_not_full_line = i;
                break;
            }
        }
        // stop if all lines are full
        if (first_not_full_line == -1)
            return;

        // create pointers to current line and its' length
        char (*line)[LINE_SIZE] = &file_data.data[current_line];
        int *len = &file_data.line_lengths[current_line];

        // line number before all operations
        int original_line = current_line;
        // variable storing previous line's last character
        char last;
        // checking if cursor moves to next line after writing
        int moved_to_next_line = lcd_col >= LINE_SIZE;

        // ------------------------------ 
        // dealing with the first line
        // ------------------------------
        // if cursor doesen't move to the next line
        if (!moved_to_next_line) {
            // if line is not full
            if (*len < LINE_SIZE) {
                // let the single-line function handle it
                LineAddChar(chr, *line, len);
                return;
            // if line is full
            } else {
                // take line's last character and add the char passed by keyboard at cursor position
                (*len)--;
                last = (*line)[*len];
                LineAddChar(chr, *line, len);
                line++;
                len++;
            }
        // if cursor should move to the next line
        } else {
            // skip the first line
            current_line++;
            line++;
            len++;
            // last character of the first line was supposed to have the char passed by keyboard
            last = chr;
            // after writing a new character, cursor will be at the second position
            lcd_col = 1;
        
        }
        
        // ------------------------------------------
        // dealing with all upcoming full lines
        // shifting all of them right while putting their last characters
        // at the beginning of the next line
        // ------------------------------------------
        for (int i = original_line+1; i < first_not_full_line; i++) {
            // store next line's last character
            char new_last = (*line)[LINE_SIZE-1];
            // shift the line to the right
            memmove(&(*line)[1], &(*line)[0], LINE_SIZE-1);
            // add previous line's last character at the beginning of this one
            (*line)[0] = last;
            // store next line's last character
            last = new_last;
            line++;
            len++;
        }

        // ------------------------------------------ 
        // dealing with the last (non-full) line
        // ------------------------------------------
        // shift the line to the right
        memmove(&(*line)[1], &(*line)[0], *len);
        // add previous line's last character at the beginning of this one
        (*line)[0] = last;
        (*len)++;
        
        // ------------------------------------------
        // printing modified characters that were not updated in-place by LineAddChar
        // ------------------------------------------
        // if cursor moved to next line
        if (moved_to_next_line) {
            // print next line at the bottom row
            PrintDataLine(original_line+1, BOTTOM_ROW);

            // if cursor was at the bottom row before moving
            if (lcd_row = BOTTOM_ROW)
                // print the unmodified line at top row
                PrintDataLine(original_line, TOP_ROW);

            // move the cursor to bottom row
            lcd_row = BOTTOM_ROW;
        // if cursor didn't move to the next line and was at the top row
        } else if (lcd_row == TOP_ROW) {
            // only update the bottom row (top one was updated by LineAddChar)
            PrintDataLine(original_line+1, BOTTOM_ROW);
        }
        // if cursor didn't move to the next line and was at the bottom row, it was updated by LineAddChar

        // set final cursor position
        SetCursor(lcd_col, lcd_row);
    }
    
}

/*
    ---
    Deletes a character from file_data at cursor position, handles multi-line operations
    ---
*/
void EditorDelete() {
    // create pointers to current line and its' length
    char (*line)[LINE_SIZE] = &file_data.data[current_line];
    int *len = &file_data.line_lengths[current_line];

    // if cursor is on a character
    if (lcd_col < *len) {
        // delete without moving any lines
        LineDelete(*line, len);
    // if cursor is not on a character and it's not at the last line
    } else if (current_line < AMOUNT_OF_LINES-1) {
        int line_to_delete = current_line;
        // if line is not empty
        if (lcd_col > 0) {
            // stop if next line does not fit into current one
            if (*(len+1) > LINE_SIZE - *len) {
                return;
            // otherwise merge next line with current one
            } else {
                int amount_to_move = *(len+1);
                memmove(&(*line)[lcd_col],
                        *(line+1),
                        amount_to_move);
                (*len) += amount_to_move;
                // print merged characters
                PrintN(&(*line)[lcd_col], amount_to_move);
                // start deleting from the next line
                line_to_delete++;
                line++;
                len++;
            }
        // if line is empty
        } else {
            // print next line in its' place
            PrintDataLine(current_line+1, lcd_row);
        }
        // delete line by moving all upcoming lines up
        for (int i = line_to_delete; i < AMOUNT_OF_LINES-1; i++) {
            memmove(*line, *(line+1), LINE_SIZE);
            *(len) = *(len+1);
            line++;
            len++;
        }
        // clear last line (after moving last lines' data is uninitialized)
        for (int i = 0; i < LINE_SIZE; i++)
            (*line)[i] = 255;
        *len = 0;
        // if deleted at the top row
        if (lcd_row == TOP_ROW)
            // print new bottom row after moving lines up
            PrintDataLine(current_line+1, BOTTOM_ROW);
        // set final cursor position
        SetCursor(lcd_col, lcd_row);
    }
}

/*
    ---
    Deletes a character from file_data before cursor position, handles multi-line operations
    ---
*/
void EditorBackspace() {
    // create pointers to current line and its' length
    char (*line)[LINE_SIZE] = &file_data.data[current_line];
    int *len = &file_data.line_lengths[current_line];

    // delete in-place if cursor is not at line's beginning
    if (lcd_col > 0) {
        SetCursor(--lcd_col, lcd_row);
        LineDelete(*line, len);
    //if cursor is at line's beginning and not at first line
    } else if (current_line > 0) {
        int amount_to_move = *len;
        // if current line doesent fit into the previous one
        if (amount_to_move > LINE_SIZE - *(len-1)) {
            // go to previous line
            ProcessArrowLeft();
            line--;
            len--;
            // remove 1 character and stop
            LineBackspace(*line, len);
        // if current line fits into previous line
        } else {
            // merge current line with previous line
            memmove(&(*(line-1))[*(len-1)],
                    *line,
                    amount_to_move);
            // delete current line by moving all upcoming lines up
            for (int i = current_line; i < AMOUNT_OF_LINES-1; i++) {
                memmove(*line, *(line+1), LINE_SIZE);
                *len = *(len+1);
                line++;
                len++;
            }
            // clear last line (after moving its' data is uninitialized)
            for (int i = 0; i < LINE_SIZE; i++)
                (*line)[i] = 255;
            *len = 0;
            // if deleted line was at bottom row on the display
            if (lcd_row == BOTTOM_ROW) {
                // print new bottom row after moving lines up
                PrintDataLine(current_line, BOTTOM_ROW);
                lcd_col = 0;
            }
            // go to previous line
            ProcessArrowLeft();
            // increase line's length after merging
            file_data.line_lengths[current_line] += amount_to_move;
            // print merged data after line's end
            PrintN(&file_data.data[current_line][lcd_col], amount_to_move);
            SetCursor(lcd_col, lcd_row);
        }
    }
}

/*
    ---
    Moves characters in file_data after the cursor to the next line, handles multi-line operations
    ---
*/
void EditorEnter() {
    // check if there is space to create a new line
    int has_empty_line = -1;
    for (int i = current_line+1; i < AMOUNT_OF_LINES; i++) {
        if (file_data.line_lengths[i] == 0) {
            has_empty_line = 1;
            break;
        }
    }
    // stop if there are no empty lines
    if (has_empty_line == -1)
        return;

    // create pointers to last line and its' length
    char (*line)[LINE_SIZE] = &file_data.data[AMOUNT_OF_LINES-1];
    int *len = &file_data.line_lengths[AMOUNT_OF_LINES-1];

    // move lines up until it hits the newly created one
    // after this loop (*line) points at newly created line
    for (char (*new_line)[LINE_SIZE] = &file_data.data[current_line+1]; line > new_line; line--, len--) {
        memmove(*line, *(line-1), LINE_SIZE);
        *len = *(len-1);
    }
    // move data after cursor from current line into newly created (which is right after current line)
    int amount_to_move = *(len-1) - lcd_col;
    memmove(*line,
        &(*(line-1))[lcd_col],
        amount_to_move);
    // set newly created line's length
    *len = amount_to_move;
    // clear uninitialized data from newly created line
    for (int i = amount_to_move; i < LINE_SIZE; i++)
        (*line)[i] = 255;
    // go to current line
    line--;
    len--;
    // clear moved characters from current line
    for (int i = lcd_col; i < *len; i++) {
        (*line)[i] = 255;
        Write(' ');
    }
    // decrease current line's length after moving
    *len -= amount_to_move;
    // go to next line on screen and print newly created line in it 
    ProcessArrowRight();
    PrintDataLine(current_line, lcd_row);
    // set final cursor position
    lcd_col = 0;
    SetCursor(lcd_col, lcd_row);
}