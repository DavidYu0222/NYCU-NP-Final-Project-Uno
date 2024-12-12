#include <fcntl.h>
#include	"unp.h"
#include  <string.h>
#include  <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define WRITE_PATH "/home/yuwei/sfml/MySFMLProject/build/gui_read.fifo"
#define READ_PATH "/home/yuwei/sfml/MySFMLProject/build/gui_write.fifo"

char id[MAXLINE];
int read_fifo_fd;
int write_fifo_fd;

/* the following two functions use ANSI Escape Sequence */
/* refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 */


void clr_scr() {
	printf("\x1B[2J");
};

void set_scr() {		// set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};

void xchg_data(int fp, int sockfd)
{
    int       maxfdp1, stdineof, peer_exit, n;
    fd_set    rset;
    char      sendline[MAXLINE], recvline[MAXLINE];

	
	set_scr();
	clr_scr();
    Writen(sockfd, id, strlen(id));
    printf("sent: %s\n", id);
	readline(sockfd, recvline, MAXLINE);
	printf("recv: %s", recvline);
	Write(write_fifo_fd, recvline, MAXLINE);
	readline(sockfd, recvline, MAXLINE);
	printf("recv: %s", recvline);	
	Write(write_fifo_fd, recvline, MAXLINE);
    stdineof = 0;
	peer_exit = 0;

    for ( ; ; ) {	
		FD_ZERO(&rset);
		maxfdp1 = 0;
		FD_SET(fp, &rset);
		maxfdp1 = fp;
        if (stdineof == 0) {
			//FD_SET(read_fifo_fd, &rset);
		}
		if (peer_exit == 0) {
			FD_SET(sockfd, &rset);
			if (sockfd > maxfdp1)
				maxfdp1 = sockfd;
		};
        maxfdp1++;
        Select(maxfdp1, &rset, NULL, NULL, NULL);
		printf("debug\n");
		if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
			printf("server\n");
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
				printf("\x1B[0;36m%s\x1B[0m", recvline);
				fflush(stdout);
			}
			else { // n < 0
			    printf("(server down)");
				return;
			};
        }
		
        if (FD_ISSET(fp, &rset)) {  /* input is readable */
			printf("gui\n");
            if (readline(fp, sendline, MAXLINE) <= 0) {
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

	if (argc != 3)
		err_quit("usage: tcpcli <IPaddress> <ID>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT+5);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	strcpy(id, argv[2]);
	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	read_fifo_fd = open(READ_PATH, O_RDONLY | O_NONBLOCK);
	write_fifo_fd = open(WRITE_PATH, O_WRONLY | O_NONBLOCK);
	if(read_fifo_fd == -1 || write_fifo_fd == -1){
		perror ("Error opening FIFOs");
		exit(1);
	}
	xchg_data(read_fifo_fd, sockfd);		/* do it all */

	exit(0);
}
