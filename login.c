#include "serv1.h"

int verify_login(char* id, char* password) {
    FILE *file = fopen("userdata.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[2*MAXID + 2];
    char username[MAXID];
    char pass[MAXID];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %s", username, pass) == 2) {
            if (strcmp(id, username) == 0){
                if(strcmp(password, pass) == 0) {
                    fclose(file);
                    return MATCH;
                }
                fclose(file);
                return PASSWD_NOT_MATCH;
            }
        }
    }

    fclose(file);
    return USER_NOT_EXIST;
}

int is_username_unique(const char* id) {
    FILE *file = fopen("userdata.txt", "r");
    if (file == NULL) {
        return -1;
    }

    char line[2*MAXID + 2];
    char username[MAXID];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s", username) == 1) {
            if (strcmp(id, username) == 0) {
                fclose(file);
                return 0;
            }
        }
    }

    fclose(file);
    return 1;
}

int add_user(char* id, char* password) {
    FILE *file = fopen("userdata.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    if (fprintf(file, "%s %s\n", id, password) < 0) {
        perror("Error writing to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 1;
}

void* login_system(void* arg){
    pthread_detach(pthread_self());
    LoginData* fdptr = (LoginData*) arg; 
    int fd = fdptr->fd;
    int login_id = fdptr->login_id;

    int 	  n;
	char	  buf[MAXLINE];
	// int       maxfdp = fd + 1;
    // fd_set    rset;
	// struct timeval tv;

    char*	  enter_msg = "Welcome to uno game lobby!\nChoose \"sign in(0)\" or \"sign up(1)\"\n";

    char*     error_msg0 = "Invalid Option (0: sign in, 1: sign up)\n";
    char*     error_msg1 = "Error: User Not Found\n";
    char*     error_msg2 = "Error: Password Not Match\n";
    char*     error_msg3 = "Error: Exceed the limitation of string length\n";

    char*     sign_msg0 = "Username:\n";
    char*     sign_msg1 = "Password:\n";
    char*     sign_msg2 = "Type Username (Don't use space in your username, maximum 50 character):\n";
    char*     sign_msg3 = "Type Password (Don't use space in your password, maximum 50 character):\n";
    char*     sign_msg4 = "Retype Password:\n";
    char*     sign_msg5 = "Login Success\n";

    char*     id;
    char      passwd[MAXID];

    id = malloc(MAXID);

	Writen(fd, enter_msg, strlen(enter_msg));

    int status = CHOOSE_OPTION;
    for(;status != LOGIN_SUCCESS;) {
        if ( (n = Read(fd, buf, MAXLINE)) == 0) {
            pthread_mutex_lock(&mutex_login_list);
            login_list[login_id].fd = -1;
            login_list[login_id].id = NULL;
            login_list[login_id].login_id = login_id;
            check_login_list_flag = UNCHECK;
            pthread_mutex_unlock(&mutex_login_list);
            Close(fd);
            free(id);
            pthread_exit(NULL);
        }
        buf[n] = '\0';
        char* token = strtok(buf, " \n");
        switch (status){
        case CHOOSE_OPTION: // Choose Sign in or Sign up
            if(token == NULL){
                Writen(fd, error_msg0, strlen(error_msg0));
                break;
            }
            int opt = atoi(token);
            if(opt == 0){
                status = SIGN_IN_USERNAME;
                Writen(fd, sign_msg0, strlen(sign_msg0));
            }else if(opt == 1){
                status = SIGN_UP_USERNAME;
                Writen(fd, sign_msg2, strlen(sign_msg2));
            }else{
                Writen(fd, error_msg0, strlen(error_msg0));
            }
            break;
        case SIGN_IN_USERNAME: // Sign in (Username)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
                break;
            }
            strcpy(id, token);
            status = SIGN_IN_PASSWORD;
            Writen(fd, sign_msg1, strlen(sign_msg1));
            break;
        case SIGN_IN_PASSWORD: // Sign in (Password)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
                break;
            }
            strcpy(passwd, token);
            int result = verify_login(id, passwd);
            if(result == MATCH){
                status = LOGIN_SUCCESS;
                Writen(fd, sign_msg5, strlen(sign_msg5));
            }else if(result == USER_NOT_EXIST){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg1, strlen(error_msg1));
            }else if(result == PASSWD_NOT_MATCH){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg2, strlen(error_msg2));
            }
            break;
        case SIGN_UP_USERNAME: // Sign up (Username)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
                break;
            }
            strcpy(id, token);
            if (!is_username_unique(id)) {
                status = CHOOSE_OPTION;
                sprintf(buf, "Error: Username '%s' already exists.\n", id);
                Writen(fd, buf, strlen(buf));
            }else{
                status = SIGN_UP_PASSWORD;
                Writen(fd, sign_msg3, strlen(sign_msg3));
            }
            break;
        case SIGN_UP_PASSWORD: // Sign up (Password)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
                break;
            }
            strcpy(passwd, token);
            status = SIGN_UP_RETYPE;
            Writen(fd, sign_msg4, strlen(sign_msg4));
            break;
        case SIGN_UP_RETYPE: // Sign up (Retype Password)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
                break;
            }
            char r_passwd[MAXID];
            strcpy(r_passwd, token);
            if(strcmp(passwd, r_passwd) == 0){
                status = LOGIN_SUCCESS;
                add_user(id, passwd);
                Writen(fd, sign_msg5, strlen(sign_msg5));
            }else{
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
            }
            break;
        default:
            break;
        }
    }

    pthread_mutex_lock(&mutex_login_list);
    login_list[login_id].fd = fd;
    login_list[login_id].id = id;
    login_list[login_id].login_id = login_id;
    check_login_list_flag = UNCHECK;
    pthread_mutex_unlock(&mutex_login_list);
    pthread_exit(NULL);
	// for(;;) {
    //     FD_ZERO(&rset);
    //     FD_SET(fd, &rset);

    //     Select(maxfdp, &rset, NULL, NULL, NULL);

    //     if(FD_ISSET(fd, &rset));{
    //         if ( (n = Read(fd, buf, MAXLINE)) == 0) {
    //             Close(fd);
    //             return;
    //         }
    //     }
    // }
}