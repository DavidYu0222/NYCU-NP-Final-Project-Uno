#include	"serv1.h"

LoginData login_list[MAXCLIENT];
int check_login_list_flag = CHECK;
pthread_mutex_t mutex_login_list;

RoomData room_datas[MAXROOM];		 // the user data pass from lobby to room
int room_check_flag[MAXROOM];		 // fast check flag for room
pthread_mutex_t mutex_room[MAXROOM]; // protect room_datas room_check_flag

int back_to_lobby_list[MAXCLIENT];	 		// the fd of user passed from room to lobby
int free_rooms[MAXROOM];			 		// the empty room need to recycle
int check_back_to_lobby_list_flag = CHECK;	// fast check flag
pthread_mutex_t mutex_back_to_lobby_list;	// protect back_to_lobby_list, free_rooms, check_back_to_lobby_list_flag

int close_list[MAXCLIENT];				// the fd of user who lost in room
int check_close_list_flag = CHECK;		// fast check flag
pthread_mutex_t mutex_close_list;		// protect close_list

void init(){
	bzero(back_to_lobby_list, sizeof(back_to_lobby_list));
	bzero(room_datas, sizeof(room_datas));
	bzero(room_check_flag, sizeof(room_check_flag));

	for(int i = 0; i < MAXROOM; i++){
		free_rooms[i] = -1;
	}

	pthread_mutex_init(&mutex_back_to_lobby_list, NULL);

	pthread_mutex_init(&mutex_close_list, NULL);

	for(int i = 0; i < MAXROOM; i++){
		pthread_mutex_init(&mutex_room[i], NULL);
	}
}

void check_data(int room_id){
	printf("room_id: %d\n", room_id);
	for(int i = 0; i < MAXMEMBER; i++){
		printf("id: %s\t, fd: %d\n", room_datas[room_id].member[i].id, room_datas[room_id].member[i].fd);
	}
}

void exit_room_handler(int room_id, fd_set *rset, int* maxfdp, int* numOfMember, Member* members, int type,int index){
	/* Notify other memberss*/
		char msg[MAXLINE];
		sprintf(msg, "(%s left the room. %d users left)\n", members[index].id, *numOfMember-1);
		for(int j = 0; j < MAXMEMBER; j++) {
			if(members[j].fd == 0 || index == j){
				continue;
			}
			Writen(members[j].fd, msg, sizeof(msg));
		}
	/* Clear data in global */
		pthread_mutex_lock(&mutex_room[room_id]);
		room_datas[room_id].numOfMember--;
		room_datas[room_id].member[index].fd = 0;
		room_datas[room_id].member[index].id = NULL;
		for(int i = 0; i < MAXMEMBER - 1; i++){
			if(room_datas[room_id].member[i].fd == 0){
				for(int j = i ; j < MAXMEMBER - 1; j++){
					room_datas[room_id].member[j] = room_datas[room_id].member[j+1];
				}
				room_datas[room_id].member[MAXMEMBER - 1].fd = 0;
				room_datas[room_id].member[MAXMEMBER - 1].id = NULL;
			}
		}
		pthread_mutex_unlock(&mutex_room[room_id]);
	
	if(type == DISCONN){
	/* Add it to close list */
		pthread_mutex_lock(&mutex_close_list);
		for (int k = 0; k < MAXCLIENT; k++){
			if(close_list[k] == 0){
				close_list[k] = members[index].fd;
				break;
			}
		}
		check_close_list_flag = UNCHECK;
		pthread_mutex_unlock(&mutex_close_list);
	}else{
	/* Add member's fd to back_to_lobby_list */
		pthread_mutex_lock(&mutex_back_to_lobby_list);
		for (int k = 0; k < MAXCLIENT; k++){
			if(back_to_lobby_list[k] == 0){
				back_to_lobby_list[k] = members[index].fd;
				break;
			}
		}
		check_back_to_lobby_list_flag = UNCHECK;
		pthread_mutex_unlock(&mutex_back_to_lobby_list);
	}
	
	/* Clear data in local*/
		FD_CLR(members[index].fd, rset);
		int maxfd = 0;
		for(int j = 0; j < MAXMEMBER; j++) {
			if(members[j].fd != 0){
				maxfd = max(members[j].fd, maxfd);
			}
		}
		*maxfdp = maxfd + 1;
		*numOfMember = *numOfMember - 1;
		members[index].fd = 0;
		members[index].id = NULL;
		for(int i = 0; i < MAXMEMBER - 1; i++){
			if(members[i].fd == 0){
				for(int j = i ; j < MAXMEMBER - 1; j++){
					members[j] = members[j+1];
				}
				members[MAXMEMBER - 1].fd = 0;
				members[MAXMEMBER - 1].id = NULL;
			}
		}
}

