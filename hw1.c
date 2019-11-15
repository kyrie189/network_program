#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
unsigned long get_file_size(const char *path);
char *eva_con_len (char *com);
char*eva_boundary (char *bou);
char *eva_header (char *bou);
char *eva_upload(char *upload);
#define url "http://localhost:8080/fileUpload.action"
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#define MAX_REQUEST_SIZE 3047
int status;
const char *get_content_type(const char* path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }

    return "application/octet-stream";
}
SOCKET create_socket(const char* host, const char *port) 
{
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));       //load structure
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);     

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    return socket_listen;
}
struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
};

static struct client_info *clients = 0;

struct client_info *get_client(SOCKET s) {
    struct client_info *ci = clients;

    while(ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }

    if (ci) return ci;
    struct client_info *n =
        (struct client_info*) calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;
    clients = n;
    return n;
}


void drop_client(struct client_info *client) {
    CLOSESOCKET(client->socket);

    struct client_info **p = &clients;

    while(*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}


const char *get_client_address(struct client_info *ci) {
    static char address_buffer[100];
    getnameinfo((struct sockaddr*)&ci->address,ci->address_length,address_buffer, sizeof(address_buffer), 0, 0,NI_NUMERICHOST);
    return address_buffer;
}




fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;

    while(ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}


void send_400(struct client_info *client) {
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n\r\nBad Request";
    send(client->socket, c400, strlen(c400), 0);
    drop_client(client);
}

void send_404(struct client_info *client) {
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNot Found";
    send(client->socket, c404, strlen(c404), 0);
    drop_client(client);
}



void serve_resource(struct client_info *client, const char *path) {

    printf("serve_resource %s %s\n", get_client_address(client), path);

    if (strcmp(path, "/") == 0) path = "/index.html";

    if (strlen(path) > 100) {
        send_400(client);
        return;
    }

    if (strstr(path, "..")) {
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);

    printf ("full_path %s",full_path);
    FILE *fp = fopen(full_path, "rb");

    if (!fp) {
        send_404(client);
        return;
    }

    fseek(fp, 0L, SEEK_END);        //point to tail
    size_t cl = ftell(fp);          //calcuate 
    rewind(fp);                     //point to head

    const char *ct = get_content_type(full_path);

#define BSIZE 1024
    char buffer[BSIZE];
    printf("Server to clint \n");
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Length: %lu\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    int r = fread(buffer, 1, BSIZE, fp);
    while (r) 
    {
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}

/*void server_in (struct client_info *client, const char *path)
{


}*/
int main (int argc ,char *argv[])
{
    SOCKET server = create_socket(0, "8080");

    while (1)
    {

        fd_set reads;
        reads = wait_on_clients(server);

        if (FD_ISSET(server, &reads)) 
        {
            struct client_info *client = get_client(-1);
        //link
         client->socket = accept(server,(struct sockaddr*) &(client->address),&(client->address_length));
        //link fail 
        if (!ISVALIDSOCKET(client->socket)) 
        {
            fprintf(stderr, "accept() failed. (%d)\n",GETSOCKETERRNO());
             return 1;
        }
        printf("New connection from %s.\n",get_client_address(client));
        }

        struct client_info *client = clients;
        while(client) {
            struct client_info *next = client->next;
            if (FD_ISSET(client->socket, &reads)) 
            {
                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client);
                    continue;
                }
                int r = recv(client->socket,client->request + client->received,MAX_REQUEST_SIZE - client->received, 0);
                if (r < 1) 
                {
                    printf("Unexpected disconnect from %s.\n",
                            get_client_address(client));
                    drop_client(client);
                } 
                else 
                {
                    printf ("%s",client->request);
                    client->received += r;
                    client->request[client->received] = 0;
                    char *q = strstr(client->request, "\r\n\r\n");
                    if (q) {
                        if (strncmp("GET /", client->request, 5)!=0) 
                        {
                            if (strncmp("POST /", client->request, 6)==0)
                            {
                                
                                char *path1 = strdup(client->request);
                                char *path3 = strdup(client->request);
                                char *path4 = strdup(client->request);
                                char *path5 = strdup(client->request);
                                char *path =  path1;
                                char *path_boundary = path3;
                                char *path_header = path4;
                                char *Content_Len = eva_con_len(path);
                                //content_length
                                long long int  c_len = atoi(Content_Len);
                                printf ("i am content_length is %lld \n",c_len);
                               
                                //boundary
                                char *boundary;
                                boundary = eva_boundary(path_boundary);
                                //printf ("boundary = %s\n",boundary);

                                //header,upload
                                char *header = eva_header(path_header);
                                //printf ("%s\r\n\r\n",header);
                                char *upload;
                                upload = eva_upload(path5);
                                //printf ("%s",upload);

                                    /*char* request = (char*)malloc(c_len);	//申请内存用于存放要发送的数据
                                    if (request == NULL)
                                    {  
                                        printf("malloc request fail !\r\n");  
                                        return -1;  
                                    }  
                                    request[0] = '\0';
                                    /******* 拼接http字节流信息 *********/	
                                    /*strcat(request,header);  	
                                    strcat(request,"\r\n\r\n");								//http头信息
                                    strcat(request,boundary);
                                    strcat(request,"\r\n\r\n");	 
                                    char filepath = argv[1];
                                    FILE* fp = fopen(filepath, "rb+");	
                                    if (fp == NULL)
                                    {  
                                        printf("open file fail!\r\n");  
                                        return -1;  
                                    }
                                    int head_len = strlen(header) + 4;
                                    int request_len = strlen (upload)+4;
                                    int readbyte = fread(request+head_len+request_len, 1, 202400, fp);//读取上传的图片信息

                                    if(readbyte < 1024) //小于1024个字节 则认为图片有问题
                                    {
                                        printf("Read picture data fail!\r\n");  
                                        return -1;
                                    }
                                   // memcpy(request+head_len+request_len+202400,send_end,end_len);  //http结束信息
                                    /*********  发送http 请求 ***********/
                                    /*if(-1 == write(client->socket,request,c_len+20240)) 					
                                    {  
                                        printf("send http package fail!\r\n");  
                                        return -1;  
                                    }*/


                                *q = 0;
                            }
                            send_400(client);
                        } 
                        else 
                        {
                            char *path =client->request + 4;
                            char *end_path = strstr(path, " ");
                            if (!end_path) {
                                send_400(client);
                            } 
                            else 
                            {
                                *end_path = 0;
                                printf("wo zhao de shi shenme %s\n",path);
                                serve_resource(client, path);
                            }
                        }  
                    } //if (q)
                }
            }

            client = next;
        }
    }
}


