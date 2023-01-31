cmake_minimum_required(VERSION 3.2)
project(dftp C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 99)

include_directories(dftpd)

add_executable(dftpd main.c main.h FtpCommands.c UserTable.c OpenedSockets.c OpenedSockets.h OpenedSockets.h FileTable.h FileTable.c FtpThread.c FtpThread.h FtpThread.h)

target_link_libraries(dftpd PRIVATE Threads::Threads)
