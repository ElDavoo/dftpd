//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_MAIN_H
#define DFTP_MAIN_H
#include "OpenedSockets.h"
#include "FileTable.h"


// Store the path to the users file without malloc
extern char users_file_path[1024];

extern OpenedSockets *openedSockets;

extern FileTable *file_table;

#endif //DFTP_MAIN_H
