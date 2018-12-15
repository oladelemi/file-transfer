//////////////////////////////////////////////////////////////////////////////////////////
////Author      :  Abdulateef Oladele
/// Description :  server.c -- a stream socket server demo. It Accepts all incoming client
//                 requests until max and sends a requested file if found 
/// Date        : 11-26-2018 
//////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAXDATASIZE 50 // max number of bytes we can get per assignment specifications
#define BACKLOG 10     // how many pending connections queue will hold
int sockfd, new_fd, fd, clientcount, status;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

///////////////////////////////////////////////////////////////////////////////////////
//function : sigint_func
//Description : a signal handler that sends the kill signal to all connected clients if 
//		a server shutdown is initiated
//Return value :nil
///////////////////////////////////////////////////////////////////////////////////////
void sigint_func(int sig) {
    if (send(new_fd, "/kill", 5, 0) == -1)perror("kill");
    close(fd);
    close(new_fd);
    close(sockfd);
    printf("exiting server\n\n");
    exit(0);
}

////////////////////////////////////////////////////////////////////////////////////
////function : child_handler
////Description : A signal handler that handles all clsoing of child processes during
//		  a shutdown
////Return value :nil
///////////////////////////////////////////////////////////////////////////////////
void child_handler(int sig) {
    waitpid(-1, &status, WNOHANG);
    if (WEXITSTATUS(status) > 0) {
        clientcount--;
    }
}

int main(int argc, char *argv[]) {
    int numbytes, counter, pid;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    char buf[MAXDATASIZE];
    socklen_t sin_size;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char *port;
    if (argc != 2) 
	{ // Checks to see if the user passes a port number as parameter and defaults to 56970 if none provided
        printf("Server Port has been set to 56970\n");
        port = "56970";
    	} else {
        port = argv[1];
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }


    printf("server: waiting for connections...\n");
    //int status, clientcount =0;

    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        // to accept incoming connections
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);

        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        if (clientcount < 10) { //ensure no more than 10 client can connect to server
            clientcount++;
        } else {
            close(new_fd);
            continue;
        }

        printf("my clientscount: %d\n", clientcount); // current number of connected clients
        pid = fork();
        if (pid == -1) {
            close(new_fd);
            continue;
        } else if (pid > 0) {
            close(new_fd);
            counter++;
            signal(SIGCHLD, child_handler);
            continue;
        } else if (pid == 0) {

            char buf[100];
            signal(SIGINT, sigint_func); // checks for a kill signal to initiate graceful exit
            counter++; // Increasing count to show all connections that has been made to the 
            //server in a session
            printf("Clients processed: %d\n", counter);

            inet_ntop(their_addr.ss_family,
                      get_in_addr((struct sockaddr *) &their_addr),
                      s, sizeof s);
            printf("server: got connection from '%s'\n", s);

            if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            char *filename = buf + 4;
            printf("File Requested : '%s'\n", filename);

            int dat;
            char transfer[50];
            fd = open(filename, O_RDONLY, 0); // opens file in read mode
            if (fd < 0) { // checks if the file is not found
                printf("File Not found in current Directory\n");
            	printf("server: Waiting for connections.....\n");

                close(new_fd);
                exit(1);
                break;

            }
            while ((dat = read(fd, transfer, 50)) > 0) {
                transfer[dat] = '\0';
                if (send(new_fd, transfer, dat, 0) == -1) {
                    perror("send");

                }
            }
            char *buf4 = "/cmsc257"; // kill string for end of data
            send(new_fd, buf4, strlen(buf4), 0);
            close(new_fd);
            printf("File transfer completed\n");
            printf("Waiting for connections.....\n");
            exit(1);

        }
    }
    return 0;
}


