#include	"serv1.h"

pthread_t rooms[MAXROOM];			 // room's thread
int busy_rooms[MAXROOM];			 // non-empty room flag

RoomData room_datas[MAXROOM];		 // the user data pass from lobby to room
int room_check_flag[MAXROOM];		 // fast check flag for room
pthread_mutex_t mutex_room[MAXROOM]; // protect room_datas room_check_flag

int back_to_lobby_list[MAXCLIENT];	 // the user fd pass from room to lobby
int free_rooms[MAXROOM];			 // the empty room need to recycle
int check_flag = CHECK;				 // fast check flag
pthread_mutex_t mutex_free_list;	 // protect back_to_lobby_list, free_rooms, check_flag

void init(){
	bzero(back_to_lobby_list, sizeof(back_to_lobby_list));
	bzero(busy_rooms, sizeof(busy_rooms));
	bzero(room_datas, sizeof(room_datas));
	bzero(room_check_flag, sizeof(room_check_flag));

	for(int i = 0; i < MAXROOM; i++){
		free_rooms[i] = -1;
	}

	pthread_mutex_init(&mutex_free_list, NULL);

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
					/* copy the members data from global to local */ 
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
				for(int i = 0; i < MAXMEMBER; i++){
					if( members[i].fd == 0 ) 
						continue;
					if( i == flag[j] ){
						Writen(members[i].fd, enter_msg, strlen(enter_msg));
						continue;
					}
					sprintf(buf, "(%s enter the room)\n", members[flag[j]].id);
					Writen(members[i].fd, buf, strlen(buf));
				}
			}
		}
		tmp = rset;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		Select(maxfdp, &tmp, NULL, NULL, &tv);
/* Process members's messages */
		for(int i = 0; i < MAXMEMBER; i++) {	
			if(members[i].fd != 0 && FD_ISSET(members[i].fd, &tmp)){
				if ( (n = Read(members[i].fd, buf, MAXLINE)) == 0) {
					printf("Read error\n");
					continue;
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
						}
					}
					pthread_mutex_unlock(&mutex_room[room_id]);
				/* Add member's fd to back_to_lobby_list */
					pthread_mutex_lock(&mutex_free_list);
					for (int k = 0; k < MAXCLIENT; k++){
						if(back_to_lobby_list[k] == 0){
							back_to_lobby_list[k] = members[i].fd;
							break;
						}
					}
					check_flag = UNCHECK;
					pthread_mutex_unlock(&mutex_free_list);
				/* Clear data in local*/
					FD_CLR(members[i].fd, &rset);
					numOfMember--;
					members[i].fd = 0;
					members[i].id = NULL;
					for(int i = 0; i < MAXMEMBER - 1; i++){
						if(members[i].fd == 0){
							for(int j = i ; j < MAXMEMBER - 1; j++){
								members[j] = members[j+1];
							}
						}
					}
				}
				else if(strcmp(buf, "start_uno\n\n") == 0){
					uno_game(numOfMember, members);
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
	pthread_mutex_lock(&mutex_free_list);
	for(int i = 0; i < MAXROOM; i++){
		if (free_rooms[i] == -1){
			free_rooms[i] = room_id;
			break;
		}
	}
	check_flag = UNCHECK;
	pthread_mutex_unlock(&mutex_free_list);

	pthread_exit(NULL);
}



