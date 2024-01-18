
set(LCD_LIB lcd) 

add_library(${LCD_LIB} STATIC lcd.c)

target_link_libraries(${LCD_LIB} pico_stdlib hardware_i2c hardware_adc)
