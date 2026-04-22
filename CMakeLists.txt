cmake_minimum_required(VERSION 4.1.2)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "aarch64")
set(DEVKITPRO "$ENV{DEVKITPRO}")
set(LIBNX "${DEVKITPRO}/libnx")
set(PORTLIBS_PATH "${DEVKITPRO}/portlibs")
set(PORTLIBS "${PORTLIBS_PATH}/switch")

list(APPEND CMAKE_PROGRAM_PATH  "${DEVKITPRO}/tools/bin")
list(APPEND CMAKE_PROGRAM_PATH  "${DEVKITPRO}/devkitA64/bin")
set(CMAKE_C_COMPILER "aarch64-none-elf-gcc")
set(CMAKE_CXX_COMPILER "aarch64-none-elf-g++")

project(SwitchPost)

set(APP_TITLE "SwitchPost")
set(APP_AUTHOR "ameOverflow")
set(APP_VERSION "1.0")

set(SOURCES
        lib/qrcodegen.c
        source/main.cpp
        source/Helpers.cpp
        source/SceneManager.cpp
        source/MusicManager.cpp
        source/SceneOptions.cpp
        source/SceneTitle.cpp
        source/SceneIntro.cpp
        source/SceneMain.cpp
        source/SceneDebug.cpp
        source/Request.cpp
        source/InpostAPI.cpp
        source/SceneCredits.cpp
        source/AssetLoader.cpp
        source/Config.cpp
        source/Config.h
        source/SceneTutorial.cpp
        source/SceneReloadMain.cpp
        source/SceneReloadMain.h
)

add_executable(SwitchPost ${SOURCES})
set_target_properties(SwitchPost PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

set(ARCH "-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE")

target_include_directories(SwitchPost PRIVATE ${LIBNX}/include ${PORTLIBS}/include ./source ./include ${PROJECT_BINARY_DIR})
target_link_directories(SwitchPost PRIVATE ${LIBNX}/lib ${PORTLIBS}/lib ./lib)
target_link_libraries(SwitchPost PRIVATE curl z raylib EGL GLESv2 glapi drm_nouveau nx m)

target_compile_options(SwitchPost PRIVATE -fno-rtti -fno-exceptions -fPIE)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_COMPILE_FLAGS "${CMAKE_C_COMPILE_FLAGS} -O2 -Wall -Wno-narrowing -Wno-error=narrowing -ffunction-sections ${ARCH}")
    set(CMAKE_CXX_COMPILE_FLAGS  "${CMAKE_CXX_COMPILE_FLAGS} -O2 -Wall -Wno-narrowing -Wno-error=narrowing -ffunction-sections ${ARCH} -fno-rtti -fno-exceptions")
    set(CMAKE_C_LINK_FLAGS  "${CMAKE_C_LINK_FLAGS} -s -O2 ${ARCH} -specs=${LIBNX}/switch.specs")
    set(CMAKE_CXX_LINK_FLAGS  "${CMAKE_CXX_LINK_FLAGS} -s -O2 ${ARCH} -specs=${LIBNX}/switch.specs")
    target_compile_definitions(SwitchPost PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO RELEASE)
else()
    set(APP_VERSION "${APP_VERSION}+dev")
    set(CMAKE_C_COMPILE_FLAGS "${CMAKE_C_COMPILE_FLAGS} -g3 -O0 -Wall -Wno-narrowing -Wno-error=narrowing -ffunction-sections ${ARCH}")
    set(CMAKE_CXX_COMPILE_FLAGS  "${CMAKE_CXX_COMPILE_FLAGS} -g3 -O0 -Wall -Wno-narrowing -Wno-error=narrowing -ffunction-sections ${ARCH} -fno-rtti -fno-exceptions")
    set(CMAKE_C_LINK_FLAGS  "${CMAKE_C_LINK_FLAGS} -g3 -O0 ${ARCH} -specs=${LIBNX}/switch.specs -Wl,-Map,${TARGET}.map")
    set(CMAKE_CXX_LINK_FLAGS  "${CMAKE_CXX_LINK_FLAGS} -g3 -O0 ${ARCH} -specs=${LIBNX}/switch.specs -Wl,-Map,${TARGET}.map")
    target_compile_definitions(SwitchPost PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE DEBUG)
endif()

target_compile_definitions(SwitchPost PRIVATE
        __SWITCH__
        SPDLOG_NO_EXCEPTIONS
        SPDLOG_NO_TLS
        SPDLOG_CLOCK_COARSE
        SPDLOG_NO_TZ_OFFSET
        ASSETS_PATH="assets"
        _GNU_SOURCE
        BUILD_TYPE="${CMAKE_BUILD_TYPE}"
        APP_TITLE="${APP_TITLE}"
        APP_VERSION="${APP_VERSION}"
        BASE_URL="https://api-inmobile-pl.easypack24.net"
)

add_custom_command(TARGET SwitchPost POST_BUILD
        COMMAND nacptool --create ${APP_TITLE} ${APP_AUTHOR} ${APP_VERSION} $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp
        COMMAND elf2nro $<TARGET_FILE:SwitchPost> $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nro --nacp=$<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp --romfsdir=${PROJECT_SOURCE_DIR}/assets --icon=${PROJECT_SOURCE_DIR}/assets/sprites/inpost_logo.jpg
        COMMENT "Converting ELF to NRO"
)