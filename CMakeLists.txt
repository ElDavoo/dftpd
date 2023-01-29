cmake_minimum_required(VERSION 3.24)
project(dftp C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


set(CMAKE_C_STANDARD 23)

add_executable(dftp dftp/main.c)
add_executable(dftpd dftpd/main.c dftpd/FtpCommands.c dftpd/FtpCommands.h dftpd/FtpCommands.h)

target_link_libraries(dftp PRIVATE Threads::Threads)
target_link_libraries(dftpd PRIVATE Threads::Threads)