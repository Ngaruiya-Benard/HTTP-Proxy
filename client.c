

/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#define BUFSIZE 50


struct __attribute__((__packed__)) header {
  unsigned short int type; //2 bytes
  char source[20]; //20 bytes
  char destination[20]; //20 bytes
  unsigned int length; //4 bytes
  unsigned int message_id; //4 bytes
};
typedef struct header* header;

header to_htons(header buffer);

header to_ntohs(header buffer);

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    
    header test = malloc(50);
    test->type = htons(1);
    memcpy(test->source, "client1\0", sizeof("client1\0"));
    memcpy(test->destination, "Server\0", sizeof("Server\0"));
    test->length = htonl(0);
    test->message_id = htonl(0);

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);


    memcpy(buf, test, 50);
    assert(buf); //is this supposed to print out anything?

    /* connect: create a connection with the server */
    printf("Tried to connect\n");
    if (connect(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");
    printf("connected\n");

    /* send the message line to the server */
    printf("Tried to writing\n");
    n = write(sockfd, buf, 50);
    printf("wrote\n");
    if (n < 0) 
      error("ERROR writing to socket");

    ////////////////////print hello ack header
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);

    if (n < 0) 
      error("ERROR reading from socket");

    header test_ack = malloc(50);
    memcpy(test_ack, buf, 50);

    fprintf(stderr, "%d\n", ntohs(test_ack->type));
    fprintf(stderr, "%s\n", test_ack->source);


    


    //////////////////////Send a chat header


    header test2 = malloc(50);
    test2->type = htons(5);
    memcpy(test2->source, "client1\0", sizeof("client1\0"));
    memcpy(test2->destination, "client1\0", sizeof("client1\0"));
    test2->length = htonl(sizeof("This is a test.\n"));
    test2->message_id = htonl(1);

    n = write(sockfd, (char*) test2, 50);
    if (n < 0) 
      error("ERROR writing to socket");

    bzero(buf, BUFSIZE);
    memcpy(buf, "This is a test.\n", sizeof("This is a test.\n"));
    n = write(sockfd, buf, sizeof("This is a test.\n"));
    if (n < 0) 
      error("ERROR writing to socket");


    /*reading the chat message returned to me*/
    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if(n < 0)
        error("ERROR on receiving chat message header\n");
    header chat_message_header = (header) buf;
    chat_message_header = to_ntohs(chat_message_header);

    bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if(n < 0)
        error("ERROR on receiving actual message");

    printf("Here is the received chat message:  %s\n", buf);
    

    ///////////////////////print client list header
    bzero(buf, BUFSIZE);
    printf("reading client lsit header\n");
    n = read(sockfd, buf, BUFSIZE);
    printf("done reading client list header\n");
    if (n < 0) 
      error("ERROR reading from socket");

    header test_client_list = malloc(50);
    memcpy(test_client_list, buf, 50);

    fprintf(stderr, "%d\n", ntohs(test_client_list->type));
    fprintf(stderr, "%s\n", test_client_list->source);

    //print client list
    bzero(buf, BUFSIZE);
    printf("reading client lsit\n");
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    fprintf(stderr, "Echo from server: %s\n", buf);
    
    // sleep(1);
    // close(sockfd);
    return 0;
}


header to_htons(header buffer) 
{
    buffer->type = htons(buffer->type);
    buffer->length = htonl(buffer->length);
    buffer->message_id = htonl(buffer->message_id);
}

header to_ntohs(header buffer) 
{
    buffer->type = ntohs(buffer->type);
    buffer->length = ntohl(buffer->length);
    buffer->message_id = ntohl(buffer->message_id);


printf("message header: \n \t message_type: %u\n\t message_source: %s\n\t message destination: %s\n\t message length: %d\n\t message ID: %d\n", buffer->type, buffer->source, buffer->destination, buffer->length, buffer->message_id);


    return buffer;
}