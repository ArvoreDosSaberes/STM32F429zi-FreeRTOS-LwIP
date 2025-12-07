#############################################################################################################################
# file:  CMakeLists.txt
# brief: Template "CMakeLists.txt" for building of executables and static libraries.
#
# usage: Edit "VARIABLES"-section to suit project requirements.
#        For debug build:
#          cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake  -S ./ -B Debug -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
#          make -C Debug VERBOSE=1 -j
#        For release build:
#          cmake -DCMAKE_TOOLCHAIN_FILE=cubeide-gcc.cmake  -S ./ -B Release -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
#          make -C Release VERBOSE=1 -j
#############################################################################################################################
cmake_minimum_required(VERSION 3.20)

#############################################################################################################################
# Carregamento do arquivo de configuração de ambiente (env.cmake)
#############################################################################################################################
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/env.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/env.cmake")
else()
    message(FATAL_ERROR "Arquivo env.cmake não encontrado! Copie env.cmake.example para env.cmake e configure as variáveis.")
endif()

#############################################################################################################################
# Validação das variáveis obrigatórias do env.cmake
#############################################################################################################################

# Valores padrão permitidos
if(NOT DEFINED ENV{LOG_LEVEL} OR "$ENV{LOG_LEVEL}" STREQUAL "")
    set(ENV{LOG_LEVEL} "1")
    message(STATUS "LOG_LEVEL não definido. Usando valor padrão: 1")
endif()

if(NOT DEFINED ENV{MQTT_BROKER} OR "$ENV{MQTT_BROKER}" STREQUAL "")
    set(ENV{MQTT_BROKER} "mqtt.rapport.tec.br")
    message(STATUS "MQTT_BROKER não definido. Usando valor padrão: mqtt.rapport.tec.br")
endif()

if(NOT DEFINED ENV{ENABLE_STACK_WATERMARK} OR "$ENV{ENABLE_STACK_WATERMARK}" STREQUAL "")
    set(ENV{ENABLE_STACK_WATERMARK} "0")
    message(STATUS "ENABLE_STACK_WATERMARK não definido. Usando valor padrão: 0")
endif()

# Variáveis obrigatórias (sem valor padrão)
if(NOT DEFINED ENV{MQTT_BROKER_PORT} OR "$ENV{MQTT_BROKER_PORT}" STREQUAL "")
    message(FATAL_ERROR "MQTT_BROKER_PORT não definido no env.cmake! Esta variável é obrigatória.")
endif()

if(NOT DEFINED ENV{MQTT_CLIENT_ID} OR "$ENV{MQTT_CLIENT_ID}" STREQUAL "")
    message(FATAL_ERROR "MQTT_CLIENT_ID não definido no env.cmake! Esta variável é obrigatória.")
endif()

if(NOT DEFINED ENV{MQTT_BASE_TOPIC} OR "$ENV{MQTT_BASE_TOPIC}" STREQUAL "")
    message(FATAL_ERROR "MQTT_BASE_TOPIC não definido no env.cmake! Esta variável é obrigatória.")
endif()

if(NOT DEFINED ENV{MQTT_RACK_NUMBER} OR "$ENV{MQTT_RACK_NUMBER}" STREQUAL "")
    message(FATAL_ERROR "MQTT_RACK_NUMBER não definido no env.cmake! Esta variável é obrigatória.")
endif()

# MQTT_USERNAME e MQTT_PASSWORD são opcionais (podem ser "" ou não definidos)
if(NOT DEFINED ENV{MQTT_USERNAME})
    set(ENV{MQTT_USERNAME} "")
endif()
if(NOT DEFINED ENV{MQTT_PASSWORD})
    set(ENV{MQTT_PASSWORD} "")
endif()

# Variáveis opcionais com valores padrão
if(NOT DEFINED ENV{MQTT_PUBLISH_INTERVAL_MS} OR "$ENV{MQTT_PUBLISH_INTERVAL_MS}" STREQUAL "")
    set(ENV{MQTT_PUBLISH_INTERVAL_MS} "5000")
endif()

if(NOT DEFINED ENV{MQTT_QOS} OR "$ENV{MQTT_QOS}" STREQUAL "")
    set(ENV{MQTT_QOS} "1")
endif()

if(NOT DEFINED ENV{MQTT_KEEP_ALIVE_S} OR "$ENV{MQTT_KEEP_ALIVE_S}" STREQUAL "")
    set(ENV{MQTT_KEEP_ALIVE_S} "60")
