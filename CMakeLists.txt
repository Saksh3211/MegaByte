cmake_minimum_required(VERSION 3.21)
project(MegaBytePrototype CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "C:/Program Files/OpenSSL-Win64/include")
set(OPENSSL_CRYPTO_LIBRARY "C:/Program Files/OpenSSL-Win64/lib/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "C:/Program Files/OpenSSL-Win64/lib/libssl.lib")
find_package(OpenSSL REQUIRED)

add_executable(megabyte_node src/node/main.cpp)
target_link_libraries(megabyte_node PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if (MSVC)
    target_compile_options(megabyte_node PRIVATE /W4)
else()
    target_compile_options(megabyte_node PRIVATE -Wall -Wextra)
endif()