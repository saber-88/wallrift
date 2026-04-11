/*
 author : github.com/saber-88
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/wallrift.sock"

void printHelp(){
  printf("\033[1mwallrift - A smooth parllax supported wallpaper engine\033[0m.\n\n");
  printf("\033[1m\033[4mUsage\033[0m: \033[1mwallrift [command] [value]\033[0m\n\n");
  printf("\033[1m\033[04mCommands\033[0m: \n\n");
  printf("  \033[1mimg\033[0m      sends the path of the wallpaper to the daemon.\n");
  printf("  \033[1mspeed\033[0m    sets the speed for parllax. Lower speed = smooth interpolation.\n");
}
const char* get_sock_path();

int main(int argc, char *argv[])
{
  float speed = 0.05; //default speed
  const char* wallpath = NULL;
  if (argc < 3) {
    printHelp();
    return 1;
  }
  
  for (int i = 1; i < argc; i++) {
    if (strcmp("img", argv[i]) == 0 && (i + 1) <= argc) {
      wallpath = argv[++i];
    }
    else if (strcmp("speed", argv[i]) == 0 && (i + 1) <= argc) {
      speed = strtof(argv[++i], NULL);
    }
  }
  
  if (!wallpath) {
    printHelp();
  }

  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd == -1) {
    perror("Failed to open socket\n");
    return 1;
  }

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  
  strncpy(addr.sun_path, get_sock_path(), sizeof(addr.sun_path) - 1); 

  if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("Connection to socket failed!\n");
    return 1;
  }

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "img %s speed %f",wallpath,speed);

  if (write(sock_fd, cmd, strlen(cmd)) == -1) {
    perror("write to socket failed!\n");
    return 1;
  }

  close(sock_fd);
  return 0;
}

