# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Config VSCode Extension (não editar) ===
if (WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ============================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Puxa o SDK (precisa vir antes do project)
include(pico_sdk_import.cmake)

project(Codigo_Desafio_Extra C CXX ASM)
pico_sdk_init()

# Executável principal
add_executable(Codigo_Desafio_Extra Codigo_Desafio_Extra.c )

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/pico-ssd1306-main)

pico_set_program_name(Codigo_Desafio_Extra "Codigo_Desafio_Extra")
pico_set_program_version(Codigo_Desafio_Extra "0.1")

# Habilita stdio via USB (ou UART se preferir)
pico_enable_stdio_uart(Codigo_Desafio_Extra 0)
pico_enable_stdio_usb(Codigo_Desafio_Extra 1)

# Linka bibliotecas necessárias
target_link_libraries(Codigo_Desafio_Extra
    pico_stdlib
    hardware_gpio
    hardware_adc
    cyw43_driver
    pico-ssd1306
    pico_cyw43_arch_lwip_threadsafe_background
)

# Include do LWIP e diretórios locais
target_include_directories(Codigo_Desafio_Extra PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/src/rp2_common/cmsis/include/lwip/apps
    # depois vêm os includes genéricos do LWIP
    ${PICO_SDK_PATH}/lib/lwip/src/include
    ${PICO_SDK_PATH}/lib/lwip/src/include/arch
    ${PICO_SDK_PATH}/lib/lwip/src/include/lwip
    ${CMAKE_CURRENT_LIST_DIR}/libs/pico-ssd1306-main
)

# Adiciona o HTTP server e o filesystem do LWIP
target_sources(Codigo_Desafio_Extra PRIVATE
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/httpd.c
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
)

target_compile_definitions(Codigo_Desafio_Extra PRIVATE
    PBUF_POOL_SIZE=64
    MEM_SIZE=16000
    TCP_SND_BUF=8192
    TCP_WND=8192
)

# Gera UF2, map, etc.
pico_add_extra_outputs(Codigo_Desafio_Extra)
