/// @author : github.com/saber-88 @contributes: wallrift-daemon
/// @author : github.com/its-19818942118 @contributes: wallrift-cli

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "file.h"

#define SOCK_PATH "/tmp/wallrift.sock"

enum State {
    eTrue ,
    eError ,
    eFalse ,
};

struct Command{
  enum State img_s;
  enum State speed_s;
  enum State query_s;
  enum State transition_s;
  
  const char* path;
  const char* transition;
  float speed;
};

void printAvailableTransition(){
  printf("\033[1mAvailable transitons:\033[0m\n");
  printf("  \033[1mwipe\033[0m\n");
  printf("  \033[1mfade\033[0m\n\n");
}
void printHelp(){
  printf("\033[1mwallrift - A smooth parallax supported wallpaper engine\033[0m.\n\n");
  printf("\033[1m\033[4mUsage\033[0m: \033[1mwallrift <COMMAND>\033[0m\n\n");
  printf("\033[1m\033[04mCommands:\033[0m \n\n");
  printf("  \033[1mimg, -i\033[0m           sends the path of the wallpaper to the daemon.\n");
  printf("  \033[1mspeed, -s\033[0m         sets the speed( 0.00 - 1.00 ) for parallax.\n");
  printf("  \033[1mtransition, -t\033[0m    sets the transition type\n");
  printf("  \033[1mquery, -q\033[0m         prints the current applied wallpaper.\n");

  printf("\033[1m\033[04m\nOptions:\033[0m \n\n");
  printf("  \033[1m-h,--help\033[0m     prints this help message.\n");
}

int handleSocket(struct Command command) {
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1) {
      perror("Failed to open socket\n");
      return EXIT_FAILURE;
    }
  
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCK_PATH);
  
    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      perror("Connection to socket failed!\n");
      fprintf(stderr, "Is wallrift-daemon running ?\n");
      close(sock_fd);
      return EXIT_FAILURE;
    }
  
    char cmd[1024] = {0};
    
    if (command.img_s == eTrue && command.speed_s == eTrue && command.transition_s == eTrue) {
      int ret = snprintf(cmd, sizeof(cmd), "img %s speed %f transition %s", command.path, command.speed, command.transition);

      if (ret < 0 || ret >= (int)sizeof(cmd)) {
          fprintf(stderr, "command too long\n");
          close(sock_fd);
          return EXIT_FAILURE;
      }
    }
    else if (command.img_s == eTrue && command.speed_s == eTrue ) {
      int ret = snprintf(cmd, sizeof(cmd), "img %s speed %f", command.path, command.speed);

      if (ret < 0 || ret >= (int)sizeof(cmd)) {
          fprintf(stderr, "command too long\n");
          close(sock_fd);
          return EXIT_FAILURE;
      }
    }
    else if (command.img_s == eTrue && command.transition_s == eTrue ) {
      int ret = snprintf(cmd, sizeof(cmd), "img %s transition %s", command.path, command.transition);

      if (ret < 0 || ret >= (int)sizeof(cmd)) {
          fprintf(stderr, "command too long\n");
          close(sock_fd);
          return EXIT_FAILURE;
      }
    }
    else if (command.img_s == eTrue) {
      int ret = snprintf(cmd, sizeof(cmd), "img %s", command.path);

      if (ret < 0 || ret >= (int)sizeof(cmd)) {
          fprintf(stderr, "command too long\n");
          close(sock_fd);
          return EXIT_FAILURE;
      }
    }
    else if (command.transition_s == eTrue) {
      int ret = snprintf(cmd, sizeof(cmd), "transition %s", command.transition);

      if (ret < 0 || ret >= (int)sizeof(cmd)) {
          fprintf(stderr, "command too long\n");
          close(sock_fd);
          return EXIT_FAILURE;
      }
    }
    else if (command.speed_s == eTrue) {
      snprintf(cmd, sizeof(cmd), "speed %f",command.speed);
    }
    else if (command.query_s == eTrue) {
      const char * currentWall = get_cached_wallpaper();
      fprintf(stdout, "current wallpaper : %s\n",  currentWall);
      close(sock_fd);
      return EXIT_SUCCESS;
    }
    
    if (cmd[0] == '\0') {
      fprintf(stderr, "No valid command to send\n");
      close(sock_fd);
      return EXIT_FAILURE;
    }
    size_t len = strlen(cmd);
    size_t total = 0;

    while (total < len) {
        ssize_t n = write(sock_fd, cmd + total, len - total);
        if (n == -1) {
            perror("write to socket failed!\n");
            close(sock_fd);
            return EXIT_FAILURE;
        }
        total += n;
    }
 
    close(sock_fd);
    return EXIT_SUCCESS;
    
}

