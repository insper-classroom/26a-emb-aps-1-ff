add_executable(pico_emb 
	main.c
	genius_jogo.c
	genius_lcd.c
	genius_som.c
)

set_target_properties(pico_emb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(../tft_lcd_ili9341)

# pull in common dependencies
target_link_libraries(pico_emb pico_stdlib hardware_pwm hardware_spi ili9341 gfx touch_resistive hardware_gpio hardware_clocks)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_emb)