void* play_room(void* arg){
	pthread_detach(pthread_self());

	RoomData* room_data = (RoomData*) arg;

	Member    members[MAXMEMBER];
	int       room_id = room_data->id;
	int 	  numOfMember = room_data->numOfMember;
	members[0] = room_data->member[0];

	int 	  n;
	char	  buf[MAXLINE];
	char	  enter_msg[MAXLINE];
	int       maxfdp;
    fd_set    tmp, rset;
	struct timeval tv;

	sprintf(enter_msg, "(Enter room. Room id: %d)\n", room_id);

	Writen(members[0].fd, enter_msg, strlen(enter_msg));

	FD_ZERO(&rset);
    FD_SET(members[0].fd, &rset);
    maxfdp = members[0].fd + 1;
	for(;numOfMember > 0;) {
		int flag[MAXMEMBER];
		bzero(&flag, sizeof(flag));
/* Check whether there exist new memberss want to enter */
		if(numOfMember != MAXMEMBER && pthread_mutex_trylock(&mutex_room[room_id]) == 0){
			/* # of members enter > 0 */
			if(room_check_flag[room_id] > 0){			
				for(int i = MAXMEMBER-1; i >= 0; i--){		
					/* Copy the members data from global to local */ 
					int newfd = room_datas[room_id].member[i].fd;
					if(newfd != 0 && newfd != members[i].fd){
						members[i].fd = newfd;
						members[i].id = room_datas[room_id].member[i].id;
						room_check_flag[room_id]--;
						flag[room_check_flag[room_id]] = i;
					}
				}
			}
            pthread_mutex_unlock(&mutex_room[room_id]);
        }
/* Send notified message to other members in room */
		for(int j = 0; j < MAXMEMBER; j++){
			if(flag[j] > 0){
				numOfMember++;
				FD_SET(members[flag[j]].fd, &rset);
				maxfdp = max(maxfdp - 1, members[flag[j]].fd) + 1;
				sprintf(buf, "(%s enter the room)\n", members[flag[j]].id);
				for(int i = 0; i < MAXMEMBER; i++){
					if( members[i].fd == 0 ) 
						continue;
					if( i == flag[j] ){
						Writen(members[i].fd, enter_msg, strlen(enter_msg));
						continue;
					}
					Writen(members[i].fd, buf, strlen(buf));
				}
			}
		}
/* Set the timer for select() (How long to check room_datas) */
		tmp = rset;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		Select(maxfdp, &tmp, NULL, NULL, &tv);
/* Process members's messages */
		for(int i = 0; i < MAXMEMBER; i++) {	
			if(members[i].fd != 0 && FD_ISSET(members[i].fd, &tmp)){
				if ( (n = Read(members[i].fd, buf, MAXLINE)) == 0) {
					/* Deal client lost*/
					exit_room_handler(room_id, &rset, &maxfdp, &numOfMember, members, DISCONN, i);
					break;
				}
				buf[n] = '\0';
			/* Member exit room (back to lobby) */
				if(strcmp(buf, "exit\n\n") == 0){
					exit_room_handler(room_id, &rset, &maxfdp, &numOfMember, members, OK, i);
				}
				else if(strcmp(buf, "start_uno\n\n") == 0){
					pthread_mutex_lock(&mutex_room[room_id]);
					room_datas[room_id].status = BUSY;
					pthread_mutex_unlock(&mutex_room[room_id]);
					Status status =  uno_game(numOfMember, members);
					/* Deal client lost*/
					if(status.status == DISCONN){
						exit_room_handler(room_id, &rset, &maxfdp, &numOfMember, members, DISCONN, status.index);
					}
					pthread_mutex_lock(&mutex_room[room_id]);
					room_datas[room_id].status = IDLE;
					pthread_mutex_unlock(&mutex_room[room_id]);
				}
				else{
					char *usr_msg = malloc(strlen(members[i].id) + strlen(buf) + 4);
					sprintf(usr_msg, "(%s) %s", members[i].id, buf);
					for(int j = 0; j < MAXMEMBER; j++) {
						if(members[j].fd == 0 || i == j){
							continue;
						}
						Writen(members[j].fd, usr_msg, strlen(usr_msg));
					}
					free(usr_msg);
				}
			}
		}
		
	}
	/* Nobody in room, free the room*/
	pthread_mutex_lock(&mutex_back_to_lobby_list);
	for(int i = 0; i < MAXROOM; i++){
		if (free_rooms[i] == -1){
			free_rooms[i] = room_id;
			break;
		}
	}
	check_back_to_lobby_list_flag = UNCHECK;
	pthread_mutex_unlock(&mutex_back_to_lobby_list);

	pthread_exit(NULL);
}

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
				Writen(fd, enter_msg, strlen(enter_msg));
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
				Writen(fd, enter_msg, strlen(enter_msg));
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
				Writen(fd, enter_msg, strlen(enter_msg));
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
				Writen(fd, enter_msg, strlen(enter_msg));
            }else if(result == PASSWD_NOT_MATCH){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg2, strlen(error_msg2));
				Writen(fd, enter_msg, strlen(enter_msg));
            }
            break;
        case SIGN_UP_USERNAME: // Sign up (Username)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
				Writen(fd, enter_msg, strlen(enter_msg));
                break;
            }
            strcpy(id, token);
            if (!is_username_unique(id)) {
                status = CHOOSE_OPTION;
                sprintf(buf, "Error: Username '%s' already exists.\n", id);
                Writen(fd, buf, strlen(buf));
				Writen(fd, enter_msg, strlen(enter_msg));
            }else{
                status = SIGN_UP_PASSWORD;
                Writen(fd, sign_msg3, strlen(sign_msg3));
            }
            break;
        case SIGN_UP_PASSWORD: // Sign up (Password)
            if(token == NULL || strlen(token) > MAXID){
                status = CHOOSE_OPTION;
                Writen(fd, error_msg3, strlen(error_msg3));
				Writen(fd, enter_msg, strlen(enter_msg));
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
				Writen(fd, enter_msg, strlen(enter_msg));
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
				Writen(fd, enter_msg, strlen(enter_msg));
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

int main(int argc, char **argv)
{
//#region initial bind
	int					listenfd;
	int 				n;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	int enable = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		printf("setsockopt(SO_REUSEADDR) fail!\n");
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT+5);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);
//#endregion
	pthread_t rooms[MAXROOM];			// room's thread
	pthread_t login[MAXCLIENT];
	int       busy_rooms[MAXROOM];		// non-empty room flag
	int		  busy_login[MAXCLIENT];
	int       curConn = 0; 				// number of the current connections to server
    int       client_list[MAXCLIENT];	// the fd of client
    bool      in_lobby_flag[MAXCLIENT];	// flag record whether client is in lobby
	char* 	  client_id[MAXCLIENT];		// store users' ID

	bzero(busy_rooms, sizeof(busy_rooms));
	bzero(in_lobby_flag, sizeof(in_lobby_flag));
	bzero(client_list, sizeof(client_list));

	char* 	  bye_msg = "Bye!\n";
	char*	  create_room_error_msg = "(CREATE ROOM FAIL!)\n";
	char*	  enter_room_error_msg1 = "(Please specify the room id!)\n";
	char*	  enter_room_error_msg2 = "(This Room not exist!)\n";
	char*	  enter_room_error_msg3 = "(This Room is full!)\n";
	char*	  enter_room_error_msg4 = "(This Room is hosting a game !)\n";

	int       maxfdp;
    fd_set    tmp, rset;
	struct timeval tv;
	char	  buf[MAXLINE];
	char	  cmd[MAXLINE];