int validateTransition(char const* restrict string){
  if
      (
        strcmp(string, "fade") == 0 || 
        strcmp(string, "wipe") == 0 
      )
  {
    return EXIT_SUCCESS;
  }
  
  return EXIT_FAILURE;
};

int validateFloat(char const* restrict string, float* outValue) {
    if (string == NULL || *string == '\0') {
        return EXIT_FAILURE;
    }

    char* endptr;
    errno = 0; // Reset errno to catch out-of-range errors
    
    float val = strtof(string, &endptr);

    // 1. Check if any conversion happened (string != endptr)
    // 2. Check if we reached the end of the string (*endptr == '\0')
    // 3. Check for overflow/underflow (errno == 0)
    if (string != endptr && *endptr == '\0' && errno == 0 && isfinite(val)) {
        if (outValue) {
            *outValue = val;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int validateImgPath(char const* restrict imgPath) {
    struct stat fileStatus;
    
    if (stat(imgPath, &fileStatus) != 0) {
        return EXIT_FAILURE;
    }
    
    if (!S_ISREG(fileStatus.st_mode)) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printHelp();
        return EXIT_FAILURE;
    }
    
    
    struct Command command = {
      .img_s = eFalse,
      .speed_s = eFalse,
      .query_s = eFalse,
      .transition_s = eFalse,
      .path = NULL,
      .speed = 0.05f,
      .transition = NULL
    };

    for (size_t argsItr = 1; argsItr < argc; ++argsItr) {
        
        char* argument = argv[argsItr];
        
        if (strcmp(argument, "-h") == 0 || strcmp(argument, "--help") == 0) {
            printHelp();
            return EXIT_SUCCESS;
        }
        
        else if (strcmp(argument, "img") == 0 || strcmp(argument, "-i") == 0) {
            if (command.img_s == eTrue) {
                fprintf(stderr,"ignoring duplicate request: 'img'\n");
            }
            else if ((argsItr + 1 < argc) && validateImgPath(argv[argsItr + 1]) == EXIT_SUCCESS) {
                command.img_s = eTrue;
                command.path = argv[++argsItr];
            }
            else {
                command.img_s = eError;
                fprintf(stderr,"invalid argument for 'img'. expected valid path!\n");
            }
        }
        
        else if (strcmp(argument, "speed") == 0 || strcmp(argument, "-s") == 0) {
            if (command.speed_s == eTrue) {
                fprintf(stderr,"ignoring duplicate request: 'speed'\n");
            }
            else if ((argsItr + 1 < argc) && validateFloat(argv[argsItr + 1], &command.speed) == EXIT_SUCCESS) {
                command.speed_s = eTrue;
                argsItr++;
            }
            else {
                command.speed_s = eError;
                fprintf(stderr,"invalid argument for 'speed'. expected a float!\n");
            }
        }

        else if (strcmp(argument, "transition") == 0 || strcmp(argument, "-t") == 0) {
            if (command.transition_s == eTrue) {
                fprintf(stderr,"ignoring duplicate request: 'transition'\n");
            }
            else if ((argsItr + 1 < argc) && validateTransition(argv[argsItr + 1]) == EXIT_SUCCESS) {
                command.transition_s = eTrue;
                command.transition = argv[++argsItr];
            }
            else {
                command.transition_s = eError;
                fprintf(stderr,"invalid argument for 'transition'. expected a valid transition!\n");
                printAvailableTransition();
            }
        }

        else if (strcmp(argument, "query") == 0 || strcmp(argument, "-q") == 0) {
            if (command.query_s == eTrue) {
                printf("ignoring duplicate request: 'query'\n");
            }
            else {
                command.query_s = eTrue;
            }
        }
        
        else {
            fprintf(stderr,"invalid request recieved: '%s'. please provide a valid request! try -h or --help for information\n" , argument);
            return EXIT_FAILURE;
        }
    }
    
    if ( command.img_s == eError || command.speed_s == eError || command.transition_s == eError) {
        fprintf(stderr,"Exiting due to errors!");
        return EXIT_FAILURE;
    }

    if (command.query_s == eTrue && (command.img_s == eTrue || command.speed_s == eTrue || command.transition_s == eTrue)) {
      fprintf(stderr, "query cannot be combined with other commands\n");
      return EXIT_FAILURE;
    }

    if (handleSocket(command) == EXIT_FAILURE) {
      fprintf(stderr, "command failed\n");
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
