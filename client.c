//////////////////////////////////////////////////////////////////////////////////
//////Author      :  Abdulateef Oladele
///// Description :  server.c -- a stream socket server demo that sends a filename 
//		to a server and recieve the file from server if found. 
// //// Date      :  11-26-2018
/////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXDATASIZE 50 // max number of bytes we can get per assignment specifications

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

    int sockfd, numbytes;
    char data[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    if (argc != 4) { // exits if arguments are incorrect
        printf("Please add paramaters. Syntax :IP, port, filename\n");
        exit(1);
    }
    char *ip = argv[1];    //  gets IP from parameter
    char *port = argv[2];         // gets PORT number from parameter
    char *filename = argv[3]; // gets filename
    char filenamecmd[100];
    strcpy(filenamecmd, "get ");
    strcat(filenamecmd, filename);  // add get to filename to submit request to server
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    if (send(sockfd, filenamecmd, strlen(filenamecmd), 0) == -1) {
        perror("send");
        close(sockfd);
    }

    int fd, dat;
    fd = open(filename, O_WRONLY | O_CREAT,
              0644); // opens filename in current directory and creates one if it doesn't exist
    if (fd < 0) // if systems encounters an error while creating file
        perror("cannot create file");
    printf("file found | created\n");
    while (1) {
        if ((numbytes = recv(sockfd, data, MAXDATASIZE + 1, 0)) < 1) {
            perror("recv");
            exit(1);
        }
        data[numbytes] = '\0';
        /// terminates write process and exits if kill signal is received from server
        if (strstr(data, "/kill") != NULL) {
            close(fd);
            close(sockfd);
            return 0;
        }
        if (strstr(data, "/cmsc257") != NULL) {
            // checks for end of transmitted file denoted by string
            break;
        } else {
            write(fd, data, numbytes);
        }
    }
    write(fd, data, numbytes - 8);

    close(fd);
    close(sockfd);
    printf("File transfer completed\n");
    return 0;

}

