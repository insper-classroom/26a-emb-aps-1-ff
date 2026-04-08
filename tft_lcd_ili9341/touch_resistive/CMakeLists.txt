add_library(touch_resistive
        touch_resistive.c
        )

target_include_directories(touch_resistive PUBLIC
	.
)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(touch_resistive pico_stdlib hardware_adc)