endif()

if(NOT DEFINED ENV{RACK_VERSION} OR "$ENV{RACK_VERSION}" STREQUAL "")
    set(ENV{RACK_VERSION} "0.0.1")
endif()

# Log das configurações carregadas
message(STATUS "=== Configurações MQTT ===")
message(STATUS "  MQTT_BROKER: $ENV{MQTT_BROKER}")
message(STATUS "  MQTT_BROKER_PORT: $ENV{MQTT_BROKER_PORT}")
message(STATUS "  MQTT_CLIENT_ID: $ENV{MQTT_CLIENT_ID}")
message(STATUS "  MQTT_USERNAME: $ENV{MQTT_USERNAME}")
message(STATUS "  MQTT_BASE_TOPIC: $ENV{MQTT_BASE_TOPIC}")
message(STATUS "  MQTT_RACK_NUMBER: $ENV{MQTT_RACK_NUMBER}")
message(STATUS "  MQTT_PUBLISH_INTERVAL_MS: $ENV{MQTT_PUBLISH_INTERVAL_MS}")
message(STATUS "  LOG_LEVEL: $ENV{LOG_LEVEL}")
message(STATUS "==========================")

###################### CONSTANTS ######################################
set (PROJECT_TYPE_EXECUTABLE          "exe")
set (PROJECT_TYPE_STATIC_LIBRARY      "static-lib")
set (MCPU_CORTEX_M0				      "-mcpu=cortex-m0")
set (MCPU_CORTEX_M0PLUS				  "-mcpu=cortex-m0plus")
set (MCPU_CORTEX_M3				      "-mcpu=cortex-m3")
set (MCPU_CORTEX_M4				      "-mcpu=cortex-m4")
set (MCPU_CORTEX_M7				      "-mcpu=cortex-m7")
set (MCPU_CORTEX_M33				  "-mcpu=cortex-m33")
set (MCPU_CORTEX_M55				  "-mcpu=cortex-m55")
set (MCPU_CORTEX_M85				  "-mcpu=cortex-m85")
set (MFPU_FPV4_SP_D16                 "-mfpu=fpv4-sp-d16")
set (MFPU_FPV4                        "-mfpu=vfpv4")
set (MFPU_FPV5_D16                    "-mfpu=fpv5-d16")
set (RUNTIME_LIBRARY_REDUCED_C        "--specs=nano.specs")
set (RUNTIME_LIBRARY_STD_C            "")
set (RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")
set (RUNTIME_LIBRARY_SYSCALLS_NONE    "")
set (MFLOAT_ABI_SOFTWARE              "-mfloat-abi=soft")
set (MFLOAT_ABI_HARDWARE              "-mfloat-abi=hard")
set (MFLOAT_ABI_MIX                   "-mfloat-abi=softfp")
#######################################################################

###################### VARIABLES ######################################
set (PROJECT_NAME             "STM32-F429zi-http")
set (PROJECT_TYPE             "exe")
set (LINKER_SCRIPT            "../STM32F429ZITX_FLASH.ld")
set (MCPU                     "-mcpu=Cortex-M4")
set (MFPU                 "-mfpu=fpv4-sp-d16")
set (MFLOAT_ABI               "-mfloat-abi=hard")
set (RUNTIME_LIBRARY          "--specs=nano.specs")
set (RUNTIME_LIBRARY_SYSCALLS "--specs=nosys.specs")


set (PROJECT_SOURCES
	# LIST SOURCE FILES HERE
	Startup/startup_stm32f429zitx.s
	Sources/main.c
	Sources/syscalls.c
	Sources/sysmem.c
	Sources/ethernetif.c
	Sources/sys_arch.c
	Sources/httpserver.c
	Sources/webpages.c
	Sources/system_config.c
	Sources/mqtt_service.c
	Sources/temperature_sensor.c
	Sources/fan_controller.c
	)

set (PROJECT_DEFINES
	# LIST COMPILER DEFINITIONS HERE
	USE_HAL_DRIVER
	STM32F429xx
	# Definições do env.cmake para MQTT
	MQTT_BROKER="$ENV{MQTT_BROKER}"
	MQTT_BROKER_PORT=$ENV{MQTT_BROKER_PORT}
	MQTT_CLIENT_ID="$ENV{MQTT_CLIENT_ID}"
	MQTT_USERNAME="$ENV{MQTT_USERNAME}"
	MQTT_PASSWORD="$ENV{MQTT_PASSWORD}"
	MQTT_BASE_TOPIC="$ENV{MQTT_BASE_TOPIC}"
	MQTT_RACK_NUMBER="$ENV{MQTT_RACK_NUMBER}"
	MQTT_PUBLISH_INTERVAL_MS=$ENV{MQTT_PUBLISH_INTERVAL_MS}
	MQTT_QOS=$ENV{MQTT_QOS}
	MQTT_KEEP_ALIVE_S=$ENV{MQTT_KEEP_ALIVE_S}
	# Outras definições do env.cmake
	LOG_LEVEL=$ENV{LOG_LEVEL}
	RACK_VERSION="$ENV{RACK_VERSION}"
	ENABLE_STACK_WATERMARK=$ENV{ENABLE_STACK_WATERMARK}
    )

set (PROJECT_INCLUDES
	# LIST INCLUDE DIRECTORIES HERE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/arch
    )

#############################################################################################################################
# Configuração do STM32CubeF4 HAL
#############################################################################################################################
include(FetchContent)
FetchContent_Declare(stm32cubef4
  GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF4.git
  GIT_TAG v1.25.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)

FetchContent_GetProperties(stm32cubef4)
if(NOT stm32cubef4_POPULATED)
  FetchContent_Populate(stm32cubef4)
endif()

set(STM32CUBEF4_DIR ${stm32cubef4_SOURCE_DIR})

# Arquivos HAL necessários
set(STM32_HAL_SRCS
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
)

# Criar biblioteca HAL
add_library(stm32_hal STATIC ${STM32_HAL_SRCS})

target_include_directories(stm32_hal PUBLIC
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
)

target_compile_definitions(stm32_hal PUBLIC
    USE_HAL_DRIVER
    STM32F429xx
)

# Suprimir warnings do código HAL da ST (que não controlamos)
target_compile_options(stm32_hal PRIVATE -Wno-unused-parameter -Wno-error)

############ MODIFY ACCORDING TO REQUIREMENTS) ########################

#######################################################################

################## PROJECT SETUP ######################################
project(${PROJECT_NAME})
enable_language(ASM)

if (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_EXECUTABLE})
  add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
  add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>)
