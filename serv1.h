#ifndef SERV1_H
#define SERV1_H

#include	"unp.h"
#include	"string.h"
#include	<sys/socket.h>
#include	<sys/time.h>
#include    <pthread.h>
#include	<time.h>
#include	<stdbool.h>

#define MAXID 		50
#define CHECK 		1
#define UNCHECK 	0

#define MAXCLIENT 	10
#define MAXROOM 	5
#define MAXMEMBER 	4

#define Clockwise	1
#define CounterClokewise -1

#define TIMEOUT     20

#define Reverse 	10
#define Skip 		11
#define DrawTwo 	12
#define Wild		13
#define DrawFour 	14
#define MAXCARD		108

typedef struct {
	char color;
	int number;
}Card;

typedef struct {
	Card hand[108];
	int size;
}Player;

typedef struct {
    int fd;
	char* id;
}Member;

typedef struct {
	int id;
	int numOfMember;
	Member member[MAXMEMBER];
}RoomData;

void uno_game(int numOfPlayer, Member* members);

#endif // SERV1_H