int
main(int argc, char **argv)
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
	int curConn=0; 
    int idle_list[MAXCLIENT];
    int online_list[MAXCLIENT];

	bzero(&online_list, sizeof(online_list));
	bzero(&idle_list, sizeof(idle_list));

	char* 	  usr_id[MAXCLIENT];	// store users' ID

	char* 	  bye_msg = "Bye!\n";
	char*	  create_room_error_msg = "(CREATE ROOM FAIL!)\n";
	char*	  enter_room_error_msg1 = "(Please specify the room id!)\n";
	char*	  enter_room_error_msg2 = "(Room not exist!)\n";
	char*	  enter_room_error_msg3 = "(Room is full!)\n";

	int       maxfdp;
    fd_set    tmp, rset;
	struct timeval tv;
	char	  buf[MAXLINE];
	char	  cmd[MAXLINE];

	init();

	FD_ZERO(&rset);
	FD_SET(0 , &rset);
    FD_SET(listenfd, &rset);
    maxfdp = listenfd + 1;
	for ( ; ; ) {
/* Check whether someone goes back to lobby or room need to free */
        if(pthread_mutex_trylock(&mutex_free_list) == 0){
            if(check_flag == UNCHECK){
                for(int i = 0; i < MAXCLIENT; i++){
                    if (back_to_lobby_list[i] != 0){
						FD_SET(back_to_lobby_list[i], &rset);
						for(int j = 0; j < MAXCLIENT; j++){
							if(idle_list[j] == back_to_lobby_list[i]){
								online_list[j] = 1;
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
                check_flag = CHECK;
				printf("CHECK OK\n");
            }
            pthread_mutex_unlock(&mutex_free_list);
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

			char* usr_id_tmp = malloc(MAXID);
			if ( (n = Read(connfd_tmp, usr_id_tmp, MAXID)) == 0) {
				printf("Read error\n");
				continue;
			}
			printf("New client ID: %s\n", usr_id_tmp);

			int i;
			for( i = 0; i < MAXCLIENT; i++) {
				if(idle_list[i] == 0){
					idle_list[i] = connfd_tmp;
					online_list[i] = 1;
					break;
				}
			}
			curConn++;
			usr_id[i] = usr_id_tmp;
			printf("New client FD: %d\n", connfd_tmp);

			char msg[MAXLINE];
			sprintf(msg, "You are the #%d user.\nYou may now type in or wait for other user.\n", i+1);
			Writen(idle_list[i], msg, MAXLINE);

			sprintf(msg, "(#%d user %s enters lobby)\n", i+1, usr_id[i]);
			for(int j = 0; j < MAXCLIENT; j++) {
				if(online_list[j] == 0 || i == j ){
					continue;
				}
				Writen(idle_list[j], msg, MAXLINE);
			}
            FD_SET(idle_list[i], &rset);
			maxfdp = max(idle_list[i], maxfdp - 1) + 1;
		}
		if(FD_ISSET(0, &tmp)){
			char    sendline[MAXLINE];
    		Fgets(sendline, MAXLINE, stdin);
			if(strcmp(sendline, "exit\n") == 0){
				break;
			}
			for(int i = 0; i < MAXROOM; i++){
				check_data(i);
				printf("\n");
			}
		}
/* Process other users' messages */
		for(int i = 0; i < MAXCLIENT; i++) {	
			if(idle_list[i] != 0 && FD_ISSET(idle_list[i], &tmp)){	
				if ( (n = Read(idle_list[i], buf, MAXLINE)) == 0) {
			/* Connection interrupt */
					Writen(idle_list[i], bye_msg, strlen(bye_msg));
					curConn--;
					char msg[MAXLINE];
					if(curConn == 1)
						sprintf(msg, "(%s left the room. You are the last one. Press Ctrl+D to leave or wait for a new user.)\n", usr_id[i]);
					else
						sprintf(msg, "(%s left the room. %d users left)\n", usr_id[i], curConn);
					for(int j = 0; j < MAXCLIENT; j++) {
						if(online_list[j] == 0 || i == j){
							continue;
						}
						Writen(idle_list[j], msg, MAXLINE);
					}
                    FD_CLR(idle_list[i], &rset);
					Close(idle_list[i]);
					idle_list[i] = 0;
					online_list[i] = 0;
                    int maxfd = listenfd;
                    for(int j = 0; j < MAXCLIENT; j++) {
                        if(online_list[j] != 0){
                            maxfd = max(idle_list[j], maxfd);
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
							room_datas[k].id = k;
							room_datas[k].numOfMember = 1;
							room_datas[k].member[0].fd = idle_list[i];
							room_datas[k].member[0].id=usr_id[i];
							FD_CLR(idle_list[i], &rset);
							online_list[i] = 0;
							printf("Remove FD:%d from rset\n", idle_list[i]);
							printf("Create room %d\n", k);
							pthread_create(&rooms[k], NULL, play_room, &room_datas[k]);
							flag = 0;
							break;
						}	
					}
					if(flag == 1)
						Writen(idle_list[i], create_room_error_msg, strlen(create_room_error_msg));
				}
			/* Enter room */
				else if(strcmp(token, "enter_room") == 0){
					token = strtok(NULL, " \n");
					if(token == NULL){
						Writen(idle_list[i], enter_room_error_msg1, strlen(enter_room_error_msg1));
						continue;
					}
					int room_id = atoi(token);

					if(room_id < 0 || room_id > MAXROOM - 1 || busy_rooms[room_id] == 0){
						Writen(idle_list[i], enter_room_error_msg2, strlen(enter_room_error_msg2));
						continue;
					}
					pthread_mutex_lock(&mutex_room[room_id]);
					int num = room_datas[room_id].numOfMember;
					if(num == 4){
						pthread_mutex_unlock(&mutex_room[room_id]);
						Writen(idle_list[i], enter_room_error_msg3, strlen(enter_room_error_msg3));
						continue;
					}
					for (int j = 0; j < MAXMEMBER; j++){
						if(room_datas[room_id].member[j].fd == 0){
							room_datas[room_id].member[j].fd = idle_list[i];
							room_datas[room_id].member[j].id = usr_id[i];
							break;
						}
					}
					room_datas[room_id].numOfMember++;
					room_check_flag[room_id]++;
					FD_CLR(idle_list[i], &rset);
					online_list[i] = 0;
					printf("Remove FD:%d from rset\n", idle_list[i]);
					printf("Enter room %d\n", room_id);
					pthread_mutex_unlock(&mutex_room[room_id]);
				}
			/* Normal message passing */
				else{
					char *usr_msg = malloc(strlen(usr_id[i]) + strlen(buf) + 4);
					sprintf(usr_msg, "(%s) %s", usr_id[i], buf);

					for(int j = 0; j < MAXCLIENT; j++) {
						if(online_list[j] == 0 || i == j){
							continue;
						}
						Writen(idle_list[j], usr_msg, strlen(usr_msg));
					}
					free(usr_msg);
				}
				
			}
		}
	}

	for(int i = 0; i < MAXCLIENT; i++){
		if(idle_list[i] != 0)
		 	Close(idle_list[i]);
	}

	pthread_mutex_destroy(&mutex_free_list);
	for(int i = 0; i < MAXROOM; i++){
		pthread_mutex_destroy(&mutex_room[i]);
	}
	return 0;
}