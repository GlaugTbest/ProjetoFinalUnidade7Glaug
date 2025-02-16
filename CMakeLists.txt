# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(testeHttp2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Adiciona todos os arquivos .c da pasta src automaticamente
file(GLOB SOURCES "src/*.c")

# Adiciona os arquivos da biblioteca SSD1306
file(GLOB LIB_SOURCES "lib/*.c")

# Inclui o arquivo ssd1306.c explicitamente (caminho correto)
set(SSD1306_SRC "lib/ssd1306.c")

# Criação do executável e inclusão das bibliotecas
add_executable(testeHttp2 testeHttp2.c ${SOURCES} ${LIB_SOURCES} ${SSD1306_SRC})

pico_set_program_name(testeHttp2 "testeHttp2")
pico_set_program_version(testeHttp2 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(testeHttp2 1)
pico_enable_stdio_usb(testeHttp2 1)

# Add the standard library to the build
target_link_libraries(testeHttp2
    pico_stdlib
    hardware_i2c    # Adiciona suporte a I2C para o SSD1306
    hardware_pwm    # Adiciona suporte a PWM
    hardware_pio
    hardware_clocks
    hardware_uart
    pico_cyw43_arch_lwip_threadsafe_background
)

# Add the standard include files to the build
target_include_directories(testeHttp2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    lib   # Garante que a biblioteca SSD1306 seja encontrada
)

# Add any user requested libraries
target_link_libraries(testeHttp2 
    pico_cyw43_arch_lwip_threadsafe_background
)

# Gera arquivos extras (UF2, binário, etc.)
pico_add_extra_outputs(testeHttp2)