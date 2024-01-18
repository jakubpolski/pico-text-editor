
set(HID_LIB hid) 

add_library(${HID_LIB} STATIC hid_app.c hid_keyboard.c) 

target_link_libraries(${HID_LIB} pico_stdlib tinyusb_host tinyusb_board)

target_include_directories(${HID_LIB} PRIVATE
    ${EDITOR_LIB_INCLUDE}
    ${HID_LIB_INCLUDE}
    ${SRC_INCLUDE}
)
