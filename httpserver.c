/* Chun Zhou
 czhou43@ucsc.edu
 httpserver.c
 c file to duplicate the HTTP server with only HEAD, PUT, and GET request.
*/


#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 1000000

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */
uint16_t strtouint16(char number[]) {
  char *last;
  long num = strtol(number, &last, 10);
  if (num <= 0 || num > UINT16_MAX || *last != '\0') {
    return 0;
  }
  return num;
}

/**
   Creates a socket for listening for connections.
   Closes the program and prints an error message on error.
 */
int create_listen_socket(uint16_t port) {
  struct sockaddr_in addr;
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    err(EXIT_FAILURE, "socket error");
  }

  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(listenfd, (struct sockaddr*)&addr, sizeof addr) < 0) {
    err(EXIT_FAILURE, "bind error");
  }

  if (listen(listenfd, 500) < 0) {
    err(EXIT_FAILURE, "listen error");
  }

  return listenfd;
}


void put(char* file, int* response_code, char* body, int length) {
    // The function use to performance the put request from client and
    // return the correspond response code that result from the put request
    // to the handle connection.
    //
    // arguments: file, response_code, body, length
    // return response_code
    
    int file_open;
    printf("in put method\n");
    if(access(file, F_OK) != 0){
        *response_code = 201;
        if((file_open = open(file, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO))<=0){
            *response_code = 403;
        }else{
            write(file_open, body, length);
        }
        close(file_open);
    }else{
        if((file_open = open(file, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO))<=0){
            *response_code = 403;
        }else{
            write(file_open, body, length);
        }
        close(file_open);
    }
    printf("end of put method\n");
    
}

void geth(int connfd, char* file, int* response_code, int* size, int* res){
    // The function do the GET and HEAD request if the request format is correct
    // return the correspond body for the GET request.
    //
    // arguments: file, response_code, size of file

    int file_open;
    char buf[BUFFER_SIZE];
    static char* response;
    struct stat stfile;
    printf("in geth\n");
    if(access(file, F_OK) != 0){

        *response_code = 404;
    }else if((file_open = open(file, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO))<=0){
        *response_code = 403;
    }else{
        stat(file, &stfile);
        *size = stfile.st_size;
        read(file_open, buf, *size+1);
        if(*response_code == 200){
            response = "HTTP/1.1 200 OK\r\nContent-length: ";
            dprintf(connfd, "%s%d\r\n\r\n", response, *size);
            write(connfd, buf, *size);
            *res = 1;
            
        }

    }
    printf("end of geth\n");
    
}

