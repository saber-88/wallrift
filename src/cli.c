/*
 author : github.com/saber-88
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

void printHelp(){
  printf("\033[1mwallrift - A smooth parllax supported wallpaper engine\033[0m.\n\n");
  printf("\033[1m\033[4mUsage\033[0m: \033[1mwallrift [command] [value]\033[0m\n\n");
  printf("\033[1m\033[04mCommands\033[0m: \n\n");
  printf("  \033[1mimg\033[0m      sends the path of the wallpaper to the daemon.\n");
  printf("  \033[1mspeed\033[0m    sets the speed for parllax. Lower speed = smooth interpolation.\n");
}

int main(int argc, char *argv[])
{
  float speed = 0.05; //default speed
  const char* wallpath = NULL;
  
  for (int i = 1; i < argc; i++ && (i + 1) <= argc) {
    if (strcmp("img", argv[i]) == 0) {
      wallpath = argv[++i];
    }
    else if (strcmp("speed", argv[i]) == 0 && (i + 1) <= argc) {
      speed = strtof(argv[++i], NULL);
    }
  }
  
  if (!wallpath) {
    printHelp();
  }

  printf("Wallpath : %s\n",wallpath);
  printf("speed : %.2f\n",speed);

}

