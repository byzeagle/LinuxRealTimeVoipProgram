#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#define PORT 10000

int main(){

	pid_t pid;

	pid = fork();
	if(pid < 0){
		perror("fork()");
		exit(EXIT_FAILURE);
	}

	if(pid > 0){
		exit(EXIT_SUCCESS);
	}

	if(setsid() < 0){
		perror("setsid()");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if(pid < 0){
		exit(EXIT_FAILURE);
	}

	if(pid > 0){
		exit(EXIT_SUCCESS);
	}

	umask(0);

	chdir("/");

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close (x);
    }

	int sockfd;
	ssize_t bytesRead;
	ssize_t bytesSent;

	struct sockaddr_in server;

	// Public IP addresses
	struct sockaddr_in client_1;
	struct sockaddr_in client_2;
	socklen_t client_addr_len_1 = sizeof(client_1);
	socklen_t client_addr_len_2 = sizeof(client_2);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Socket");
		exit(EXIT_FAILURE);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);

	char address[100];

	if (bind(sockfd, (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	openlog ("UDP-Hole-Punching-Server", LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	syslog (LOG_NOTICE, "Daemon has started with userid : %d", getuid ());

	while(1){
		if ((bytesRead = recvfrom(sockfd, NULL, 0, 0, (struct sockaddr*) &client_1, &client_addr_len_1)) < 0) {
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}

		inet_ntop(AF_INET, &(client_1.sin_addr), address, sizeof(address));
		syslog(LOG_INFO, "Client IP address: %s", address);

		if ((bytesSent = recvfrom(sockfd, NULL, 0, 0, (struct sockaddr*) &client_2, &client_addr_len_2)) < 0) {
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}

		inet_ntop(AF_INET, &(client_2.sin_addr), address, sizeof(address));
		syslog(LOG_INFO, "Client IP address: %s", address);

		if((bytesSent = sendto(sockfd, &client_1, sizeof(client_1), 0, (struct sockaddr*) &client_2, client_addr_len_2)) < 0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}

		if((bytesSent = sendto(sockfd, &client_2, sizeof(client_2), 0, (struct sockaddr*) &client_1, client_addr_len_1)) < 0){
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
	}

	closelog ();
	return 0;
}