void respond(int connfd, int response_code, char* method, int body_length){
    // The function that helps to send the response message with
    // correspond response_code and request method.
    //
    // arguments: connfd, response_code, method
    // return: response
    
    int res = 0;
    static char* response;

    if(strcmp(method, "PUT")==0){ // the response message with the body in PUT request.
        if(response_code == 200){
            response = "HTTP/1.1 200 OK\r\nContent-length: 3\r\n\r\nOK\n";
        }else if(response_code == 201){
            response = "HTTP/1.1 201 Created\r\nContent-length: 8\r\n\r\nCreated\n";
        }else if(response_code == 400){
            response = "HTTP/1.1 400 Bad Request\r\nContent-length: 12\r\n\r\nBad Request\n";
        }else if(response_code == 403){
            response = "HTTP/1.1 403 Forbidden\r\nContent-length: 10\r\n\r\nForbidden\n";
        }else if(response_code == 500){
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-length: 22\r\n\r\nInternal Server Error\n";
        }else if(response_code == 404){
            response = "HTTP/1.1 404 Not Found\r\nContent-length: 10\r\n\r\nNot Found\n";
        }           
    }else if(strcmp(method, "GET") ==0){ // the response message in GET request.
        if(response_code == 400){
            response = "HTTP/1.1 400 Bad Request\r\nContent-length: 12\r\n\r\nBad Request\n";
        }else if(response_code == 403){
            response = "HTTP/1.1 403 Forbidden\r\nContent-length: 10\r\n\r\nForbidden\n";
        }else if(response_code == 500){
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-length: 22\r\n\r\nInternal Server Error\n";
        }else if(response_code == 404){
            response = "HTTP/1.1 404 Not Found\r\nContent-length: 10\r\n\r\nNot Found\n";
        }       
    }else if(strcmp(method, "HEAD")==0){ // response message in HEAD request.
        if(response_code == 200){
            response = "HTTP/1.1 200 OK\r\nContent-length: ";
            dprintf(connfd, "%s%d\r\n\r\n", response, body_length);
            res++;
        }else if(response_code == 400){
            response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        }else if(response_code == 403){
            response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        }else if(response_code == 500){
            response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }else if(response_code == 404){
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
    }
    
    if(response_code == 501){
        response = "HTTP/1.1 501 Not Implemented\r\nContent-length: 16\r\n\r\nNot Implemented\n";
    }
    
    if (res == 0){
        dprintf(connfd, "%s", response);
    }
}

void writelog(char *log_file, char *method, char *file, char *hostname, int response_code, pthread_mutex_t *lock, char *http, int length, sem_t *count){
    /*
    The function that deal with the write to log_file
    argument: char *log_file, char *method, char *file, char *hostname, int response_code, pthread_mutex_t *lock, char *http, int length, sem_t *count
    The function will check if the log_file is accessable and write the correct log message to the log_file.
    */
    
    //intialize the variables for later use.
    int file_open;
    char message[1000];
    int counts;
    
    pthread_mutex_lock(lock); // lock the log_file when writing on it.
    sem_getvalue(count,&counts);
    
    //use count to see if the file is first time access or not. Truncate the log_file if first time use it.
    if(counts == 0){ 
        if((file_open = open(log_file, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO))<=0){
            warn("log_file error");
        }
        sem_post(count);
    }else{ 
        if((file_open = open(log_file, O_RDWR|O_CREAT|O_APPEND, S_IRWXU|S_IRWXG|S_IRWXO))<=0){
            warn("log_file error");
        }
    }
    
    //write to log_file if the request success.
    if(response_code == 200 || response_code == 201){
        sprintf(message, "%s\t/%s\t%s\t%d\n", method, file, hostname, length);
        write(file_open, message, strlen(message));
        close(file_open);
    }else if(strcmp(http, "HTTP/1.1")==0){ // With the Fail request but correct http.
        sprintf(message, "FAIL\t%s /%s HTTP/1.1\t%d\n", method, file, response_code);
        write(file_open, message, strlen(message));
        close(file_open);
    }else{ // with the error http.
        sprintf(message, "FAIL\t%s /%s HTTP/1.0\t%d\n", method, file, response_code);
        write(file_open, message, strlen(message));
        close(file_open);
    }
    pthread_mutex_unlock(lock);
}

void handle_connection(int connfd, pthread_mutex_t *lock, char* log_file, sem_t *count) {
  // do something
  // create the initial variables.
    
 
    char conn_buf[BUFFER_SIZE];
    char body[BUFFER_SIZE];
    
    
    int bytes_read;
    
    while((bytes_read = recv(connfd, conn_buf, BUFFER_SIZE, 0)) > 0){ //continue open the connection
        // receive the information from client.
        if (bytes_read < 0){
            warn("Read failed"); // check if the bytes receive is 0 or not.
        }
        
        printf("connect: %s", conn_buf);
        const char split[4] = "\r\n"; // to separate each line.
        char *token= strtok(conn_buf, split);
        char method[20], file1[100], http[20];
        char *file;
        int response_code = 200;
        int counter = 0;
        int length = -1;
        int size = 0;
        char host[100], Host[100];
        
        sscanf(token, "%s %s %s", method, file1, http); // parse the first line of request and
                                       //get the file and the request method.
        
        // check if the request have the appropriate method.
        if (strcmp(method, "PUT")==0){
            recv(connfd, body, BUFFER_SIZE, 0);
        }else if(strcmp(method, "GET") == 0){
            counter = counter;
        }else if(strcmp(method, "HEAD") ==0){
            counter = counter;
        }else{
            response_code = 501; // response code 501 is request method is unknown.
        }
        
        if(strcmp(http,"HTTP/1.1")!=0){
            response_code = 400;
        }
        // parse the strings and get the Content-Length and body from it.
        char first[20], later[20];
        token = strtok(NULL, split); // token already Null so split is NULL
     
        sscanf(token, "%s %s", Host, host);
        
        while(token != NULL){
            sscanf(token, "%s %s", first, later);
            if(strcmp(method, "PUT")==0 && strcmp(first, "Content-Length:")==0){
                counter = counter + 1;
                length = atoi(later);
            }
            token = strtok(NULL, split);
        }
        // parse the / from the object name.
        file = strtok(file1, "/");
        
        // create the variables that use to check the object name format.
        char file_check = file[0];
        int count_file = 0;
        int res = 0;
        
        // check the format of the object name,
        //if the length of object name is 15.
        if(strlen(file) != 15){          
            response_code = 400; // response_code to 400 if object name is invalid
        }
        
        // check the format of the object name.
        // check if the object name is all alpha and number.
        while(file_check != '\0'){
            if(isalnum(file_check) == 0){
                response_code = 400;
            }
            count_file++;
            file_check = file[count_file];
        }
        
        if(strcmp(method, "PUT") == 0 && counter == 0){ // check if the PUT request has
                                      // Content-Length or not
            response_code = 400;
        }
        printf("test1:%d\n", response_code);
        if(strcmp(method, "PUT") == 0 && response_code == 200 && length != -1){
        // do the PUT request work if the method is PUT and it is a valid request
            put(file, &response_code, body, length);
        }else if((strcmp(method, "GET")==0 || strcmp(method, "HEAD")==0 )&& response_code == 200){
            geth(connfd, file, &response_code, &size, &res);
        }
        if (size!=0){
            length = size;
        }
        // call the response function to preform the response message with the correspond response_code
        if (res == 0){
            respond(connfd, response_code, method, size);
        }
        //check if the log_file as name and call writelog function to write, otherwise do nothing.
        if(strcmp(log_file, " ") !=0 ){
            writelog(log_file, method, file, host, response_code, lock, http, length, count);
        }
  }
 
  // when done, close socket
  close(connfd);
}



typedef struct thread_record {
    //The class that contain all the share memory resources for threads to use
    
    int *shared_int;
    char *shared_buffer;
    int *shared_numbers;
    sem_t *count;
    pthread_mutex_t *lock;
    sem_t *sem;
    sem_t *sem2;
    int connfd;
    
} thread_record;

void * dispatcher(void *arg){
    // the function of dispatcher thread
    
    thread_record *record = (thread_record *) arg;
    int num;
    num = *record->shared_numbers;
    num = num + 1;
}

void * worker(void *arg){
    // The function of the worker thread, input is the pointer to type of 
    // shared information class.
    
    // create some initial vairables to use.
    thread_record *record = (thread_record *) arg;
    int num;
    while(1){ // use while loop to make sure that the worker is continue working non stop.
        sem_wait(record->sem); // sem wait to check if there is avaliable connection id to take.
        pthread_mutex_lock(record->lock); // lock the critical region to prevent other thread use at the same time.
        for (int i = 0; i < *record->shared_numbers; i ++){ // loop through list and get connect id.
            if(record->shared_int[i] != 0){
                num = record->shared_int[i];
                record->shared_int[i] = 0;            
                pthread_mutex_unlock(record->lock); // unlock the critical region.

                handle_connection(num, record->lock, record->shared_buffer, record->count);
                break;
            }
        }
        sem_post(record->sem2); // post sem2 for allowing the dispatcher know the avaliability of this worker thread.
    }
}

int main(int argc, char *argv[]) {
  // creation of initial variables
  int listenfd;
  uint16_t port;
  int threads_num = 4;
  char* log_file = " ";
  int opt;
  int numbers;
  int value = 0;
  
  // creation of lock and semaphore variables  
  pthread_mutex_t lock;
  pthread_mutex_init(&lock, NULL);
  pthread_t tid;
  
  sem_t sem;
  sem_init(&sem, 0, 0);
  sem_t sem2;
  sem_t count;
  sem_init(&count, 0, 0);
 
  thread_record record;
  int list[100] = { 0 };
  printf("list: %d\n",list[5]);
 
 
  if (argc == 2) {
    port = strtouint16(argv[1]);
  }else {
    for (int i = 1; i <argc; i++){ // parse out the hostnumber from input command.
        if (strcmp(argv[i],"-N")==0){
            i++;
        }else if(strcmp(argv[i],"-l")==0){
            i++;
        }else{
            port = strtouint16(argv[i]);
        }
    }
    while((opt = getopt(argc, argv, "N:l:"))!=-1){ // use getopt to parse the option
        switch(opt) {
            case 'N':
                threads_num = atoi(optarg);
                printf("threads_num: %d\n", threads_num);
                break;
            case 'l':
                log_file = optarg;
                printf("log_file: %s\n", log_file);
                break;
        }
    }
  }
  if (port == 0) {
        errx(EXIT_FAILURE, "invalid port number: %s", argv[1]);
  }
  
  // creation of all the variables to the thread_record
  record.shared_int = list;
  record.shared_buffer = log_file;
  record.lock = &lock;
  record.count = &count;
  numbers = threads_num;
  record.shared_numbers = &numbers;
  sem_init(&sem2, 0, threads_num);
  pthread_t tids[threads_num];
  
  record.sem = &sem;
  record.sem2 = &sem2;
  
  
  listenfd = create_listen_socket(port);
  for (int i = 0; i < threads_num; i++){// use for loop to create worker thread.
      pthread_create(&tids[i], NULL, &worker, &record);
  }
  pthread_create(&tid, NULL, &dispatcher, &record);
  while(1) {
    sem_wait(&sem2); // the semaphore2 decrement that check if there are avaliable worker thread. 
    int connfd = accept(listenfd, NULL, NULL);
    if (connfd < 0) {
      warn("accept error");
      continue;
    }
    
    pthread_mutex_lock(&lock); // lock before access shared contents.
    for (int i = 0; i < threads_num; i++){ // loop to store the connect id in share list.
        if (list[i] == 0){
            list[i] = connfd;
            value ++;
            break;
        }
    }
    sem_post(&sem); // increment semaphore1 to indicate the worker thread with avaliable connect id.
    sem_getvalue(&sem, &value);
    pthread_mutex_unlock(&lock); // unlock the share content for other thread to access.
    
  }
  return EXIT_SUCCESS;
}





