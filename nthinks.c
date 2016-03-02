#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

struct arg_struct {
    int sock;
    char* localdir;
};
 
void *connection_handler(void *);
 
int main(int argc , char **argv) {

    pid_t process_id = 0;
    pid_t sid = 0;
    process_id = fork();
    if (process_id < 0) {
        perror("fork");
        exit(1);
    }

    if (process_id > 0) {
        printf("process_id of child process %d \n", process_id);
        exit(0);
    }

    umask(0);
    sid = setsid();
    if(sid < 0) {
        exit(1);
    }
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // </DAEMONIZING>

    int opt;
    char *cvalue = NULL;
    char *end;

    int port = 8888;
    char *host = NULL;
    char *localdir = NULL;



    while ((opt = getopt (argc, argv, "h:p:d:")) != -1) {
        switch (opt) {
            case 'p':
                port = strtol(optarg, &end, 10);
                break;
            case 'h':
                host = optarg;
                break;
            case 'd':
                localdir = optarg;
                break;
        }
    }

    printf("Using port: %d\n", port);
    printf("Using host: %s\n", host);
    printf("Using local directory: %s\n", localdir);

    DIR *d;
    struct dirent *dir;
    d = opendir(localdir);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG)
                printf("%s\n", dir->d_name);
        }

        closedir(d);
    }


    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons( port );
     
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    listen(socket_desc , 3);
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;
    
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
        puts("Connection accepted");

        struct arg_struct args;
        args.sock = client_sock;
        args.localdir = localdir;
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void *)&args) < 0) {
            perror("could not create thread");
            return 1;
        }
         
        //pthread_join( thread_id , NULL);
        pthread_detach(thread_id);
        puts("Handler assigned");
    }
     
    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 

void *connection_handler(void *arguments)
{
    struct arg_struct *args = arguments;

    int sock = args->sock;
    char *dir = args->localdir;
    int read_size;
    char *response = calloc(1, 4000);

    char client_message[4000];
    int fp;

    struct stat file_stat;
    off_t offset;
    int remain_data;
    int sent_bytes = 0;

     
    read_size = recv(sock, client_message, 4000, 0);

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1) {
        perror("recv failed");
    }

    client_message[read_size] = '\0';

    char *pch = strtok(client_message, " ");
    if (pch == 0)
        printf("strtok() failed\n");
    
    pch = strtok(NULL, " ");

    char *fullpath = malloc(
        strlen(realpath(dir, NULL)) + strlen(pch) + 1
    );
    sprintf(fullpath, "%s%s", realpath(dir, NULL), pch);

    printf("Fullpath: %s\n", fullpath);

    if( access( fullpath, F_OK ) != -1 ) {
        fp = open(fullpath, O_RDONLY);
        if (fstat(fp, &file_stat) < 0) {
            perror("fstat");
            exit(EXIT_FAILURE);
        }
        offset = 0;
        remain_data = file_stat.st_size;

        sprintf(
            response,
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n\r\n",
            remain_data
        );

        printf("%s\n", response);

        write(sock, response, strlen(response));

        while (((sent_bytes = sendfile(sock, fp, &offset, BUFSIZ)) > 0) && (remain_data > 0))
        {
                fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, (int)offset, remain_data);
                remain_data -= sent_bytes;
                fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, (int)offset, remain_data);
        }

        close(fp);
    } else {
        sprintf(
            response,
            "HTTP/1.0 404 Not Found\r\n\r\n"
        );

        write(sock, response, strlen(response));
    }

    memset(client_message, 0, 4000);
    free(fullpath);

    close(sock);
         
    return 0;
} 