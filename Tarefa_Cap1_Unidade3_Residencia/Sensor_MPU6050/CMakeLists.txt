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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Sensor_MPU6050 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Define os arquivos fonte do projeto
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" # Inclui todos os .c da pasta src
)

# Adiciona o executável (TARGET) Sensor_MPU6050
add_executable(Sensor_MPU6050 ${PROJECT_SOURCES})

# AGORA, o target "Sensor_MPU6050" existe e podemos configurar suas propriedades.

# Define as pastas de inclusão para seus próprios headers e bibliotecas
target_include_directories(Sensor_MPU6050 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc # Adicionado para incluir os headers em 'inc'
    # Esta linha abaixo está correta se os headers do ssd1306 estiverem diretamente nesta pasta.
    # Se os headers estiverem em um subdiretório como 'libs/pico-ssd1306-main/src', ajuste o caminho.
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/pico-ssd1306-main
)

# Adiciona a subdiretório da biblioteca ssd1306
# Isso geralmente cria um target 'pico-ssd1306' que você pode vincular
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/pico-ssd1306-main)

pico_set_program_name(Sensor_MPU6050 "Sensor_MPU6050")
pico_set_program_version(Sensor_MPU6050 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Sensor_MPU6050 0)
pico_enable_stdio_usb(Sensor_MPU6050 1)

# Vincular as bibliotecas necessárias
target_link_libraries(Sensor_MPU6050 PUBLIC
    pico_stdlib
    hardware_i2c      # Para I2C (usado pelo MPU6050 e Display)
    hardware_pwm      # Para PWM (usado pelo Servo)
    pico-ssd1306      # Biblioteca do display (target criado por add_subdirectory)
    m                 # Biblioteca matemática para funções como atan2f, sqrtf, fabsf, M_PI
)

pico_add_extra_outputs(Sensor_MPU6050)