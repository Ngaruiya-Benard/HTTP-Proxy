/**************************************************************************
 *
 *Benard Ngaruiya bngaru01
 *
 *HTTP proxy server
 *
 *
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>


typedef struct{
	char* data;
	char* name;
	int timeStored,living_time;
	int accessTime;
	int dataSize;
} cache_node;

typedef struct {
	int full;
	int size;
	cache_node *storeCache[10];
}cache;


/*
 *Function handles communication between the proxy and the server
 *Function has no return type
 *
 */
void connect_server( char* hostname, char* received_buffer, int portno, int newsockfd, cache* myCache);


/*
 *Function prints error message as passed into it
 */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}


/*function parses the buffer to extract the hostname*/
char* hostname_parser(char* buffer, char** server_portno);


/*
 *Checks for fresh data in the cache
 */
int freshData(cache *storeCache, char *name, int newsockfd);


/*
 *Returns the living time of an entry in a cache
 */
int get_LivingTime(char *response);

/*
 *Function returns the name of an object in the cache associated with a portno
 *
 */
char  *getName(char *request, int portno);


/*
 *
 *Function handles requests to write to the client
 *
 */
void client_write(cache_node *data, int age, int newsockfd);

/*
 *returns the age of an entry if its not stale or 0 if it is stale
 *
 */
int stale(cache_node *record);


/*
 *function updates new entries 
 *
 */
void updateCache(cache *currCache, cache_node *newNode);

/*
 *function finds out if an entity has been recently cached or not
 *
 */
bool cacheData(cache *currCache, cache_node *newNode);


/*
 *Function returns a stale entry from the cache
 *
 */
int getStale(cache *currCache);


/*
 *Function return the index of the least accessed entry in the cache
 *
 */
int getLeastAccessed(cache *currCache);


int main(int argc, char *argv[])
{
        char* server_portno = malloc(100);
	cache* myCache = malloc(sizeof(cache));
        int sockfd, newsockfd, portno;
        socklen_t clilen;
        char* buffer = malloc(300);
        struct sockaddr_in serv_addr, cli_addr;
        if (argc < 2) {
                fprintf(stderr,"ERROR, no port provided\n");
                exit(1);
        }
        int reuse = 1;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
                error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        portno = atoi(argv[1]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const int*) &reuse, sizeof(reuse));
        if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
        while(1) {

                  listen(sockfd,5);
        	clilen = sizeof(cli_addr);
        	newsockfd = accept(sockfd, 
                         	(struct sockaddr *) &cli_addr, 
                         	&clilen);
        	if (newsockfd < 0) 
               		error("ERROR on accept");
        	bzero(buffer,256);
        	int n;
        	n = read(newsockfd,buffer,255);
        	if (n < 0) 
                	error("ERROR reading from socket");
                char* hostname = hostname_parser(buffer, &server_portno);
                int serv_portno = atoi((const char*) server_portno);
                
                char* name = getName(buffer, serv_portno);
                int index = freshData(myCache, name, newsockfd);
                if(index < 0) {
                	connect_server(hostname, buffer, serv_portno, newsockfd, myCache);
                }
                
                
                close(newsockfd);
        }
        
     return 0; 
}




char* hostname_parser(char* buffer, char** server_portno)
{
        char* finder = strstr(buffer, "Host:");
        if(finder == NULL)
                error("cannot find hostname\n");

        char* curr = finder;
        int i = 0;
        char* hostname = malloc(100);

        while(curr[i + 6] != '\r' && curr[i+6] != ':') {
                hostname[i] = curr[i + 6];
                i++;
        }
        hostname[i] = '\0';
        if(curr[i + 6] == ':')
        {
                i++;
                int j = 0;
                while(curr[i + 6] != '\n') {
             		(*server_portno)[j] = curr[i+6];
                        i++;
                        j++;
                }
                (*server_portno)[j] = '\0';
        }

        else
		*server_portno = "80";
        return hostname;
}


void connect_server( char* hostname, char* received_buffer, int portno, int newsockfd, cache* myCache)
{
        int sockfd, n;
    struct hostent *server;
    struct sockaddr_in servaddr;
    char server_response[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&servaddr.sin_addr.s_addr,
         server->h_length);
    servaddr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr)) < 0) 
        error("ERROR connecting");
    
    printf("user request is: %s\n", received_buffer);
    n = write(sockfd,received_buffer,strlen(received_buffer));
    
    if (n < 0) 
         error("ERROR writing to socket");
    
   
    bzero(server_response, 1024);
    int size = 0;
    int i  = 0;
    int j = 0;
    char* curr = malloc(sizeof(char) * 0);
    
	while(n = read(sockfd, server_response, 1024)) {
    		size = size + n;
    		curr = realloc(curr, sizeof(char) * size);
    		while(i < size) {
    			curr[i] = server_response[j];
    			i++;
    			j++;
    		}	
    		j = 0;
    		bzero(server_response, 1024);
    	}
    	
    	cache_node* newCache_node = malloc(sizeof(cache_node));
    	newCache_node->data = curr;
	newCache_node->name = getName(received_buffer, portno);
	newCache_node->timeStored = time(NULL);
	newCache_node->living_time = get_LivingTime(curr);
	newCache_node->accessTime = time(NULL);
	newCache_node->dataSize = size;
    	
    	client_write(newCache_node, 0, newsockfd);
    	close(sockfd);
    	
    	if(newCache_node->living_time != 0) {
    		updateCache(myCache, newCache_node);
    	}
    	
}

