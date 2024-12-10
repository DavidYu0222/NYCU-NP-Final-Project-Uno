#include "serv1.h"

void initDeck(Card* Deck){
    char colors[] = {'R', 'G' , 'B', 'Y'};
    for(int i = 0; i < 4; i++){
        for(int j = 1; j <= 12; j++) {
            for(int k = 0; k < 2; k++){
                Deck[25*i + 2*j + k - 1].number = j;
                Deck[25*i + 2*j + k - 1].color = colors[i];
            }
        }
        Deck[25*i].number = 0;
        Deck[25*i].color = colors[i];
    }
    for(int i = 100; i < 104; i++){
        Deck[i].color = 'N';
        Deck[i].number = DrawFour;
    }
    for(int i = 104; i < 108; i++){
        Deck[i].color = 'N';
        Deck[i].number = Wild;
    }
}

void shuffleDeck(Card* Deck, int size) {
    srand(time(NULL));

    for(int i = size - 1; i > 0; i--){
        int j = rand()%(i + 1);
        Card tmp = Deck[i];
        Deck[i] = Deck[j];
        Deck[j] = tmp;
    }
};

void dealCard(Card* Deck, int* sptr, Player* player) {
    for(int i = 0; i < MAXCARD; i++){
        if(player->hand[i].color == '\0'){
            player->size++;
            player->hand[i] = Deck[*sptr];
            *sptr = (*sptr + 1) % MAXCARD;
            break;
        }
    }
}

int* printHand(Player* player, int fd) {
    int size = player->size;
    int* optmap = malloc((size+1) * sizeof(int));
    int index = 1;
    char sendline[MAXLINE];
    bzero(sendline, sizeof(sendline));
    strcat(sendline, "0: draw\n");
    for(int i = 0; i < MAXCARD && index <= size; i++){
        char color = player->hand[i].color;
        int number = player->hand[i].number;
        char buf[20];
        if(color == '\0'){
            continue;
        }
        else if(color == 'R' || color == 'G' || color == 'B' || color == 'Y'){
            if(number == DrawTwo)
                sprintf(buf, "%d: %c DrawTwo\n", index, color);
            else if(number == Reverse)
                sprintf(buf, "%d: %c Reverse\n", index, color);
            else if(number == Skip)
                sprintf(buf, "%d: %c Skip\n", index, color);
            else
                sprintf(buf, "%d: %c%d\n", index, color, number);
        }
        else if(color == 'N'){
            if(number == Wild)
                sprintf(buf, "%d: Wild\n", index);
            else
                sprintf(buf, "%d: DrawFour\n", index);
        }
        strcat(sendline, buf);
        optmap[index] = i;
        index++;
    }
    Writen(fd, sendline, strlen(sendline));
    return optmap;
}

void printStatus(int numOfPlayer, Player* players, Member* members, Card top, int fd){
    char color = top.color;
    int number = top.number;
    char buf[30];
    char sendline[MAXLINE];

    if(number == DrawTwo)
        sprintf(buf, "Current top is %c DrawTwo\n", color);
    else if(number == Reverse)
        sprintf(buf, "Current top is %c Reverse\n", color);
    else if(number == Skip)
        sprintf(buf, "Current top is %c Skip\n", color);
    else if(number == Wild)
        sprintf(buf, "Current top is %c Wild\n", color);
    else if(number == DrawFour)
        sprintf(buf, "Current top is %c DrawFour\n", color);
    else
        sprintf(buf, "Current top is %c%d\n", color, number);

    if(numOfPlayer == 2){
        sprintf(sendline, "Player 1(%s): %d | Player 2(%s): %d\n", members[0].id, players[0].size, members[1].id, players[1].size);
    }else if(numOfPlayer == 3){
        sprintf(sendline, "Player 1(%s): %d | Player 2(%s): %d | Player 3(%s): %d\n", members[0].id, players[0].size, members[1].id, players[1].size, members[2].id, players[2].size);
    }else{
        sprintf(sendline, "Player 1(%s): %d | Player 2(%s): %d | Player 3(%s): %d | Player 4(%s): %d\n", members[0].id, players[0].size, members[1].id, players[1].size, members[2].id, players[2].size, members[3].id, players[3].size);
    }

    strcat(sendline, buf);
    if(fd == -1){
        for(int i = 0; i < numOfPlayer; i++){
            Writen(members[i].fd, sendline, strlen(sendline));
        }
    }else{
        Writen(fd, sendline, strlen(sendline));
    }
    
}

bool isNumber(char *str) {
    if (str == NULL || *str == '\0') return false;

    while (*str) {
        if (!isdigit((unsigned char)*str)) return false;
        str++;
    }
    
    return true; 
}


