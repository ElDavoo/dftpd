//
// Created by Davide on 29/01/2023.
//
#include <stdio.h>
#include <string.h>
#include "UserTable.h"

// Method to get the user from the file
User GetUserFromFile (char *username, char *users_file_path) {
    FILE *users_file = fopen(users_file_path, "r");
    User usr;
    usr.username = NULL;
    usr.password = NULL;
    char line[1024];
    while (fgets(line, sizeof(line), users_file)) {
        char *token = strtok(line, ",");
        if (strcmp(token, username) == 0) {
            usr.username = username;
            usr.password = strtok(NULL, ",");
            return usr;
        }
    }

    return usr;
}