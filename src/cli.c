/// @author : github.com/saber-88 @contributes: wallrift-daemon
/// @author : github.com/its-19818942118 @contributes: wallrift-cli

#define WALLRIFT_VERSION "1.5.0"

#include "command.h"
#include "file.h"
#include "log.h"
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/wallrift.sock"

void print_available_transitions() {
  printf("\033[1mAvailable transitons:\033[0m\n");
  printf("  \033[1mwipe\033[0m\n");
  printf("  \033[1mfade\033[0m\n");
  printf("  \033[1mnone\033[0m\n\n");
}
void print_help() {
  printf("\033[1mwallrift - A smooth parallax supported wallpaper "
         "engine\033[0m.\n\n");
  printf("\033[1m\033[4mUsage\033[0m: \033[1mwallrift <COMMAND>\033[0m\n\n");
  printf("\033[1m\033[04mCommands:\033[0m \n\n");
  printf("  \033[1mimg, -i\033[0m           sends the path of the wallpaper to "
         "the daemon.\n");
  printf("  \033[1mspeed, -s\033[0m         sets the speed( 0.00 - 1.00 ) for "
         "parallax.\n");
  printf("  \033[1mtransition, -t\033[0m    sets the transition type\n");
  printf("  \033[1mquery, -q\033[0m         prints the current applied wallpaper.\n");
  printf("  \033[1moutput, -o\033[0m        targets a specific Wayland monitor output name.\n"); 

  printf("\033[1m\033[04m\nOptions:\033[0m \n\n");
  printf("  \033[1m-h,--help\033[0m         prints this help message.\n");
  printf("  \033[1m-v,--version\033[0m      prints the current version of wallrift.\n");

}

int handle_socket(struct Command command) {
  if (command.query_s == eTrue) {
    printf("%s\n", get_cached_wallpaper());
    return EXIT_SUCCESS;
  }
  int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock_fd == -1) {
    perror("Failed to open socket\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;

  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCK_PATH);

  if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("Connection to socket failed!\n");
    fprintf(stderr, "Is wallrift-daemon running ?\n");
    close(sock_fd);
    return EXIT_FAILURE;
  }
  ssize_t sent = send_all(sock_fd, &command, sizeof(struct Command));

  if (sent < 0) {
    LOG_ERR("SOCK", "Failed to send command.");
  }

  close(sock_fd);
  return EXIT_SUCCESS;
}

