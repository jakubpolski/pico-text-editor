set(EDITOR_LIB editor) 

add_library(${EDITOR_LIB} STATIC editor.c)

target_link_libraries(${EDITOR_LIB} files hid lcd pico_stdlib)

target_include_directories(${EDITOR_LIB} PRIVATE
    ${LCD_LIB_INCLUDE}
    ${HID_LIB_INCLUDE}
    ${FILES_LIB_INCLUDE}
    ${EDITOR_LIB_INCLUDE}
    ${SRC_INCLUDE}
)
