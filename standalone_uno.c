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

int* printHand(Player* player) {
    int size = player->size;
    int* optmap = malloc((size+1) * sizeof(int));
    int index = 1;
    printf("0: draw\n");
    for(int i = 0; i < MAXCARD && index <= size; i++){
        char color = player->hand[i].color;
        int number = player->hand[i].number;
        char buf[MAXLINE];
        if(color == '\0'){
            continue;
        }
        else if(color == 'R' || color == 'G' || color == 'B' || color == 'Y'){
            if(number == DrawTwo)
                sprintf(buf, "%d: %c DrawTwo", index, color);
            else if(number == Reverse)
                sprintf(buf, "%d: %c Reverse", index, color);
            else if(number == Skip)
                sprintf(buf, "%d: %c Skip", index, color);
            else
                sprintf(buf, "%d: %c%d", index, color, number);
        }
        else if(color == 'N'){
            if(number == Wild)
                sprintf(buf, "%d: Wild", index);
            else
                sprintf(buf, "%d: DrawFour", index);
        }
        printf("%s\n", buf);
        optmap[index] = i;
        index++;
    }
    return optmap;
}

// void waitPlayer(Card* Deck, int* top, int* sptr, int* eptr, int* order, Player curPlayer){
    
// }

void uno_game(int numOfPlayer, Member* members){
    Player* players = calloc(numOfPlayer, sizeof(Player));
    Card Deck[MAXCARD];
    Card top;
    int sptr = 0;
    int eptr = 0;
    int gameover = 0;
    int order = Clockwise;
    int skip = 0;
    int drawTwo = 0;
    int curPlayer = 0;

    initDeck(Deck);

    shuffleDeck(Deck, MAXCARD);

    for (int i = 0; i < 7; i++){   
        for(int j = 0; j < numOfPlayer; j++){
            dealCard(Deck, &sptr, &players[j]);
        }
    }

    // for(int i = 0; i < numOfPlayer; i++){
    //     printf("Player %d's hand:\n", i+1);
    //     printHand(&players[i]);
    // }

    int       maxfdp;
    fd_set    rset;
	struct timeval tv;
    int       n;
	char	  buf[MAXLINE];

    top = Deck[sptr++];
    while(top.color == 'N'){
        Deck[eptr++] = top;
        top = Deck[sptr++];
    }
        
    while(gameover != 1){
        printf("Now is Player %d's round!\n", curPlayer);
        printf("Current top is %c%d\n", top.color, top.number);
        if(drawTwo == 1){
            printf("Player %d draw two\n", curPlayer);
            drawTwo = 0;
            dealCard(Deck, &sptr, &players[curPlayer]);
            dealCard(Deck, &sptr, &players[curPlayer]);
        }
        if(skip == 1){
            printf("Skip Player %d!\n", curPlayer);
            skip = 0;
            curPlayer = (curPlayer + order) % numOfPlayer;
            continue;
        }
        int* optmap = printHand(&players[curPlayer]);

        clock_t start, end;
        double player_time_used = 0;
        start = clock();
        for(;player_time_used <= TIMEOUT;){
            FD_ZERO(&rset);
            FD_SET(0, &rset);
            maxfdp = 1;
		    tv.tv_sec = 1;
		    tv.tv_usec = 0;

		    Select(maxfdp, &rset, NULL, NULL, &tv);
            end = clock();
            player_time_used = ((double)(end - start)) / 100;

            if(FD_ISSET(0, &rset)){
				if ( (n = Read(0, buf, MAXLINE)) == 0) {
					printf("Read error\n");
					continue;
				}
				buf[n] = '\0';
                char *token = strtok(buf, " \n");
                if(token == NULL){
                    printf("Invalid Option\n");
                    continue;
                }
				int opt = atoi(token);
                if(opt == 0){
                    dealCard(Deck, &sptr, &players[curPlayer]);
                    break;
                }else if(opt < 0 || opt > players[curPlayer].size ){
                    printf("Invalid Option\n");
                }else if(players[curPlayer].hand[optmap[opt]].color == 'N'){
                    char *token = strtok(NULL, " \n");
                    if(token == NULL){
                        printf("You can use \"%d Red\" to wild the color\n", opt);
                    }else if(token[0] == 'R' || token[0] == 'G' || token[0] == 'B' || token[0] == 'Y'){
                        Deck[eptr++] = players[curPlayer].hand[optmap[opt]];
                        players[curPlayer].hand[optmap[opt]].color = token[0];
                        top = players[curPlayer].hand[optmap[opt]];
                        players[curPlayer].hand[optmap[opt]].color = '\0';
                        players[curPlayer].hand[optmap[opt]].number = 0;
                        players[curPlayer].size--;
                        break;
                    }else{
                        printf("\"%s\" is not a valid color\n", token);
                    }
                }else if((players[curPlayer].hand[optmap[opt]].color != top.color) && (players[curPlayer].hand[optmap[opt]].number != top.number)){
                    printf("Color or Number not match\n");
                }else{
                    if(players[curPlayer].hand[optmap[opt]].number == Skip){
                        skip = 1;
                    }
                    if(players[curPlayer].hand[optmap[opt]].number == DrawTwo){
                        skip = 1;
                        drawTwo = 1;
                    }
                    if(players[curPlayer].hand[optmap[opt]].number == Reverse){
                        order = -1 * order;
                    }
                    Deck[eptr++] = players[curPlayer].hand[optmap[opt]];
                    top = players[curPlayer].hand[optmap[opt]];
                    players[curPlayer].hand[optmap[opt]].color = '\0';
                    players[curPlayer].hand[optmap[opt]].number = 0;
                    players[curPlayer].size--;
                    break;
                }
            }
        }
        //printf("Used time: %f\n", player_time_used);
        if(player_time_used > TIMEOUT){
            printf("TIMEOUT\n");
            dealCard(Deck, &sptr, &players[curPlayer]);
        }
        if(players[curPlayer].size == 1){
            printf("Player %d Uno!\n", curPlayer);
        }
        if(players[curPlayer].size == 0){
            printf("Player %d Win!\n", curPlayer);
            gameover = 1;
        }

        curPlayer = (curPlayer + order) % numOfPlayer;
        if(curPlayer < 0) curPlayer = numOfPlayer - 1;
        free(optmap);
        //waitPlayer(Deck, &top, &sptr, &eptr, &order, &players[curPlayer]);
    }

    free(players);

    return;
}

int main(){
    // char    sendline[MAXLINE];
    // Fgets(sendline, MAXLINE, stdin);

    // char *token = strtok(sendline, " \n");
    // printf("_%s_\n", token);
    // token = strtok(NULL, " ");
    // int id = atoi(token);
    // printf("_%d_\n", id);

    uno_game(3, NULL);
    return 0;
}
