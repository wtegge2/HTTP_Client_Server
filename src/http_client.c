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
#include <string.h>


#define PORT "80" 

#define BUFFER 4096

#define MAXDATASIZE 5000

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) {
     return &(((struct sockaddr_in*)sa)->sin_addr);
 }

 return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




int main(int argc, char *argv[])
{
 int sockfd, numbytes;  
 char buf[MAXDATASIZE];
 struct addrinfo hints, *servinfo, *p;
 int rv;
 char s[INET6_ADDRSTRLEN];


 char *full_url = argv[1];
 // char *port_number= "80";
 // char *full_path;
 char full_request[MAXDATASIZE];
 char *start_of_path = NULL;
 char *start_of_port = NULL;
 char *hostname_start = NULL;

  int no_port_flag = 0;


  size_t hostname_length;
  size_t port_length;
  size_t path_length;
  char *hostname;
  char *path;
  char *port;


 // checking for valid URL
 char *start = full_url;
 while(strncmp(start, "http://", 7) != 0){
     if(*start == '\0'){
         fprintf(stderr, "Invalid \n");
         exit(1);
     }
     start++;
 }

 start = start + 7;
 
 // get the start of port number
 start_of_port = start;
 while(*start_of_port != ':'){
     if(*start_of_port == '\0'){
          no_port_flag = 1;
          start_of_port = start;
          break;
     }
     start_of_port++;
 }


 // get the start of file path
 start_of_path = start_of_port;
 while(*start_of_path != '/'){
     if(*start_of_path == '\0'){
          fprintf(stderr, "Path not found\n");
          exit(1);
     }
     start_of_path++;
 }

 hostname_start = start;

   printf("no_port_flag: %d \n", no_port_flag);


  if(no_port_flag == 1){

        printf("HERE NO PORT! \n");

      hostname_length = start_of_path - hostname_start;


      port_length = 0;


      path_length = strlen(start_of_path);


      hostname = malloc(hostname_length + 1);
 
      path = malloc(path_length + 1);


      strncpy(hostname, hostname_start, hostname_length);
      hostname[hostname_length] = '\0';


      port = PORT;


      strncpy(path, start_of_path + 1, path_length - 1);
      path[path_length - 1] = '\0';

  }
  else{

      hostname_length = start_of_port - hostname_start;
      
      port_length = start_of_path - start_of_port;

      path_length = strlen(start_of_path);

      hostname = malloc(hostname_length + 1);

      port = malloc(port_length + 1);
 
      path = malloc(path_length + 1);

      strncpy(hostname, hostname_start, hostname_length);
      hostname[hostname_length] = '\0';

      strncpy(port, start_of_port + 1, port_length - 1); 
      port[port_length - 1] = '\0';

      strncpy(path, start_of_path + 1, path_length - 1);
      path[path_length - 1] = '\0';


  }


 // Print extracted hostname, port, and path
 printf("Hostname: %s\n", hostname);
 printf("Port: %s\n", port);
 printf("Path: %s\n", path);


 if (argc != 2) {
     fprintf(stderr,"usage: client hostname\n");
     exit(1);
 }

 memset(&hints, 0, sizeof hints);
 hints.ai_family = AF_UNSPEC;
 hints.ai_socktype = SOCK_STREAM;




 if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
     printf("error with port or hostname!");
     return 1;
 }


 // loop through all the results and connect to the first we can
 for(p = servinfo; p != NULL; p = p->ai_next) {
     if ((sockfd = socket(p->ai_family, p->ai_socktype,
             p->ai_protocol)) == -1) {
         perror("client: socket");
         continue;
     }

     if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("client: connect");
         continue;
     }
     break;
 }


 if (p == NULL) {
     fprintf(stderr, "client: failed to connect\n");
     return 2;
 }


 inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
         s, sizeof s);
 printf("client: connecting to %s\n", s);


 freeaddrinfo(servinfo); // all done with this structure


 // create the GET request
 if(path[0] == '/'){
     // for absolute paths
      if(no_port_flag == 1){
          sprintf(full_request, "GET /%s HTTP/1.1\r\n\r\n", path);
          printf("Path: %s \n", path);
      }
      else{
          sprintf(full_request, "GET /%s HTTP/1.1\r\nHost: %s:%s\r\nConnection: closed\r\n\r\n", path, hostname, port);
            printf("Path: %s \n", path);
      }


 }
 else{

      if(no_port_flag == 1){
          sprintf(full_request, "GET %s HTTP/1.1\r\n\r\n", path);
          printf("Path: %s \n", path);
      }
      else{
          sprintf(full_request, "GET %s HTTP/1.1\r\nHost: %s:%s\r\nConnection: closed\r\n\r\n", path, hostname, port);
            printf("Path: %s \n", path);
      }




 }

 // send request and check for error
 if(send(sockfd, full_request, strlen(full_request), 0) == -1){

     perror("send");

     close(sockfd);

     exit(1);


 }


 FILE *file = fopen("output", "w");
 if(!file){

     perror("Error with output file");

     close(sockfd);

     exit(1);

 }

    int flag = 0;
    char *phrase_1 = "HTTP/1.1 200 OK\r\n\r\n";
    char *phrase_2 = "HTTP/1.1 400 Bad Request\r\n\r\n";
    char *phrase_3 = "HTTP/1.1 404 File Not Found\r\n\r\n";
    char *position_1;
    char *position_2;
    char *position_3;

    while((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
        
        position_1 = strstr(buf, phrase_1);
        position_2 = strstr(buf, phrase_2);
        position_3 = strstr(buf, phrase_3);

        if(position_1 != NULL){

            flag = fwrite(buf, 1, position_1 - buf, file);

            printf("Skipped phrase 1\n");

            if(flag != (position_1 - buf)){
                printf("ERROR 1 \n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

            flag = fwrite(position_1 + strlen(phrase_1), 1, numbytes - (position_1 - buf) - strlen(phrase_1), file);

            if(flag != (numbytes - (position_1 - buf) - strlen(phrase_1))){
                printf("ERROR 2\n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

        }
        else if(position_2 != NULL){

            flag = fwrite(buf, 1, position_2 - buf, file);

            printf("Skipped phrase 2 \n");

            if(flag != (position_2 - buf)){
                printf("ERROR 1 \n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

            flag = fwrite(position_2 + strlen(phrase_2), 1, numbytes - (position_2 - buf) - strlen(phrase_2), file);

            if(flag != (numbytes - (position_2 - buf) - strlen(phrase_2))){
                printf("ERROR 2\n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

        }
        else if(position_3 != NULL){

            break;

            flag = fwrite(buf, 1, position_3 - buf, file);

            printf("Skipped phrase 3\n");

            if(flag != (position_3 - buf)){
                printf("ERROR 1 \n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

            flag = fwrite(position_3 + strlen(phrase_3), 1, numbytes - (position_3 - buf) - strlen(phrase_3), file);

            if(flag != (numbytes - (position_3 - buf) - strlen(phrase_3))){
                printf("ERROR 3\n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }

        }
        else{
            flag = fwrite(buf, 1, numbytes, file);

            if(flag != numbytes){
                printf("ERROR \n");
                printf("number of bytes written: %d \n", flag);
                printf("number of bytes recieved: %d \n", numbytes);
                printf("error with fwrite! \n");
                break;
            }
        }
        memset(buf, '\0', MAXDATASIZE);
    }




 if(numbytes < 0){

     perror("recv");

     close(sockfd);

     fclose(file);

     exit(1);

 }

 close(sockfd);

 fclose(file);

 free(hostname);

  if(no_port_flag != 1){
      free(port);
  }

 free(path);

 return 0;
}