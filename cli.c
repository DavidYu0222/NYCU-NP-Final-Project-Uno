#include	"unp.h"
#include  <string.h>
#include  <stdio.h>
#include  <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char* hello_msg = "handshake";

void clr_scr() {
	printf("\x1B[2J");
};

void set_scr() {		// set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};

void xchg_data(FILE *fp, int sockfd)
{
    int       maxfdp1, stdineof, peer_exit, n;
    fd_set    rset;
    char      sendline[MAXLINE], recvline[MAXLINE];

	
	set_scr();
	clr_scr();
    Writen(sockfd, hello_msg, strlen(hello_msg));
    stdineof = 0;
	peer_exit = 0;

    for ( ; ; ) {	
		FD_ZERO(&rset);
		maxfdp1 = 0;
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);
			maxfdp1 = fileno(fp);
		};	
		if (peer_exit == 0) {
			FD_SET(sockfd, &rset);
			if (sockfd > maxfdp1)
				maxfdp1 = sockfd;
		};	
        maxfdp1++;
        Select(maxfdp1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
			n = read(sockfd, recvline, MAXLINE);
			if (n == 0) {
 		   		if (stdineof == 1)
                    return;         /* normal termination */
		   		else {
					printf("(End of input from the peer!)");
					peer_exit = 1;
					return;
				};
            }
			else if (n > 0) {
				recvline[n] = '\0';
				char *line, *token;
				char *lineSavePtr, *tokenSavePtr;

				line = strtok_r(recvline, "\n", &lineSavePtr);
				while(line != NULL){
					char tmp[MAXLINE];
					strcpy(tmp, line);
					token = strtok_r(tmp, " ", &tokenSavePtr);
					if(line[0] == '('){
						printf("\x1B[0;36m%s\n\x1B[0m", line);
					}else if(strcmp(line, "br") == 0){
						printf("\n");
					}else if(strcmp(token, "Error:") == 0){
						printf("\x1B[0;91m%s\n\x1B[0m", line);
					}else{
						printf("\x1B[0;93m%s\n\x1B[0m", line);
					}
					line = strtok_r(NULL, "\n", &lineSavePtr);
				}
				
				fflush(stdout);
			}
			else { // n < 0
			    printf("(server down)");
				return;
			};
        }
		
        if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */

            if (Fgets(sendline, MAXLINE, fp) == NULL) {
				if (peer_exit)
					return;
				else {
					printf("(leaving...)\n");
					stdineof = 1;
					Shutdown(sockfd, SHUT_WR);      /* send FIN */
				};
            }
			else {
				n = strlen(sendline);
				sendline[n] = '\n';
				Writen(sockfd, sendline, n+1);
			};
        }
    }
};

int
main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT+6);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	xchg_data(stdin, sockfd);

	exit(0);
}