int validate_output(char const *restrict name){
  if (!name || strlen(name) < 3 || strlen(name) > MAX_OUTPUT_LEN) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
int validate_transition(char const *restrict string) {
  if (strcmp(string, "fade") == 0 || strcmp(string, "wipe") == 0 ||
      strcmp(string, "none") == 0) {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
};

int validate_float(char const *restrict string, float *outValue) {
  if (string == NULL || *string == '\0') {
    return EXIT_FAILURE;
  }

  char *endptr;
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

int validate_img_path(char const *restrict imgPath) {
  struct stat fileStatus;

  if (stat(imgPath, &fileStatus) != 0) {
    return EXIT_FAILURE;
  }

  if (!S_ISREG(fileStatus.st_mode)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void print_version(){
  printf("%s\n",WALLRIFT_VERSION);
}
int main(int argc, char *argv[]) {

  if (argc < 2) {
    print_help();
    return EXIT_FAILURE;
  }

  struct Command command;
  memset(&command, 0, sizeof(command));

  command.img_s = eFalse;
  command.speed_s = eFalse;
  command.query_s = eFalse;
  command.transition_s = eFalse;
  command.ouput_s = eFalse;
  command.speed = 0.05f;

  for (size_t argsItr = 1; argsItr < argc; ++argsItr) {

    char *argument = argv[argsItr];

    if (strcmp(argument, "-h") == 0 || strcmp(argument, "--help") == 0) {
      print_help();
      return EXIT_SUCCESS;
    }

    else if (strcmp(argument, "-v") == 0 || strcmp(argument, "--version") == 0) {
      print_version();
      return EXIT_SUCCESS;
    }

    else if (strcmp(argument, "img") == 0 || strcmp(argument, "-i") == 0) {
      if (command.img_s == eTrue) {
        fprintf(stderr, "ignoring duplicate request: 'img'\n");
      } else if ((argsItr + 1 < argc) &&
                 validate_img_path(argv[argsItr + 1]) == EXIT_SUCCESS) {
        command.img_s = eTrue;
        strncpy(command.path, argv[++argsItr], MAX_PATH_LEN - 1);
        command.path[MAX_PATH_LEN - 1] = '\0';
      } else {
        command.img_s = eError;
        fprintf(stderr, "invalid argument for 'img'. expected valid path!\n");
      }
    }

    else if (strcmp(argument, "speed") == 0 || strcmp(argument, "-s") == 0) {

      float temp_speed = 0.0f;
      if (command.speed_s == eTrue) {
        fprintf(stderr, "ignoring duplicate request: 'speed'\n");
      } else if ((argsItr + 1 < argc) &&
                 validate_float(argv[argsItr + 1], &temp_speed) ==
                     EXIT_SUCCESS) {
        command.speed_s = eTrue;
        command.speed = temp_speed;
        argsItr++;
      } else {
        command.speed_s = eError;
        fprintf(stderr, "invalid argument for 'speed'. expected a float!\n");
      }
    }

    else if (strcmp(argument, "transition") == 0 ||
             strcmp(argument, "-t") == 0) {
      if (command.transition_s == eTrue) {
        fprintf(stderr, "ignoring duplicate request: 'transition'\n");
      } else if ((argsItr + 1 < argc) &&
                 validate_transition(argv[argsItr + 1]) == EXIT_SUCCESS) {
        command.transition_s = eTrue;
        strncpy(command.transition, argv[++argsItr], MAX_TRANS_LEN - 1);
        command.transition[MAX_TRANS_LEN - 1] = '\0';
      } else {
        command.transition_s = eError;
        fprintf(stderr, "invalid argument for 'transition'. expected a valid "
                        "transition!\n");
        print_available_transitions();
      }
    }

    else if (strcmp(argument, "output") == 0 || strcmp(argument, "-o") == 0) {
      if (command.ouput_s == eTrue) {
        fprintf(stderr, "ignoring duplicate request: 'output'\n");
      } else if ((argsItr + 1 < argc) && validate_output(argv[argsItr + 1]) == EXIT_SUCCESS) {
        command.ouput_s = eTrue;
        strncpy(command.output, argv[++argsItr], MAX_OUTPUT_LEN - 1);
        command.output[MAX_OUTPUT_LEN - 1] = '\0';
      } else {
        command.ouput_s = eError;
        fprintf(stderr, "invalid argument for 'output'. expected a valid "
                        "output name!\n");
      }
    }

    else if (strcmp(argument, "query") == 0 || strcmp(argument, "-q") == 0) {
      if (command.query_s == eTrue) {
        printf("ignoring duplicate request: 'query'\n");
      } else {
        command.query_s = eTrue;
      }
    }

    else {
      fprintf(stderr,
              "invalid request recieved: '%s'. please provide a valid request! "
              "try -h or --help for information\n",
              argument);
      return EXIT_FAILURE;
    }
  }

  if (command.img_s == eError || command.speed_s == eError ||
      command.transition_s == eError || command.ouput_s == eError) {
    fprintf(stderr, "Exiting due to errors!\n");
    return EXIT_FAILURE;
  }
  if (command.ouput_s == eTrue && (command.img_s == eFalse)) {
    fprintf(stderr, "output option needs img path\n");
    return EXIT_FAILURE;
  }

  if (command.query_s == eTrue &&
      (command.img_s == eTrue || command.speed_s == eTrue ||
       command.transition_s == eTrue)) {
    fprintf(stderr, "query cannot be combined with other commands\n");
    return EXIT_FAILURE;
  }

  if (handle_socket(command) == EXIT_FAILURE) {
    fprintf(stderr, "command failed\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
