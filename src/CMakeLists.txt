
set(APP main)

add_executable(${APP}
    main.c
)

# Use file with persistent storage configured
pico_set_linker_script(${APP}
    ${CMAKE_SOURCE_DIR}/memmap_custom.ld
)

pico_add_extra_outputs(${APP})

target_link_libraries(${APP}
    files
    hid
    editor
    pico_stdlib
)

target_include_directories(${APP} PUBLIC
    ${FILES_LIB_INCLUDE}
    ${EDITOR_LIB_INCLUDE}
    ${HID_LIB_INCLUDE}
    ${SRC_INCLUDE}
)