/* Initialize global varible */
	init();

	FD_ZERO(&rset);
	FD_SET(0 , &rset);
    FD_SET(listenfd, &rset);
    maxfdp = listenfd + 1;
	for ( ; ; ) {
/* Check whether someone goes back to lobby or room need to free */
        if(pthread_mutex_trylock(&mutex_back_to_lobby_list) == 0){
            if(check_back_to_lobby_list_flag == UNCHECK){
                for(int i = 0; i < MAXCLIENT; i++){
                    if (back_to_lobby_list[i] != 0){
						FD_SET(back_to_lobby_list[i], &rset);
						maxfdp = max(maxfdp - 1, back_to_lobby_list[i]) + 1;
						for(int j = 0; j < MAXCLIENT; j++){
							if(client_list[j] == back_to_lobby_list[i]){
								in_lobby_flag[j] = true;
								break;
							}
						}
						printf("Add FD (%d) back to rset\n", back_to_lobby_list[i]);
						back_to_lobby_list[i] = 0;
                    }
                }
				for(int i = 0; i < MAXROOM; i++){
                    if (free_rooms[i] != -1){
						printf("Free Room (ID:%d)\n", free_rooms[i]);
						busy_rooms[free_rooms[i]] = 0;
						free_rooms[i] = -1;
                    }
                }
                check_back_to_lobby_list_flag = CHECK;
				printf("CHECK OK\n");
            }
            pthread_mutex_unlock(&mutex_back_to_lobby_list);
        }

/* Check whether someone close the connection */
        if(pthread_mutex_trylock(&mutex_close_list) == 0){
            if(check_close_list_flag == UNCHECK){
                for(int i = 0; i < MAXCLIENT; i++){
                    if (close_list[i] != 0){
						Close(close_list[i]);
						int j;
						for(j = 0; j < MAXCLIENT; j++){
							if(client_list[j] == close_list[i]) break;
						}
						client_list[j] = 0;
						client_id[j] = NULL;
						in_lobby_flag[j] = false;
						printf("Close Connection FD (%d)\n", close_list[i]);
						close_list[i] = 0;
                    }
                }
                check_close_list_flag = CHECK;
				printf("CHECK CLOSE OK\n");
            }
            pthread_mutex_unlock(&mutex_close_list);
        }

/* Check the login from login system */
		if(pthread_mutex_trylock(&mutex_login_list) == 0){
			if(check_login_list_flag == UNCHECK){
				for(int k = 0; k < MAXCLIENT; k++) {
					if(login_list[k].fd > 0){
						int i;
						for( i = 0; i < MAXCLIENT; i++) {
							if(client_list[i] == 0){
								client_list[i] = login_list[k].fd;
								client_id[i] = login_list[k].id;
								in_lobby_flag[i] = true;
								break;
							}
						}
						login_list[k].login_id = 0;
						login_list[k].fd = 0;
						login_list[k].id = NULL;
						busy_login[login_list[k].login_id] = 0;
						curConn++;
						printf("New client FD: %d\n", login_list[k].fd);

						char msg[MAXLINE];
						sprintf(msg, "Welcome to lobby!\nYou are the #%d user.\n", i+1);
						Writen(client_list[i], msg, MAXLINE);

						sprintf(msg, "(#%d user %s enters lobby)\n", i+1, client_id[i]);
						for(int j = 0; j < MAXCLIENT; j++) {
							if(in_lobby_flag[j] == false || i == j ){
								continue;
							}
							Writen(client_list[j], msg, MAXLINE);
						}
						FD_SET(client_list[i], &rset);
						maxfdp = max(client_list[i], maxfdp - 1) + 1;
					}
					else if (login_list[k].fd < 0){
						busy_login[k] = 0;
						login_list[k].fd = 0;
						login_list[k].login_id = 0;
					}
				}
				check_login_list_flag = CHECK;
			}
			pthread_mutex_unlock(&mutex_login_list);
		}

		tmp = rset;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		Select(maxfdp, &tmp, NULL, NULL, &tv);
/* Accept new client*/
		if (curConn < MAXCLIENT && FD_ISSET(listenfd, &tmp)) {
			printf("Accept new client\n");
			clilen = sizeof(cliaddr);
			int connfd_tmp;
			if ( (connfd_tmp = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
				if (errno == EINTR)
					continue;		/* back to for() */
				else
					printf("Accept error\n");
                    continue;
			}

			char client_id_tmp[MAXID];
			if ( (n = Read(connfd_tmp, client_id_tmp, MAXID)) == 0) {
				printf("Read error\n");
				continue;
			}
			//printf("New client ID: %s\n", client_id_tmp);
			for(int i = 0; i < MAXCLIENT; i++){
				if(busy_login[i] == 0){
					LoginData loginData;
					loginData.login_id = i;
					loginData.fd = connfd_tmp;
					pthread_create(&login[i], NULL, login_system, &loginData);
					busy_login[i] = 1;
					break;
				}
			}
		}
/* Server status */
		if(FD_ISSET(0, &tmp)){
			char    sendline[MAXLINE];
    		Fgets(sendline, MAXLINE, stdin);
			if(strcmp(sendline, "exit\n") == 0){
				break;
			}
			if(strcmp(sendline, "cli\n") == 0){
				for(int i = 0; i < MAXCLIENT; i++){
					if(in_lobby_flag[i])
						printf("%d: %d %s\tin lobby\n", i, client_list[i], client_id[i]);
					else
						printf("%d: %d %s\tnot in lobby\n", i, client_list[i], client_id[i]);
				}
			}else if(strcmp(sendline, "login\n") == 0){
				for(int i = 0; i < MAXCLIENT; i++){
					printf("%d: busy_login %d\n", i, busy_login[i]);
				}
			}
			else{
				for(int i = 0; i < MAXROOM; i++){
					check_data(i);
					printf("\n");
				}
			}

		}
/* Process other clients' messages */
		for(int i = 0; i < MAXCLIENT; i++) {	
			if(client_list[i] != 0 && FD_ISSET(client_list[i], &tmp)){	
			/* Connection interrupt */
				if ( (n = Read(client_list[i], buf, MAXLINE)) == 0) {
					Writen(client_list[i], bye_msg, strlen(bye_msg));
					curConn--;
					char msg[MAXLINE];
					sprintf(msg, "(%s left the lobby. %d users left)\n", client_id[i], curConn);
					for(int j = 0; j < MAXCLIENT; j++) {
						if(in_lobby_flag[j] == false || i == j){
							continue;
						}
						Writen(client_list[j], msg, MAXLINE);
					}
                    FD_CLR(client_list[i], &rset);
					Close(client_list[i]);
					client_list[i] = 0;
					client_id[i] = NULL;
					in_lobby_flag[i] = false;
                    int maxfd = listenfd;
                    for(int j = 0; j < MAXCLIENT; j++) {
                        if(in_lobby_flag[j] == true){
                            maxfd = max(client_list[j], maxfd);
                        }
                    }
                    maxfdp = maxfd + 1;
					continue;
				}
				
				buf[n] = '\0';

				strcpy(cmd, buf);
				char* token = strtok(cmd, " \n");
				if(token == NULL){
					continue;
				}
			/* Create room */
				else if(strcmp(token, "create_room") == 0){
					int flag = 1;
					for(int k = 0; k < MAXROOM; k++){
						if(busy_rooms[k] == 0){
							busy_rooms[k] = 1;
							room_datas[k].status = IDLE;
							room_datas[k].id = k;
							room_datas[k].numOfMember = 1;
							room_datas[k].member[0].fd = client_list[i];
							room_datas[k].member[0].id=client_id[i];
							FD_CLR(client_list[i], &rset);
							in_lobby_flag[i] = false;
							int maxfd = listenfd;
							for(int j = 0; j < MAXCLIENT; j++) {
								if(in_lobby_flag[j] == true){
									maxfd = max(client_list[j], maxfd);
								}
							}
							maxfdp = maxfd + 1;
							printf("Remove FD:%d from rset\n", client_list[i]);
							printf("Create room %d\n", k);
							pthread_create(&rooms[k], NULL, play_room, &room_datas[k]);
							flag = 0;
							break;
						}	
					}
					if(flag == 1)
						Writen(client_list[i], create_room_error_msg, strlen(create_room_error_msg));
				}
			/* Enter room */
				else if(strcmp(token, "enter_room") == 0){
					token = strtok(NULL, " \n");
					if(token == NULL){
						/* No specify room id*/
						Writen(client_list[i], enter_room_error_msg1, strlen(enter_room_error_msg1));
						continue;
					}
					int room_id = atoi(token);

					if(room_id < 0 || room_id > MAXROOM - 1 || busy_rooms[room_id] == 0){
						/* Room not exist */
						Writen(client_list[i], enter_room_error_msg2, strlen(enter_room_error_msg2));
						continue;
					}
					pthread_mutex_lock(&mutex_room[room_id]);
					if(room_datas[room_id].numOfMember == 4){
						/* Reach the maximum client of room */
						pthread_mutex_unlock(&mutex_room[room_id]);
						Writen(client_list[i], enter_room_error_msg3, strlen(enter_room_error_msg3));
						continue;
					}else if(room_datas[room_id].status){
						/* Game is going on in the room */
						pthread_mutex_unlock(&mutex_room[room_id]);
						Writen(client_list[i], enter_room_error_msg4, strlen(enter_room_error_msg4));
						continue;
					}
					for (int j = 0; j < MAXMEMBER; j++){
						if(room_datas[room_id].member[j].fd == 0){
							room_datas[room_id].member[j].fd = client_list[i];
							room_datas[room_id].member[j].id = client_id[i];
							break;
						}
					}
					room_datas[room_id].numOfMember++;
					room_check_flag[room_id]++;
					FD_CLR(client_list[i], &rset);
					int maxfd = listenfd;
                    for(int j = 0; j < MAXCLIENT; j++) {
                        if(in_lobby_flag[j] == true){
                            maxfd = max(client_list[j], maxfd);
                        }
                    }
                    maxfdp = maxfd + 1;
					in_lobby_flag[i] = false;
					printf("Remove FD:%d from rset\n", client_list[i]);
					printf("Enter room %d\n", room_id);
					pthread_mutex_unlock(&mutex_room[room_id]);
				}
			/* List the online client and room */
				else if(strcmp(token, "-ls") == 0){
					char sendline[MAXLINE];
					sprintf(sendline, "Online List\n");
					for(int j = 0; j < MAXCLIENT; j++){
						char buf[MAXID + 30];
						if(in_lobby_flag[j]){
							sprintf(buf, "user %d: %s\tin lobby\n", j, client_id[j]);
							strcat(sendline, buf);
						}else if(client_id[j] != NULL) {
							sprintf(buf, "user %d: %s\tnot in lobby\n", j, client_id[j]);
							strcat(sendline, buf);
						}
					}
					bool flag = 1;
					char* room_msg0 = "\nAvailable Room List\n";\
					strcat(sendline, room_msg0);
					for(int j = 0; j < MAXROOM; j++){
						char buf[30];
						if(busy_rooms[j]){
							sprintf(buf, "room %d has %d clients\n", j, room_datas[j].numOfMember);
							strcat(sendline, buf);
							flag = 0;
						}
					}
					if(flag) strcat(sendline, "NULL\n");
					Writen(client_list[i], sendline, strlen(sendline));
				}
				// else if(strcmp(token, "login") == 0){
				// 	pthread_create(&login, NULL, login_system, &client_list[i]);
				// 	FD_CLR(client_list[i], &rset);
				// }
			/* Normal message passing */
				else{
					char *usr_msg = malloc(strlen(client_id[i]) + strlen(buf) + 4);
					sprintf(usr_msg, "(%s) %s", client_id[i], buf);

					for(int j = 0; j < MAXCLIENT; j++) {
						if(in_lobby_flag[j] == false || i == j){
							continue;
						}
						Writen(client_list[j], usr_msg, strlen(usr_msg));
					}
					free(usr_msg);
				}
				
			}
		}
	}

	for(int i = 0; i < MAXCLIENT; i++){
		if(client_list[i] != 0)
		 	Close(client_list[i]);
	}

	pthread_mutex_destroy(&mutex_back_to_lobby_list);
	pthread_mutex_destroy(&mutex_close_list);
	for(int i = 0; i < MAXROOM; i++){
		pthread_mutex_destroy(&mutex_room[i]);
	}
	return 0;
}