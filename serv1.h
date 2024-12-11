#ifndef SERV1_H
#define SERV1_H

#include	"unp.h"
#include    <stdio.h>
#include	"string.h"
#include	<sys/socket.h>
#include	<sys/time.h>
#include    <pthread.h>
#include	<time.h>
#include	<stdbool.h>
#include    <ctype.h>

// Server setting constant
#define MAXCLIENT 	10
#define MAXROOM 	5
#define MAXMEMBER 	4
#define MAXID 		50

// Game constant
#define Clockwise	1
#define CounterClokewise -1

#define TIMEOUT     20

#define Reverse 	10
#define Skip 		11
#define DrawTwo 	12
#define Wild		13
#define DrawFour 	14
#define MAXCARD		108

// Check flag
#define CHECK 		1
#define UNCHECK 	0

// Struct Status.status
#define OK 			0
#define DISCONN 	1

// Struct RoomData.status
#define IDLE false
#define BUSY true

// Verify login
#define MATCH 				0
#define USER_NOT_EXIST 		1
#define PASSWD_NOT_MATCH 	2

// login system status (FSM)
#define CHOOSE_OPTION 	 0
#define SIGN_IN_USERNAME 1
#define SIGN_IN_PASSWORD 2
#define SIGN_UP_USERNAME 3
#define SIGN_UP_PASSWORD 4
#define SIGN_UP_RETYPE   5
#define LOGIN_SUCCESS    6 

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
	bool status;
	int id;
	int numOfMember;
	Member member[MAXMEMBER];
}RoomData;

typedef struct {
	int login_id;
	int fd;
	char* id;
}LoginData;

typedef struct {
	int status;
	int index;
}Status;

Status uno_game(int numOfPlayer, Member* members);

#endif // SERV1_H