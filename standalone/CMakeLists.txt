cmake_minimum_required(VERSION 3.16)
project(ProjectReboot3 VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_WITH_DISCORD "Enable Discord webhook support" ON)
option(BUILD_WITH_ENET "Enable ENet networking" ON)

# Platform detection
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS=1)
    add_definitions(-DUNICODE -D_UNICODE)
elseif(UNIX)
    add_definitions(-DPLATFORM_LINUX=1)
endif()

# Target version: Fortnite 17.50 (Season 7 Chapter 2)
add_definitions(-DFORTNITE_VERSION=1750)
add_definitions(-DENGINE_VERSION=426)

# =============================================================================
# Dependencies
# =============================================================================

# ENet (networking)
if(BUILD_WITH_ENET)
    include(FetchContent)
    FetchContent_Declare(
        enet
        GIT_REPOSITORY https://github.com/lsalzman/enet.git
        GIT_TAG v1.3.18
    )
    FetchContent_MakeAvailable(enet)
    add_definitions(-DWITH_ENET=1)
endif()

# nlohmann/json (configuration)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# spdlog (logging)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# fmt (formatting - bundled with spdlog but also standalone)
# spdlog brings fmt

# libcurl (for discord webhooks)
if(BUILD_WITH_DISCORD)
    find_package(CURL)
    if(CURL_FOUND)
        add_definitions(-DWITH_DISCORD=1)
    else()
        message(WARNING "libcurl not found, Discord webhook support disabled")
        set(BUILD_WITH_DISCORD OFF)
    endif()
endif()

# =============================================================================
# Source files
# =============================================================================

set(CORE_SOURCES
    src/main.cpp
    src/core/engine.cpp
    src/core/object_system.cpp
    src/core/asset_registry.cpp
    src/core/property_system.cpp
    src/core/event_dispatcher.cpp
)

set(GAME_SOURCES
    src/game/game_mode.cpp
    src/game/game_state.cpp
    src/game/player_controller.cpp
    src/game/player_pawn.cpp
    src/game/player_state.cpp
    src/game/inventory.cpp
    src/game/building.cpp
    src/game/loot.cpp
    src/game/safe_zone.cpp
    src/game/vehicles.cpp
    src/game/abilities.cpp
)

set(NET_SOURCES
    src/net/net_driver.cpp
    src/net/net_connection.cpp
    src/net/replication.cpp
    src/net/channel.cpp
)

set(UTIL_SOURCES
    src/util/config.cpp
    src/util/logging.cpp
    src/util/pak_parser.cpp
)

if(BUILD_WITH_DISCORD)
    list(APPEND UTIL_SOURCES src/util/discord.cpp)
endif()

# =============================================================================
# Executable
# =============================================================================

add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${GAME_SOURCES}
    ${NET_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/game
    ${CMAKE_CURRENT_SOURCE_DIR}/include/net
    ${CMAKE_CURRENT_SOURCE_DIR}/include/util
    ${enet_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

if(BUILD_WITH_ENET)
    target_link_libraries(${PROJECT_NAME} PRIVATE enet)
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm)
    endif()
endif()

if(BUILD_WITH_DISCORD AND CURL_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
endif()

# =============================================================================
# Install
# =============================================================================

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)
