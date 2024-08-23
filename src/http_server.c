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


#define PORT "80"       // the port users will be connecting to

#define BACKLOG 20      // how many pending connections queue will hold

#define BUFFER 5000     // size of the buffers to send and recieve data


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    char request[BUFFER];       // buffer to hold the request

    char response[BUFFER];      // buffer to hold the response

    int bytes;                  // number of bytes

    char *port;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP


    port = argv[1];

    printf("Port: %s \n", port);

    if(port == NULL){

        port = PORT;

    }

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
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

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        memset(request, '\0', BUFFER);      // setup request buffer with NULL characters


        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            recv(new_fd, request, BUFFER, 0);       // recieves message and stores it into request buffer
    
            char *request_ = request;       // get pointer to each char in request

            printf("Original request: %s", request_);
            
            int temp = 0;                   // init temp variable

            // loop through until a NULL or space character is found and increment temp
            while (request_[temp] != '\0' && request_[temp] != ' ') {
               
               temp++;
           
            }

            // check for a get request
            if (strncmp(request_, "GET /", 5) != 0) {
               strcpy(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
               if (send(new_fd, response, strlen(response), 0) == -1) {
                   perror("send");
               }
               exit(1);
            }

            printf("Full Request: %s", request);
            printf("Full Request: %s", request_);

            request_ = request + 5;      // skip 4 characters in request 

            printf("Full Request: %s", request_);

            temp = 0;                    // reset temp counter

            // loop through until a NULL or space character is found and increment temp
            while (request_[temp] != '\0' && request_[temp] != ' ') {
               
               temp++;
           
            }



            if (request_[temp] == '\0') {
               strcpy(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
               if (send(new_fd, response, strlen(response), 0) == -1) {
                   perror("send");
               }
               exit(1);
            }


            char file_path[BUFFER];

            strncpy(file_path, request_, temp);

            printf("First char in path: %c \n", file_path[0]);

            printf("File path: %s \n", file_path);

            file_path[temp] = '\0';

            printf("First char in path: %c \n", file_path[0]);

            printf("File path: %s \n", file_path);

            FILE *file = fopen(file_path, "rb");

            if (!file) {
               strcpy(response, "HTTP/1.1 404 File Not Found\r\n\r\n");
               if (send(new_fd, response, strlen(response), 0) == -1) {
                   perror("send");
               }
               perror("fopen");
               exit(1);
            }

            // file is found so we send 200 OK message
            strcpy(response, "HTTP/1.1 200 OK\r\n\r\n");
            if (send(new_fd, response, strlen(response), 0) == -1) {
               perror("send");
               exit(1);
            } 

            // send contents of file
            memset(response, '\0', BUFFER);         // fill response buffer with NULL characters

           size_t bytes_read;
           while ((bytes_read = fread(response, 1, BUFFER, file)) > 0) {
               if (send(new_fd, response, bytes_read, 0) == -1) {
                   perror("send");
                   break;
               }
               memset(response, '\0', BUFFER);
            }

            fclose(file);
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}