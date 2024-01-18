set(FILE_LIB files) 

add_library(${FILE_LIB} STATIC files.c)

target_link_libraries(${FILE_LIB} pico_stdlib lcd hardware_flash)

target_include_directories(${FILE_LIB} PRIVATE
    ${LCD_LIB_INCLUDE}
)