/*function returns the time to Live*/
int get_LivingTime(char *response){
    char *holder;
    int livingTime;
    holder = strstr(response, "max-age=");
    if (holder != NULL){
        if((holder + 8)[0] == '0'){
            livingTime = 0;
            return 0;
        }
        livingTime = atoi(holder + 8);
    }
    if((livingTime == 0) || (holder == NULL)){
        livingTime = 3600;
    }
    return livingTime;

}

char  *getName(char *request, int portno){
    char *objectName = malloc(100);
    char *holder;
    char presentString[6];
    int i = 0, j = 0;
    holder = strstr(request, "GET ") + 4;
    while(holder[i] != ' '){
        objectName[i] = holder[i];
        i++;
    }
    objectName[i] = ':';
    i++;
    sprintf(presentString,"%d",portno);
    while(presentString[j] != '\0'){
        objectName[i] = presentString[j];
        i++;
        j++;
    }
    objectName[i] = '\0';
    return objectName;
}

int freshData(cache *currCache, char *name, int newsockfd){
    for(int i = 0; i < currCache->size; i++){
        if (strcmp(name, currCache->storeCache[i]->name) == 0){
            int age = stale(currCache->storeCache[i]);
            if(age){
                printf("Got request: %s from the cache with its age: %d\n\n", currCache->storeCache[i]->name, age);
                client_write(currCache->storeCache[i], age, newsockfd);
                currCache->storeCache[i]->accessTime = time(NULL);
                return i;
            }
        }
    }
    return -1;
}


void client_write(cache_node *data, int age, int newsockfd){
    char *currRep;
    int sizeData;
    if(age == 0){
        sizeData = data->dataSize;
        currRep = data->data;
    }else{
        char ageStr[100];
        sprintf(ageStr,"%d",age);
        char *toAdd = "Age: ";
        sizeData = data->dataSize + strlen(ageStr) + strlen(toAdd) + 2;
        currRep = malloc(sizeData);
        int i = 0, j = 0, c = 0, d = 0;
        bool firstline = true;
        while(i < (sizeData)){
            if(data->data[j] != '\n' && firstline == true){
                if(data->data[j + 1] == '\n'){
                    firstline = false;
                    currRep[i] = data->data[j];
                    i++; j++;
                    break;
                }
                currRep[i] = data->data[j];
                i++; j++;
            }
        }
        currRep[i] = '\n';
        i++; j++;
        while(toAdd[c] != '\0'){
            currRep[i] = toAdd[c];
            i++; c++;
        }
        while(ageStr[d] != '\0'){
            currRep[i] = ageStr[d];
            i++; d++;
        }
        currRep[i] = '\r';
        i++;
        currRep[i] = '\n';
        i++;
        while(i < sizeData){
            currRep[i] = data->data[j];
            i++; j++;
        }
    }
    //printf("%s\n", currRep);
    int n = write(newsockfd, currRep, sizeData);
    if (n < 0) 
        error("ERROR writing to socket");
}


int stale(cache_node *record){
    time_t currentTime = time(NULL);
    int age = currentTime - record->timeStored;
    if(age >= record->living_time){
        printf("%s is stale\n", record->name);
        age = 0;
    }
    return age;
}

void updateCache(cache *currCache, cache_node *newNode){
    if((currCache->full == false) && !(cacheData(currCache, newNode))){
        printf(" %s Has been successfully added to the cache\n\n", newNode->name);
        currCache->storeCache[currCache->size] = newNode;
        currCache->size += 1;
        if(currCache->size == 10)
            currCache->full = true;
    }else if(cacheData(currCache, newNode)){
        for(int i = 0; i < currCache->size; i++){
            if(strcmp(newNode->name, currCache->storeCache[i]->name) == 0){
                printf("%s has been successfully updated in the cache\n\n", currCache->storeCache[i]->name);
                currCache->storeCache[i] = newNode;
            }
        }
    }else{
        int staleDataIndex = getStale(currCache);
        if(staleDataIndex != -1){
            printf("%s is stale and has been ejected\n\n", currCache->storeCache[staleDataIndex]->name);
            currCache->storeCache[staleDataIndex] = newNode;
        }else{
            int leastAccessedIndex = getLeastAccessed(currCache);
            printf("%s is the least accessed entity and has been ejected\n\n", currCache->storeCache[leastAccessedIndex]->name);
            currCache->storeCache[leastAccessedIndex] = newNode;
        }
    }
}





bool cacheData(cache *currCache, cache_node *newNode){
    for(int i = 0; i < currCache->size; i++){
        if(strcmp(newNode->name, currCache->storeCache[i]->name) == 0){
            return true;
        }
    }
    return false;
}

int getStale(cache *currCache){
    for(int i = 0; i < currCache->size; i++){
        if(stale(currCache->storeCache[i]) == 0)
            return i;
    }
    return -1;
}

int getLeastAccessed(cache *currCache){
    int smallest = currCache->storeCache[0]->accessTime;
    int smallestIndex = 0;
    for(int i = 0; i < currCache->size; i++){
        if(currCache->storeCache[i]->accessTime < smallest){
            smallest = currCache->storeCache[i]->accessTime;
            smallestIndex = i;
        }
    }
    return smallestIndex;
}





