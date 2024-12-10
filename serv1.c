#include	"serv1.h"

RoomData room_datas[MAXROOM];		 // the user data pass from lobby to room
int room_check_flag[MAXROOM];		 // fast check flag for room
pthread_mutex_t mutex_room[MAXROOM]; // protect room_datas room_check_flag

int back_to_lobby_list[MAXCLIENT];	 		// the user fd pass from room to lobby
int free_rooms[MAXROOM];			 		// the empty room need to recycle
int check_back_to_lobby_list_flag = CHECK;				 		// fast check flag
pthread_mutex_t mutex_back_to_lobby_list;	// protect back_to_lobby_list, free_rooms, check_back_to_lobby_list_flag

int close_list[MAXCLIENT];				// 
int check_close_list_flag = CHECK;	
pthread_mutex_t mutex_close_list;

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
		printf("id: %s, fd: %d\n", room_datas[room_id].member[i].id, room_datas[room_id].member[i].fd);
	}
}

void disconnect_in_room_handler(int room_id, fd_set *rset, int* maxfdp, int * numOfMember, Member* members, int index){
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
					disconnect_in_room_handler(room_id, &rset, &maxfdp, &numOfMember, members, i);
					break;
				}
				buf[n] = '\0';
			/* Member exit room (back to lobby) */
				if(strcmp(buf, "exit\n\n") == 0){
				/* Notify other memberss*/
					char msg[MAXLINE];
					sprintf(msg, "(%s left the room. %d users left)\n", members[i].id, numOfMember-1);
					for(int j = 0; j < MAXMEMBER; j++) {
						if(members[j].fd == 0 || i == j){
							continue;
						}
						Writen(members[j].fd, msg, sizeof(msg));
					}
				/* Clear data in global */
					pthread_mutex_lock(&mutex_room[room_id]);
					room_datas[room_id].numOfMember--;
					room_datas[room_id].member[i].fd = 0;
					room_datas[room_id].member[i].id = NULL;
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
				/* Add member's fd to back_to_lobby_list */
					pthread_mutex_lock(&mutex_back_to_lobby_list);
					for (int k = 0; k < MAXCLIENT; k++){
						if(back_to_lobby_list[k] == 0){
							back_to_lobby_list[k] = members[i].fd;
							break;
						}
					}
					check_back_to_lobby_list_flag = UNCHECK;
					pthread_mutex_unlock(&mutex_back_to_lobby_list);
				/* Clear data in local*/
					FD_CLR(members[i].fd, &rset);
					int maxfd = 0;
                    for(int j = 0; j < MAXMEMBER; j++) {
                        if(members[j].fd != 0){
                            maxfd = max(members[j].fd, maxfd);
                        }
                    }
                    maxfdp = maxfd + 1;
					numOfMember--;
					members[i].fd = 0;
					members[i].id = NULL;
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
				else if(strcmp(buf, "start_uno\n\n") == 0){
					pthread_mutex_lock(&mutex_room[room_id]);
					room_datas[room_id].status = BUSY;
					pthread_mutex_unlock(&mutex_room[room_id]);
					Status status =  uno_game(numOfMember, members);
					/* Deal client lost*/
					if(status.status == DISCONN){
						disconnect_in_room_handler(room_id, &rset, &maxfdp, &numOfMember, members, status.index);
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
	int       busy_rooms[MAXROOM];		// non-empty room flag
	int       curConn = 0; 				// number of the current connections to server
    int       client_list[MAXCLIENT];	// the fd of client
    bool      in_lobby_flag[MAXCLIENT];	// flag record whether client is in lobby
	char* 	  client_id[MAXCLIENT];		// store users' ID

	bzero(busy_rooms, sizeof(busy_rooms));
	bzero(&in_lobby_flag, sizeof(in_lobby_flag));
	bzero(&client_list, sizeof(client_list));

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

			char* client_id_tmp = malloc(MAXID);
			if ( (n = Read(connfd_tmp, client_id_tmp, MAXID)) == 0) {
				printf("Read error\n");
				continue;
			}
			printf("New client ID: %s\n", client_id_tmp);

			int i;
			for( i = 0; i < MAXCLIENT; i++) {
				if(client_list[i] == 0){
					client_list[i] = connfd_tmp;
					in_lobby_flag[i] = true;
					break;
				}
			}
			curConn++;
			client_id[i] = client_id_tmp;
			printf("New client FD: %d\n", connfd_tmp);

			char msg[MAXLINE];
			sprintf(msg, "Welcome to lobby!\n You are the #%d user.\n", i+1);
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
			}else{
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
					sprintf(msg, "(%s left the lobby.\n %d users left)\n", client_id[i], curConn);
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
							sprintf(buf, "user %d:\t%s in lobby\n", j, client_id[j]);
							strcat(sendline, buf);
						}else if(client_id[j] != NULL) {
							sprintf(buf, "user %d:\t%s not in lobby\n", j, client_id[j]);
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