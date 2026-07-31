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
set(CMAKE_TOOLCHAIN_FILE "${DEVKITPRO}/cmake/Switch.cmake")

project(SwitchPost)

set(APP_TITLE "SwitchPost")
set(APP_AUTHOR "ameOverflow")
set(APP_VERSION "1.2.2-dev")

set(SOURCES
        source/qrcodegen.c
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
        source/InPostAPI.cpp
        source/SceneCredits.cpp
        source/AssetLoader.cpp
        source/Config.cpp
        source/SceneTutorial.cpp
        source/SceneError.cpp
        source/SceneLoading.cpp
        source/i18n.cpp
        source/SoundManager.cpp
)
set(PLATFORM "NX" CACHE STRING "Target platform for raylib" FORCE)

# set stuffs required by switchpost and raylib
add_compile_definitions(__SWITCH__)
set(ARCH -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE)

add_compile_options(
        ${ARCH}
        -fpermissive
        -Wno-narrowing
        -Wno-error=narrowing
        -ffunction-sections
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

add_link_options(
        ${ARCH}
        -specs=${LIBNX}/switch.specs
)

include(FetchContent)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/luizpestana/raylib-nx.git
        GIT_TAG        6.0
)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(SwitchPost ${SOURCES})
set_target_properties(SwitchPost PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(SwitchPost PRIVATE ${LIBNX}/include ${PORTLIBS}/include ./source ./include ${raylib_SOURCE_DIR}/src ${PROJECT_BINARY_DIR})
target_link_directories(SwitchPost PRIVATE ${LIBNX}/lib ${PORTLIBS}/lib)
target_link_libraries(SwitchPost PRIVATE curl z raylib EGL GLESv2 glapi drm_nouveau nx m)

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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(SwitchPost PRIVATE -O2)
    target_link_options(SwitchPost PRIVATE -s -O2)
    target_compile_definitions(SwitchPost PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO RELEASE)

    add_custom_command(TARGET SwitchPost POST_BUILD
            COMMAND nacptool --create ${APP_TITLE} ${APP_AUTHOR} ${APP_VERSION} $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp
            COMMAND elf2nro $<TARGET_FILE:SwitchPost> $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nro --nacp=$<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp --romfsdir=${PROJECT_SOURCE_DIR}/assets --icon=${PROJECT_SOURCE_DIR}/assets/sprites/inpost_logo.jpg
            COMMENT "Converting ELF to NRO"
    )
else()
    target_compile_options(SwitchPost PRIVATE -g3 -O0)
    target_link_options(SwitchPost PRIVATE -g3 -O0 -Wl,-Map,${TARGET}.map)
    target_compile_definitions(SwitchPost PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE DEBUG)

    add_custom_command(TARGET SwitchPost POST_BUILD
            COMMAND nacptool --create \"${APP_TITLE} DEBUG\" ${APP_AUTHOR} ${APP_VERSION} $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp
            COMMAND elf2nro $<TARGET_FILE:SwitchPost> $<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nro --nacp=$<TARGET_FILE_DIR:SwitchPost>/${PROJECT_NAME}.nacp --romfsdir=${PROJECT_SOURCE_DIR}/assets --icon=${PROJECT_SOURCE_DIR}/assets/sprites/inpost_logo.jpg
            COMMENT "Converting ELF to NRO"
    )
endif()