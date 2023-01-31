cmake_minimum_required(VERSION 3.24)
project(dftp C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


set(CMAKE_C_STANDARD 99)

include_directories(dftpd)

add_executable(dftpd dftpd/main.c dftpd/main.h dftpd/FtpCommands.c dftpd/UserTable.c dftpd/OpenedSockets.c dftpd/OpenedSockets.h dftpd/OpenedSockets.h dftpd/FileTable.h dftpd/FileTable.c)

target_link_libraries(dftpd PRIVATE Threads::Threads)
