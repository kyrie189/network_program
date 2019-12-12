#define _GNU_SOURCE

#include "OXchess_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define BUF_MAXSIZE 1024


struct userinfo {
  char id[100];
  int playwith;
};
void error_handling(char *message);
int fdt[MAX_CLIENT_NUM] = {0};    //client fd 
char mes[1024];
int SendToClient(int fd, char *buf, int Size);
struct userinfo users[100];
int find_fd(char *name);
int win_dis[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
                     {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

void exit_with_response(const char response[]) {
  perror(response);
  exit(EXIT_FAILURE);
}

void exit_on_error(const int value, const char response[])
 {
  if (value < 0)
   {
    exit_with_response(response);
  }
}

void message_all_user(char *chatting)
 {
  int i = 0;
  for (i = 0; i < 100; i += 1) 
  {
    if (users[i].id[0] != '\0')
    {
      printf("%s", chatting);
      send(i, chatting, strlen(chatting), 0);
    }
  }
}

void message_handler(char *mes, int sender) {
  int instruction = 0;
  sscanf(mes, "%d", &instruction);
  switch (instruction)
  {
  case 9: 
  {
    message_all_user(mes);
    break;
  }
  case 1: 
  {     // add user
    char name[100];
    sscanf(mes, "1 %s", name);
    strncpy(users[sender].id, name, 100);
    send(sender, "1", 1, 0);
    printf("1:%s\n", name);
    break;
  }
  case 2: 
  { // show
    char buf[BUF_MAXSIZE], tmp[100];
    int p = sprintf(buf, "2 ");
    for (int i = 0; i < 100; i ++)
    {
      if (strcmp(users[i].id, "") != 0) 
      {
        sscanf(users[i].id, "%s", tmp);
        p = sprintf(buf + p, "%s ", tmp) + p;
      }
    }
    printf("2:%s\n", buf);
    send(sender, buf, strlen(buf), 0);
    break;
  }
  case 3:     // invite users to plat ox 
  { 
    char user_a[100], user_b[100];    
    char buf[BUF_MAXSIZE];
    sscanf(mes, "3 %s %s", user_a, user_b);
    int b_fd = find_fd(user_b);
    sprintf(buf, "4 %s invite you. Accept?\n", user_a);
    send(b_fd, buf, strlen(buf), 0);
    printf("3:%s", buf);
    break;
  }
  case 5:     // agree(1) or not(0)
  { 
    
    int state;
    char inviter[100];
    sscanf(mes, "5 %d %s", &state, inviter);
    if (state == 1) {
      send(sender, "6\n", 2, 0);
      send(find_fd(inviter), "6\n", 2, 0);
      int fd = find_fd(inviter);
      users[sender].playwith = fd;
      users[fd].playwith = sender;
      printf("6:\n");
    }
    break;
  }
  case 7: 
  {
    int board[9];
    char state[100];
    char buf[BUF_MAXSIZE];
    sscanf(mes, "7  %d %d %d %d %d %d %d %d %d", &board[0], &board[1],
           &board[2], &board[3], &board[4], &board[5], &board[6], &board[7],
           &board[8]);
    for (int i = 0; i < 100; i += 1) 
    {
      state[i] = '\0';
    }

    memset(buf, '\0', BUF_MAXSIZE);
    memset(state, '\0', sizeof(state));
    strcat(state, users[sender].id);
    for (int i = 0; i < 8; i += 1)
     {
      if (board[win_dis[i][0]] == board[win_dis[i][1]] &&board[win_dis[i][1]] == board[win_dis[i][2]]) 
      {
        if (board[win_dis[i][0]] != 0)
        {
          strcat(state, "_Win!\n");
          sprintf(buf, "8  %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1],
                      board[2], board[3], board[4], board[5], board[6], board[7],board[8], state);
          printf("7:%s", buf);
          send(sender, buf, sizeof(buf), 0);
          send(users[sender].playwith, buf, sizeof(buf), 0);
          return;
        }
      }
    }
    memset(buf, '\0', BUF_MAXSIZE);
    memset(state, '\0', sizeof(state));
    for (int i = 0; i < 9; i += 1) 
    {
      if (i == 8) {
        strcat(state, "Tie!\n");
        sprintf(buf, "8  %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1],
                board[2], board[3], board[4], board[5], board[6], board[7],board[8], state);
        printf("7:%s", buf);
        send(sender, buf, sizeof(buf), 0);
        send(users[sender].playwith, buf, sizeof(buf), 0);
        return;
      }
      if (board[i] == 0) {
        break;
      }
    }
    memset(buf, '\0', BUF_MAXSIZE);
    memset(state, '\0', sizeof(state));
    strcat(state, users[users[sender].playwith].id);
    strcat(state, "_your_tern!\n");
    sprintf(buf, "8  %d %d %d %d %d %d %d %d %d %s\n", board[0], board[1],
            board[2], board[3], board[4], board[5], board[6], board[7],
            board[8], state);
    printf("7:%s", buf);
    send(sender, buf, sizeof(buf), 0);
    send(users[sender].playwith, buf, sizeof(buf), 0);
    break;
  }
  }
}
void *pthread_service(void *socket)
 {
  int sock = *(int *)socket;
  while (true) 
  {
    int numbytes;
    int i;
    numbytes = recv(sock, mes, BUF_MAXSIZE, 0);
    printf("\n\n%s\n\n\n", mes);

    // close socket
    if (numbytes <= 0) 
    {
      for (i = 0; i < MAX_CLIENT_NUM; i += 1) 
      {
        if (sock == fdt[i])
        {
          fdt[i] = 0;
        }
      }
      memset(users[sock].id, '\0', sizeof(users[sock].id));
      users[sock].playwith = -1;
      break;
    }

    message_handler(mes, sock);
    bzero(mes, BUF_MAXSIZE);
  }
  close(sock);
}

int main(int argc,char *argv[]) 
{
  int serv_sock;
  struct sockaddr_in serv_addr;
  struct sockaddr_in client;
  int sin_size;
  sin_size = sizeof(struct sockaddr_in);
  int socket_count = 0;

  for (int i = 0; i < 100; i ++) //initial all user
  {
    for (int j = 0; j < 100; j++)
    {
      users[i].id[j] = '\0';
    }
    users[i].playwith = -1;
  }

  serv_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (serv_sock == -1)
  {
    error_handling("error_serv_sock");
  }
  memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;        //   inet_addr("192.168.112.133"); 
	serv_addr.sin_port = htons(atoi(argv[1]));
  int r = bind (serv_sock , (struct sockaddr*)&serv_addr,sizeof (serv_addr));
  if (r == -1)
  {
      error_handling("bind_error");
  }
   r = listen(serv_sock , 5);
  if (r == -1)
  {
      error_handling ("listen_eror");
  }
  printf("please wating for response of client !!!\n!");
  int new_socket;
  while (true) 
  {
    new_socket = accept(serv_sock, (struct sockaddr *)&client, &sin_size);
    if (new_socket == -1)
    {
      error_handling("error_accept");
    }
    if (socket_count >= MAX_CLIENT_NUM)  // can't exceed max client number;
    {
      printf("no more client is allowed\n");
      close(new_socket);
    }
    for (int i = 0; i < MAX_CLIENT_NUM; i += 1)   
    {
      if (fdt[i] == 0) 
      {
        fdt[i] = new_socket;
        break;
      }
    }
    pthread_t thread;
    pthread_create(&thread, NULL, (void *)pthread_service, &new_socket);
    socket_count += 1;
  }
  close(serv_sock);
}

int find_fd(char *name) 
{
  for (int i = 0; i < 100; i += 1) 
  {
    if (strcmp(name, users[i].id) == 0)
    {
      return i;
    }
  }
  return -1;
}
void error_handling(char *message)
{
	fputs(message,stderr);
	fputs("\n",stderr);
	exit(1);
}
