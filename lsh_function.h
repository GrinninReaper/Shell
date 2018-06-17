#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>


int lsh_launch(char **args);
int lsh_launchSpeciale(char ***commandes);
int lsh_execute(char **args);
char *lsh_read_line(void);
char **lsh_split_line(char *line);
int lsh_num_builtins();
int lsh_changeEnv(char **args);
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_cat(char **args);
int lsh_find(char **args);
int lsh_ls(char **args);
int lsh_cp(char **args);
void lsh_loop(void);



