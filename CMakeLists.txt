cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(analog_keyboard_humanizer C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# PIO USB requires running from RAM for timing accuracy
set(PICO_PIO_USB_DP_PIN 20)  # RP2350-USB-A USB-A D+ pin

add_executable(analog_keyboard_humanizer
    src/main.c
    src/humanizer.c
    src/storage.c
    src/xinput_device.c
    src/usb_host.c
    src/usb_descriptors.c
)

target_include_directories(analog_keyboard_humanizer PRIVATE
    src
    ${PICO_SDK_PATH}/lib/tinyusb/src
    ${PICO_SDK_PATH}/lib/tinyusb/hw
)

# Link required libraries
target_link_libraries(analog_keyboard_humanizer
    pico_stdlib
    pico_multicore
    hardware_flash
    hardware_sync
    tinyusb_device
    tinyusb_host
    tinyusb_board
    pico_unique_id
)

# PIO USB for host port
target_link_libraries(analog_keyboard_humanizer
    tinyusb_piohost
)

# USB-A pins for RP2350-USB-A board
target_compile_definitions(analog_keyboard_humanizer PRIVATE
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    # PIO USB host on USB-A port
    PIO_USB_DP_PIN=20
    PIO_USB_USE_TINYUSB=1
    # Run USB host on core 1
    USB_HOST_CORE=1
    # Firmware version
    FIRMWARE_VERSION_MAJOR=1
    FIRMWARE_VERSION_MINOR=0
    FIRMWARE_VERSION_PATCH=0
)

# Enable USB serial for config tool communication
pico_enable_stdio_usb(analog_keyboard_humanizer 0)
pico_enable_stdio_uart(analog_keyboard_humanizer 1)

# Generate UF2
pico_add_extra_outputs(analog_keyboard_humanizer)

# Print memory usage
pico_set_binary_type(analog_keyboard_humanizer copy_to_ram)