Status uno_game(int numOfPlayer, Member* members){
    Player* players = calloc(numOfPlayer, sizeof(Player));
    Status return_status;
    Card Deck[MAXCARD];
    Card top;
    int sptr = 0;
    int eptr = 0;
    int gameover = 0;
    int order = Clockwise;
    int skip = 0;
    int drawTwo = 0;
    int drawFour = 0;
    int curPlayer = 0;

    int       maxfdp;
    fd_set    rset;
	struct timeval tv;
    int       n;
	char	  recvline[MAXLINE];
    char      sendline[MAXLINE];
    char* invalid_error_msg0 = "Invalid Option(Null String)\n";
    char* invalid_error_msg1 = "Invalid Option(Not A Number)\n";
    char* invalid_error_msg2 = "Invalid Option(Out Of Range)\n";
    char* invalid_error_msg3 = "Color or Number Not Match\n";
    char* notified_msg0 = "\nYou Timeout\n";
    char* notified_msg1 = "\nYou Uno!\n";
    char* notified_msg2 = "\nYou Win!\n";
    char* notified_msg3 = "\nNow is Your turn!\n";
    char* notified_msg4 = "\nYou draw two cards!";
    char* notified_msg5 = "\nYou draw four cards!";
    char* notified_msg6 = "\nYour turn is skipped!\n";
    
    initDeck(Deck);

    shuffleDeck(Deck, MAXCARD);

/* Deal each player 7 cards*/
    for (int i = 0; i < 7; i++){   
        for(int j = 0; j < numOfPlayer; j++){
            dealCard(Deck, &sptr, &players[j]);
        }
    }

/* Ensure the top card is not Wild or DrawFour*/
    top = Deck[sptr++];
    while(top.color == 'N'){
        Deck[eptr++] = top;
        top = Deck[sptr++];
    }

/* Game start*/
    while(gameover != 1){
    /* Deal Skip and DrawTwo (played by the previous player)*/
        if(drawTwo == 1){
            sprintf(sendline, "\nPlayer %d(%s) draw two", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg4, strlen(notified_msg4));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
            drawTwo = 0;
            dealCard(Deck, &sptr, &players[curPlayer]);
            dealCard(Deck, &sptr, &players[curPlayer]);
        }
        if(drawFour == 1){
            sprintf(sendline, "\nPlayer %d(%s) draw four", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg5, strlen(notified_msg5));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
            drawFour = 0;
            dealCard(Deck, &sptr, &players[curPlayer]);
            dealCard(Deck, &sptr, &players[curPlayer]);
            dealCard(Deck, &sptr, &players[curPlayer]);
            dealCard(Deck, &sptr, &players[curPlayer]);
        }
        if(skip == 1){
            sprintf(sendline, "\nPlayer %d(%s)'s turn is skipped!\n", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg6, strlen(notified_msg6));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
            skip = 0;
            curPlayer = (curPlayer + order) % numOfPlayer;
            if(curPlayer < 0) curPlayer = numOfPlayer - 1;
            continue;
        }
    /* Notify players whose turn it is now */
        sprintf(sendline, "\nNow is Player %d(%s)'s turn!\n", curPlayer+1, members[curPlayer].id);
        for(int i = 0; i < numOfPlayer; i++){
            if(i == curPlayer){
                Writen(members[i].fd, notified_msg3, strlen(notified_msg3));
            }else{
                Writen(members[i].fd, sendline, strlen(sendline));
            }
        }
        printStatus(numOfPlayer, players, members, top, -1);
    /* Show the hands of the current player */
        int* optmap = printHand(&players[curPlayer], members[curPlayer].fd);

    /* Wait the current player */
        struct timeval start, end;
        double player_time_used = 0;
        gettimeofday(&start, NULL);
        for(;player_time_used <= TIMEOUT;){
            FD_ZERO(&rset);
            int maxfd = 0;
            for(int i = 0; i < MAXMEMBER; i++){
                if(members[i].fd != 0){
                    FD_SET(members[i].fd, &rset);
                    maxfd = max(members[i].fd, maxfd);
                }
            }
            maxfdp = maxfd + 1;

		    tv.tv_sec = 1;
		    tv.tv_usec = 0;
		    Select(maxfdp, &rset, NULL, NULL, &tv);
            gettimeofday(&end, NULL);
            player_time_used = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            
        /* Deal the message from the current player*/
            if(FD_ISSET(members[curPlayer].fd, &rset)){
				if((n = Read(members[curPlayer].fd, recvline, MAXLINE)) == 0) {
					return_status.status = DISCONN;
                    return_status.index = curPlayer;
					return return_status;
				}
				recvline[n] = '\0';
                char *token = strtok(recvline, " \n");
                if(token == NULL){
                    Writen(members[curPlayer].fd, invalid_error_msg0, strlen(invalid_error_msg0));
                    continue;
                }else if(!isNumber(token)){
                    Writen(members[curPlayer].fd, invalid_error_msg1, strlen(invalid_error_msg1));
                    continue;
                }
				int opt = atoi(token);
                if(opt == 0){
                    dealCard(Deck, &sptr, &players[curPlayer]);
                    break;
                }else if(opt < 0 || opt > players[curPlayer].size ){
                    Writen(members[curPlayer].fd, invalid_error_msg2, strlen(invalid_error_msg2));
                }else if(players[curPlayer].hand[optmap[opt]].color == 'N'){
                    char *token = strtok(NULL, " \n");
                    if(token == NULL){
                        sprintf(sendline, "You can use \"%d Red\" to wild the color\n", opt);
                        Writen(members[curPlayer].fd, sendline, strlen(sendline));
                    }else if(token[0] == 'R' || token[0] == 'G' || token[0] == 'B' || token[0] == 'Y'){
                        if(players[curPlayer].hand[optmap[opt]].number == DrawFour){
                            drawFour = 1;
                            skip = 1;
                        }
                        Deck[eptr++] = players[curPlayer].hand[optmap[opt]];
                        players[curPlayer].hand[optmap[opt]].color = token[0];
                        top = players[curPlayer].hand[optmap[opt]];
                        players[curPlayer].hand[optmap[opt]].color = '\0';
                        players[curPlayer].hand[optmap[opt]].number = 0;
                        players[curPlayer].size--;
                        break;
                    }else{
                        sprintf(sendline, "\"%s\" is not a valid color\n", token);
                        Writen(members[curPlayer].fd, sendline, strlen(sendline));
                    }
                }else if((players[curPlayer].hand[optmap[opt]].color != top.color) && (players[curPlayer].hand[optmap[opt]].number != top.number)){
                    Writen(members[curPlayer].fd, invalid_error_msg3, strlen(invalid_error_msg3));
                }else{
                    if(players[curPlayer].hand[optmap[opt]].number == Skip){
                        skip = 1;
                    }
                    if(players[curPlayer].hand[optmap[opt]].number == DrawTwo){
                        skip = 1;
                        drawTwo = 1;
                    }
                    if(players[curPlayer].hand[optmap[opt]].number == Reverse){
                        if(numOfPlayer == 2){
                            skip = 1;
                        }else{
                            order = -1 * order;
                        }
                    }
                    Deck[eptr++] = players[curPlayer].hand[optmap[opt]];
                    top = players[curPlayer].hand[optmap[opt]];
                    players[curPlayer].hand[optmap[opt]].color = '\0';
                    players[curPlayer].hand[optmap[opt]].number = 0;
                    players[curPlayer].size--;
                    break;
                }
            }
            for(int i = 0; i < MAXMEMBER; i++){
                if(i != curPlayer && FD_ISSET(members[i].fd, &rset)){
                    if((n = Read(members[i].fd, recvline, MAXLINE)) == 0) {
                        return_status.status = DISCONN;
                        return_status.index = i;
                        return return_status;
                    }
                }
            }
        }
    /* Current player run out of its time */
        if(player_time_used > TIMEOUT){
            dealCard(Deck, &sptr, &players[curPlayer]);
            sprintf(sendline, "\nPlayer %d(%s) Timeout\n", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg0, strlen(notified_msg0));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
        }
    /* Check whether the current player uno */
        if(players[curPlayer].size == 1){
            sprintf(sendline, "\nPlayer %d(%s) Uno!\n", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg1, strlen(notified_msg1));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
        }
    /* Check gameover*/
        if(players[curPlayer].size == 0){
            gameover = 1;
            sprintf(sendline, "\nPlayer %d(%s) Win!\n", curPlayer+1, members[curPlayer].id);
            for(int i = 0; i < numOfPlayer; i++){
                if(i == curPlayer){
                    Writen(members[i].fd, notified_msg2, strlen(notified_msg2));
                }else{
                    Writen(members[i].fd, sendline, strlen(sendline));
                }
            }
        }
    /* Calculate the Next players*/
        curPlayer = (curPlayer + order) % numOfPlayer;
        if(curPlayer < 0) curPlayer = numOfPlayer - 1;
        free(optmap);
        //printf("Used time: %f\n", player_time_used);
    }

    free(players);

    return_status.status = OK;

    return return_status;
}
