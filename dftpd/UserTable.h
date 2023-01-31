//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_USERTABLE_H
#define DFTP_USERTABLE_H

typedef struct {
    char *username;
    char *password;
} User;

User GetUserFromFile(char *username, char *users_file_path);

#endif //DFTP_USERTABLE_H
