cmake_minimum_required(VERSION 3.20)
project(SecureAuthenticator)

set(CMAKE_CXX_STANDARD 20)

# 1. The Auth Server (The Core System)
# We will create the file src/Server.cpp in the next step
add_executable(AuthServer src/Server.cpp
        include/sha256.h)
# Link Windows Socket Library (ws2_32)
target_link_libraries(AuthServer ws2_32)

# 2. The Client App (The User Interface)
# We will create the file src/Client.cpp in the next step
add_executable(UserClient src/Client.cpp
        include/sha256.h)
target_link_libraries(UserClient ws2_32)