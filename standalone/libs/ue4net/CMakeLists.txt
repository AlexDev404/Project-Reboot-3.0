# UE4 Networking Shared Library
# Extracted from Unreal Engine 4.27 source (OpenHUTB/engine mirror)
# Compiled standalone with compatibility shim

cmake_minimum_required(VERSION 3.16)
project(ue4net VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform detection
if(WIN32)
    set(UE4NET_PLATFORM_WINDOWS 1)
elseif(UNIX AND NOT APPLE)
    set(UE4NET_PLATFORM_LINUX 1)
elseif(APPLE)
    set(UE4NET_PLATFORM_MAC 1)
endif()

# Source files
set(UE4NET_SOURCES
    src/Core/BitReader.cpp
    src/Core/BitWriter.cpp
    src/Net/NetPacketNotify.cpp
    src/Crypto/SHA1.cpp
    src/PacketHandlers/StatelessConnectHandlerComponent.cpp
    src/Channel/DataBunch.cpp
    src/Channel/Channel.cpp
    src/Channel/ControlChannel.cpp
    src/Channel/ActorChannel.cpp
    src/Channel/NetConnection.cpp
    src/Channel/NetDriver.cpp
    src/Replication/RepLayout.cpp
    src/RPC/RPCDispatch.cpp
)

# Header files (for IDE integration)
file(GLOB_RECURSE UE4NET_HEADERS include/*.h)

# Build as shared library
add_library(ue4net SHARED ${UE4NET_SOURCES} ${UE4NET_HEADERS})

# Include directories
target_include_directories(ue4net
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Export macros
target_compile_definitions(ue4net
    PRIVATE
        UE4NET_BUILDING=1
    PUBLIC
        $<$<BOOL:${UE4NET_PLATFORM_WINDOWS}>:UE4NET_PLATFORM_WINDOWS=1>
        $<$<BOOL:${UE4NET_PLATFORM_LINUX}>:UE4NET_PLATFORM_LINUX=1>
        $<$<BOOL:${UE4NET_PLATFORM_MAC}>:UE4NET_PLATFORM_MAC=1>
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(ue4net PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(ue4net PRIVATE ws2_32)
elseif(UNIX)
    target_link_libraries(ue4net PRIVATE pthread)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(ue4net PRIVATE /W3 /wd4100 /wd4244 /wd4267 /wd4458)
else()
    target_compile_options(ue4net PRIVATE -Wall -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable)
endif()

# Install rules
install(TARGETS ue4net
    EXPORT ue4net-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
