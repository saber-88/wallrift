#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
const char* get_sock_path()
{
  static char socket_path[256]={0};//If path is created once then use it everytime 
                                   //Though it is of not much use as each process is only able to execute it once :(
  if(socket_path[0]!='\0')
  {
   return socket_path;
  }
  char dir_path[256]={0};
  snprintf(dir_path, sizeof(dir_path),"/tmp/wallrift/%s",getenv("USER"));
  printf("%s\n",socket_path);
  mkdir("/tmp/wallrift",0700);
  mkdir(dir_path,0700);
  snprintf(socket_path, sizeof(socket_path), "%s/wallrift.sock",dir_path);
  printf("The Socket Path is %s\n",socket_path);
  return socket_path;
}


