cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(analog_keyboard_humanizer C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# PIO USB path
if(DEFINED PICO_PIO_USB_PATH)
    set(PICO_PIO_USB_SRCS
        ${PICO_PIO_USB_PATH}/src/pio_usb.c
        ${PICO_PIO_USB_PATH}/src/pio_usb_host.c
        ${PICO_PIO_USB_PATH}/src/pio_usb_device.c
        ${PICO_PIO_USB_PATH}/src/usb_crc.c
    )
    set(PICO_PIO_USB_INC ${PICO_PIO_USB_PATH}/src)
endif()

# tusb_xinput path
if(DEFINED TUSB_XINPUT_PATH)
    set(TUSB_XINPUT_SRCS
        ${TUSB_XINPUT_PATH}/xinput_host.c
    )
    set(TUSB_XINPUT_INC ${TUSB_XINPUT_PATH})
endif()

pico_sdk_init()

add_executable(analog_keyboard_humanizer
    src/main.c
    src/humanizer.c
    src/storage.c
    src/xinput_device.c
    src/usb_host.c
    src/usb_descriptors.c
    ${PICO_PIO_USB_SRCS}
    ${TUSB_XINPUT_SRCS}
)

target_include_directories(analog_keyboard_humanizer PRIVATE
    src
    ${PICO_SDK_PATH}/lib/tinyusb/src
    ${PICO_SDK_PATH}/lib/tinyusb/hw
    ${PICO_SDK_PATH}/lib/tinyusb/src/class/xinput
    ${PICO_PIO_USB_INC}
    ${TUSB_XINPUT_INC}
)

target_link_libraries(analog_keyboard_humanizer
    pico_stdlib
    pico_multicore
    hardware_flash
    hardware_sync
    hardware_pio
    hardware_dma
    hardware_gpio
    tinyusb_device
    tinyusb_host
    tinyusb_board
    pico_unique_id
)

target_compile_definitions(analog_keyboard_humanizer PRIVATE
    PICO_DEFAULT_UART=0
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    # Adafruit Feather RP2040 USB Host
    PIO_USB_DP_PIN=16
    USB_HOST_5V_POWER_PIN=18
    CFG_TUH_RPI_PIO_USB=1
    # Enable XInput host driver
    CFG_TUH_XINPUT=1
    FIRMWARE_VERSION_MAJOR=1
    FIRMWARE_VERSION_MINOR=0
    FIRMWARE_VERSION_PATCH=0
)

pico_enable_stdio_usb(analog_keyboard_humanizer 0)
pico_enable_stdio_uart(analog_keyboard_humanizer 1)
pico_add_extra_outputs(analog_keyboard_humanizer)