elseif (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_STATIC_LIBRARY})
  add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

add_compile_definitions (${PROJECT_DEFINES})
include_directories (${PROJECT_INCLUDES})

set (CMAKE_EXECUTABLE_SUFFIX ".elf")
set (CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set (CMAKE_C_FLAGS "${MCPU} -std=gnu11 ${MFPU} ${MFLOAT_ABI} ${RUNTIME_LIBRARY} -mthumb -Wall -Werror")
set (CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} ${RUNTIME_LIBRARY_SYSCALLS} -Wl,-Map=test.map -Wl,--gc-sections -static -Wl,--start-group -lc -lm -Wl,--end-group")
set (CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

# Baixando o FreeRTOS do GitHub
include(FetchContent)
FetchContent_Declare(FreeRTOS
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
  GIT_TAG V11.2.0
)

# Configuração do FreeRTOS - target obrigatório para definir o diretório do FreeRTOSConfig.h
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
)
target_compile_definitions(freertos_config
  INTERFACE
    projCOVERAGE_TEST=0
)

# Definir o port do FreeRTOS para Cortex-M4F (com FPU)
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")

# Definir o heap implementation (heap_4 é recomendado para a maioria dos casos)
set(FREERTOS_HEAP "4" CACHE STRING "")

FetchContent_MakeAvailable(FreeRTOS)

# Linkar FreeRTOS ao projeto principal
target_link_libraries(${PROJECT_NAME} freertos_kernel)

#############################################################################################################################
# Configuração do LwIP
#############################################################################################################################

# Baixando o LwIP do repositório oficial
FetchContent_Declare(lwip
  GIT_REPOSITORY https://git.savannah.nongnu.org/git/lwip.git
  GIT_TAG STABLE-2_2_0_RELEASE
)

FetchContent_GetProperties(lwip)
if(NOT lwip_POPULATED)
  FetchContent_Populate(lwip)
endif()

# Criar biblioteca LwIP
set(LWIP_DIR ${lwip_SOURCE_DIR})

# Arquivos fonte do core LwIP
set(LWIP_CORE_SRCS
    ${LWIP_DIR}/src/core/init.c
    ${LWIP_DIR}/src/core/def.c
    ${LWIP_DIR}/src/core/dns.c
    ${LWIP_DIR}/src/core/inet_chksum.c
    ${LWIP_DIR}/src/core/ip.c
    ${LWIP_DIR}/src/core/mem.c
    ${LWIP_DIR}/src/core/memp.c
    ${LWIP_DIR}/src/core/netif.c
    ${LWIP_DIR}/src/core/pbuf.c
    ${LWIP_DIR}/src/core/raw.c
    ${LWIP_DIR}/src/core/stats.c
    ${LWIP_DIR}/src/core/sys.c
    ${LWIP_DIR}/src/core/altcp.c
    ${LWIP_DIR}/src/core/altcp_alloc.c
    ${LWIP_DIR}/src/core/altcp_tcp.c
    ${LWIP_DIR}/src/core/tcp.c
    ${LWIP_DIR}/src/core/tcp_in.c
    ${LWIP_DIR}/src/core/tcp_out.c
    ${LWIP_DIR}/src/core/timeouts.c
    ${LWIP_DIR}/src/core/udp.c
)

# Arquivos IPv4
set(LWIP_IPV4_SRCS
    ${LWIP_DIR}/src/core/ipv4/acd.c
    ${LWIP_DIR}/src/core/ipv4/autoip.c
    ${LWIP_DIR}/src/core/ipv4/dhcp.c
    ${LWIP_DIR}/src/core/ipv4/etharp.c
    ${LWIP_DIR}/src/core/ipv4/icmp.c
    ${LWIP_DIR}/src/core/ipv4/igmp.c
    ${LWIP_DIR}/src/core/ipv4/ip4_frag.c
    ${LWIP_DIR}/src/core/ipv4/ip4.c
    ${LWIP_DIR}/src/core/ipv4/ip4_addr.c
)

# Arquivos de API
set(LWIP_API_SRCS
    ${LWIP_DIR}/src/api/api_lib.c
    ${LWIP_DIR}/src/api/api_msg.c
    ${LWIP_DIR}/src/api/err.c
    ${LWIP_DIR}/src/api/if_api.c
    ${LWIP_DIR}/src/api/netbuf.c
    ${LWIP_DIR}/src/api/netdb.c
    ${LWIP_DIR}/src/api/netifapi.c
    ${LWIP_DIR}/src/api/sockets.c
    ${LWIP_DIR}/src/api/tcpip.c
)

# Arquivos netif
set(LWIP_NETIF_SRCS
    ${LWIP_DIR}/src/netif/ethernet.c
)

# Arquivos de aplicação (httpd)
set(LWIP_APPS_SRCS
    ${LWIP_DIR}/src/apps/http/httpd.c
    ${LWIP_DIR}/src/apps/http/fs.c
)

# Arquivos do cliente MQTT
set(LWIP_MQTT_SRCS
    ${LWIP_DIR}/src/apps/mqtt/mqtt.c
)

# Criar biblioteca estática LwIP
add_library(lwip_lib STATIC
    ${LWIP_CORE_SRCS}
    ${LWIP_IPV4_SRCS}
    ${LWIP_API_SRCS}
    ${LWIP_NETIF_SRCS}
    ${LWIP_APPS_SRCS}
    ${LWIP_MQTT_SRCS}
)

# Incluir diretórios do LwIP
target_include_directories(lwip_lib PUBLIC
    ${LWIP_DIR}/src/include
    ${LWIP_DIR}/src/include/lwip/apps
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/arch
    ${freertos_SOURCE_DIR}/include
    ${freertos_SOURCE_DIR}/portable/GCC/ARM_CM4F
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Include
)

# Flags de compilação para LwIP
target_compile_definitions(lwip_lib PUBLIC
    LWIP_DEBUG=0
)

# Linkar LwIP ao projeto
target_link_libraries(${PROJECT_NAME} lwip_lib)

# Linkar STM32 HAL ao projeto
target_link_libraries(${PROJECT_NAME} stm32_hal)

# Adicionar includes do HAL ao projeto principal
target_include_directories(${PROJECT_NAME} PUBLIC
    ${STM32CUBEF4_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${STM32CUBEF4_DIR}/Drivers/CMSIS/Include
)