unsigned long get_file_size(const char *path)  //获取文件大小
{    
    unsigned long filesize = -1;        
    struct stat statbuff;    
    if(stat(path, &statbuff) < 0){    
        return filesize;    
    }else{    
        filesize = statbuff.st_size;    
    }    
    return filesize;    
} 

char *eva_con_len (char *com)
{
    char *Content_Len = strstr(com, "Content-Length");
    char *path3 = Content_Len;
    while (1)
    {
        if (*path3 =='\n')
        {
            *path3 = '\0';
            break;
        }
        path3++;
    }
    Content_Len = strstr(Content_Len," ");
    return Content_Len;
}

char*eva_boundary (char *bou)
{
    char *bou1 = strstr(bou,"boundary");
    char *path3 = bou1;
    while (1)
    {
        if (*bou1 == '\n')
        {
            bou1--;
            *bou1 = '\0';
            break;
        }
        bou1++;
    }
    path3+=9;
    return path3;
}

char *eva_header (char *bou)
{
    char *bou1 = strstr(bou,"\r\n\r\n");
    *bou1 = 0;
    return bou;
}

char *eva_upload(char *upload)
{ 
    char *bou1 = strstr(upload,"\r\n\r\n");
    bou1+=4;
    char *bou2 = strstr(bou1,"\r\n\r\n");
    *bou2 =0; 

    return bou1;

}  

int server_in ()
{
    unsigned long totalsize = 0;
}


