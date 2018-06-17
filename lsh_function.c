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

#define LSH_RL_BUFSIZE 1024 //taille standart de notre bloc (read)
#define LSH_TOK_BUFSIZE 64 //taille standart de notre bloc
#define LSH_TOK_DELIM " \t\r\n\a" //delimiteurs

//Commandes internes implementees dans notre programme
char *builtin_str[] =
{
  "cd",
  "help",
  "exit",
  "changeEnv",
  "cat",
  "find",
  "ls",
  "cp"
};



/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process, on remplace processus par notre programme
    if (execvp(args[0], args) == -1) //erreur programme
    {
      perror("lsh");
    }
    printf("line 50\n");
    exit(EXIT_FAILURE);//on affiche informations de l'erreur
  }
  else if (pid < 0) //erreur fork
  {
    perror("lsh");
  } else //le pere recoit le numero du PID du fils
  {
    do
    {
      waitpid(pid, &status, WUNTRACED);//on attends processus mort ou termine
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));//tant que processus pas terminer normalement(WIFEXITED) ou stopper par erreur (WIFIBLOCKED)
  }

  return 1;
}

//Execution en cas de commande redirection ou pipe
int lsh_launchSpeciale(char ***commandes)
{
    pid_t pid;
    int pfd[2];
    int backUpStdOUT;
    int backUpStdIN;
    int backUpStdERR;
    int position = 0;
    while (commandes[position+2] != NULL) //Tant que redirection ou pipe possible
    {
        printf("entree en redirection\n");
        pipe(pfd);
        pid = fork();
        if(pid==-1)//fork erreur
        {
            printf("Child process could not be created\n");
            return -1;
        }
        //https://github.com/jmreyes/simple-c-shell/blob/master/simple-c-shell.c
        //https://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell
        if(pid==0)//fils
        {

            if(strcmp(commandes[position+1][0],">")==0)//redirection stdout
            {
                pfd[1] = open(commandes[position+2][0], O_CREAT | O_TRUNC | O_WRONLY, 0600);
                backUpStdOUT = dup(STDOUT_FILENO);
                close(STDOUT_FILENO);
                close(pfd[0]);
                if(position != 0)//pas premier bout de commande
                {
					dup2(pfd[0], STDIN_FILENO); //on recupere input
				}
				dup2(pfd[1], STDOUT_FILENO); //on transfere output
                position += 2;
                if (execvp(commandes[position-2][0],commandes[position-2])==-1){
                    printf("error");
                    kill(getpid(),SIGTERM);
                }
                dup2(backUpStdOUT,STDOUT_FILENO);
            }
            else if(strcmp(commandes[position+1][0],"<")==0) // redirection stdin
            {
                pfd[0] = open(commandes[position+2][0], O_RDONLY, 0600); //on ouvre input
                backUpStdIN = dup(STDIN_FILENO);
                close(STDIN_FILENO);
                close(pfd[1]);
                dup2(pfd[0], STDIN_FILENO); //on transfere input
                position += 2;
                if (execvp(commandes[position-2][0],commandes[position-2])==-1)
                {
                    printf("error");
                    kill(getpid(),SIGTERM);
                }
                dup2(backUpStdIN,STDIN_FILENO);
            }
            else if(strcmp(commandes[position+1][0],"2>")==0) // redirection stderr
            {
                pfd[1] = open(commandes[position+2][0], O_CREAT | O_TRUNC | O_WRONLY, 0600); //on ouvre chemin
                backUpStdERR = dup(STDERR_FILENO);
                dup2(pfd[1], STDERR_FILENO); //on transfere stderr
                position += 2;
                if (execvp(commandes[position-2][0],commandes[position-2])==-1)
                {
                    printf("error");
                    fflush(stdout);
                    kill(getpid(),SIGTERM);
                }
                dup2(backUpStdERR,STDERR_FILENO);
            }
            else if(strcmp(commandes[position+1][0],"|")==0) // cas pipe
            {

                close(pfd[0]);
                printf("On est dans le fils dernier else \n");
                if(position != 0) //pas premier
                {
                    printf("On est dans le if 1 fils\n");
					dup2(pfd[0], STDIN_FILENO); //on recupere input
				}
                dup2(pfd[1], STDOUT_FILENO); // on transfere output
                printf("Il a fait dup\n");
                if (execvp(commandes[position][0],commandes[position])==-1){
                    printf("On est dans le if 2s fils\n");
                    printf("error");
                    kill(getpid(),SIGTERM);
                }
                else {
                    printf("On est dans else fils\n");
                }
            }

		}
		//pere
		if((strcmp(commandes[position+1][0],"|")==0) //on est cas redirection ou pipe
			||(strcmp(commandes[position+1][0],">")==0)
			||(strcmp(commandes[position+1][0],"<")==0)
			||(strcmp(commandes[position+1][0],"2>")==0))
		{
                printf("Début if\n");
			wait(&pid); // on attends fin fils
			close(pfd[1]);
			dup2(pfd[0], STDIN_FILENO); // on recupere input
			position+=2;
			if((strcmp(commandes[position+1][0],"|")==0) // si commande pas finit, on attends nouvelle boucle
			||(strcmp(commandes[position+1][0],">")==0)
			||(strcmp(commandes[position+1][0],"<")==0)
			||(strcmp(commandes[position+1][0],"2>")==0))
			{
			    printf("Second if\n");

			}
			else //commande finit
			{
			    printf("On est dans else\n");
				if (execvp(commandes[position][0],commandes[position])==-1) //on peut executer
				{
				    printf("Debut 3 if\n");
					printf("error");
					kill(getpid(),SIGTERM);
				}
				else{
                    printf("Fin else\n");
				}
                return lsh_launch(args);

			}
			printf("Fin if\n");
		}
	}
	printf("Ending process\n");
    return 1;

}


/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "lsh: allocation error\n");
    printf("line 198\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    c = getchar();//On lit caractere de la ligne

    if (c == EOF) //End of file, on peut sortir de la boucle
    {
        printf("line 208\n");
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    //depasse buffer, on doit reallouer
    if (position >= bufsize)
    {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
              printf("line 227\n");
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


/*brise la ligne en arguments
renvoie le tableau des arguments*/
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens)
  {
          printf("line 246\n");
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);//couper ligne selon delimiteurs
  while (token != NULL)//on parcourt arguments
  {
    tokens[position] = token;
    position++;

    //depasse buffer, on doit reallouer
    if (position >= bufsize)
    {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
            printf("line \n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);//on passe le delimiteur
  }
  tokens[position] = NULL;
  return tokens;
}



int lsh_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

int lsh_changeEnv(char **args)
{
	if (args[1] == NULL)
	{
		fprintf(stderr, "Merci de donner le nom de la variable d'environnement\n");
	}
	else
	{
		if (args[2] == NULL)
		{
			fprintf(stderr, "Merci de donner le nouveau contenu de la variable d'environnement\n");
		}
		else
		{
			setenv (args[1], args[2], 1);
		}
	}
	return 1;
}


/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "Pas d'arguments rentrés, Directory monte d'un cran\n");
        chdir("..");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("lsh");
        }
    }
    return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Projet de Système de Justine Thanh NGUYEN HOANG TUNG et Anandou CANDASSAMY\n");
  printf("Taper la commande que vous voulez tester\n");
  printf("Voici la liste des commandes disponibles:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Se reporter au rapport pour plus d'information :).\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
   @brief Bultin command: print file data.
   @param args List of args.  args[0] is "cat". args[1] is the file.
   @return Always returns 1, to continue executing.
 */
int lsh_cat(char **args)
{
	if (args[1] == NULL) //Pas assez d'arguments
	{
		fprintf(stderr, "Pas d'arguments rentrés, Donnez fichier\n");
	}
	else if(args[2] != NULL) //Trop d'arguments
	{
		fprintf(stderr, "Trop d'arguments rentrés\n");

	}
	else{
		struct stat fileStat;
		stat(args[1],&fileStat);
		if(S_ISREG(fileStat.st_mode)){
			FILE *fp = NULL;
			fp = fopen(args[1],"r");
			if(fp != NULL){
				int caractere = 0;
				do{
					caractere = fgetc(fp);
					printf("%c",caractere);
					//strcat(monresultat,caractere)
				}while(caractere != EOF);
				fclose(fp);
			}
			else{
				perror("erreur lecture fichier\n");
			}
		}
		else{
			fprintf(stderr,"Not a file\n");
		}

	}
	return 1;
}

/**
   @brief Bultin command: find file.
   @param args List of args.  args[0] is "find".  args[1] is param or file. args[2] is file or null.
   @return Always returns 1, to continue executing.
 */
int lsh_find(char **args)
{
	//on a un argument
	if(args[2] == NULL || args[1] == NULL){
		fprintf(stderr, "Erreur : nombre d'arguments insuffisants\n");
	}
	else if(args[3] != NULL){
		if((strcmp(args[2],"-exec")==0)){
			//On sauvegarde le path actuel
			char cwd[1024];
			getcwd(cwd, sizeof(cwd));

			//On récupere la commande a exec (ainsi que les possibles parametres)
			char **arguments = calloc(100,sizeof(char*));
			char *argument = calloc(100,sizeof(char));
			int indiceCommande = 3;
			int indiceArguments = 0;
			while(strcmp(args[indiceCommande],"{}")!=0){
				arguments[indiceArguments]=args[indiceCommande];
				indiceArguments++; indiceCommande++;
			}

			//On se dirige la ou la commande doit être exec
			char **direction = calloc(100,sizeof(char*));
			direction[0] = "cd";
			if(strcmp(args[1],".")==0)
				direction[1] = "/";
			else
				direction[1] = args[1];
			lsh_cd(direction);

			//On execute la commande dans un process fils
			pid_t pid;
			pid = fork();
			int status;
			if (pid == 0){
    				// Child process, on remplace processus par notre programme
    				if (execvp(args[3], arguments) == -1){
      					perror("lsh");
    				}
                    printf("line 443\n");
    				exit(EXIT_FAILURE);//on affiche informations de l'erreur
  			}
  			else if(pid < 0){
    				perror("lsh");
  			}
			else{ //le pere recoit le numero du PID du fils
  				do {
      					waitpid(pid, &status, WUNTRACED);//on attends processus mort ou termine
    				} while (!WIFEXITED(status) && !WIFSIGNALED(status));//tant que processus pas terminer normalement(WIFEXITED) ou stopper par erreur (WIFIBLOCKED)
  			}

			//On se redirige vers le chemin d'origine de l'exec de la commande find
			char **redirection = malloc(100*sizeof(char*));
			direction[0] = "cd";
			direction[1] = cwd;
			lsh_cd(direction);

			free(argument);
			free(arguments);

		}
		else{
			fprintf(stderr, "Erreur d'arguments \n");
		}
	}
	else{
		// Si c'est l'option name
		if(strcmp(args[1],"-name") ==0){
			struct dirent *lecture;
			DIR *rep;
			rep = opendir("." );
			char* path = calloc(500,sizeof(char));
			strcat(path,"./");
			while ((lecture = readdir(rep))) {
				if(strcmp(lecture->d_name,args[2])==0){
					strcat(path,lecture->d_name);
					printf("%s\n",path);
				}
				if (lecture->d_type == DT_DIR){ //Si c'est un dossier
					char* cheminTampon = calloc(200,sizeof(char));
					strcat(cheminTampon,"./");
					lectureDir(cheminTampon,lecture->d_name,args[2]);
				}

			}
			closedir(rep);
		}
		//Si l'option est incorrect
		else{
			fprintf(stderr,"Nom d'option incorrect\n");
		}
	}
	return 1;
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_ls(char **args){
	if(args[1] != NULL){
		int i=1;
		int a=0;
		int l=0;
		int la=0;
		while(args[i] != NULL){
			if(strcmp(args[i],"-la") == 0 || strcmp(args[i],"-al")==0){
				la =1;
			}
			else if(strcmp(args[i],"-l") == 0 && l==0){
				l=1;
			}
			else if(strcmp(args[i],"-a") == 0 && l==0){
				a=1;
			}
			i++;
		}
		if(la==1 || (a==1 && l==1)){
						struct dirent *lecture;
			DIR *rep;
			rep = opendir("." );
			char* path = calloc(500,sizeof(char));
			strcat(path,"./");
			while ((lecture = readdir(rep))) {
				char* resultat = calloc(200,sizeof(char));
				struct stat fileStat;
				stat(lecture->d_name,&fileStat);
				printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
				printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
				printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
				printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
				printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
				printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
				printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
				printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
				printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
				printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
				printf(" %d",fileStat.st_nlink);
				printf(" %s",getpwuid(fileStat.st_uid)->pw_name);
				printf(" %d",fileStat.st_size);

				//Partie heure
				char *mois = calloc(10,sizeof(char));
				char *jour = calloc(10,sizeof(char));
				char *date = calloc(20,sizeof(char));
				strftime(jour, 50,"%d", localtime(&fileStat.st_mtime));
				strftime(mois, 50, "%m", localtime(&fileStat.st_mtime));
				strftime(date, 50, "%H:%M", localtime(&fileStat.st_mtime));


				char moisLettres[4];
				if(strcmp(mois,"01") ==0){
					moisLettres[0] = 'j';
					moisLettres[1] = 'a';
					moisLettres[2] = 'n';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"02") ==0){
					moisLettres[0] = 'f';
					moisLettres[1] = 'e';
					moisLettres[2] = 'v';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"03") ==0){
					moisLettres[0] = 'm';
					moisLettres[1] = 'a';
					moisLettres[2] = 'r';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"04") ==0){
					moisLettres[0] = 'a';
					moisLettres[1] = 'v';
					moisLettres[2] = 'r';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"05") ==0){
					moisLettres[0] = 'm';
					moisLettres[1] = 'a';
					moisLettres[2] = 'i';
				}
				else if(strcmp(mois,"06") ==0){
					moisLettres[0] = 'j';
					moisLettres[1] = 'u';
					moisLettres[2] = 'n';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"07") ==0){
					moisLettres[0] = 'j';
					moisLettres[1] = 'u';
					moisLettres[2] = 'l';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"08") ==0){
					moisLettres[0] = 'a';
					moisLettres[1] = 'o';
					moisLettres[2] = 'u';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"09") ==0){
					moisLettres[0] = 's';
					moisLettres[1] = 'e';
					moisLettres[2] = 'p';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"10") ==0){
					moisLettres[0] = 'o';
					moisLettres[1] = 'c';
					moisLettres[2] = 't';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"11") ==0){
					moisLettres[0] = 'n';
					moisLettres[1] = 'o';
					moisLettres[2] = 'v';
					//moisLettres[3] = '.';
				}
				else if(strcmp(mois,"12") ==0){
					moisLettres[0] = 'd';
					moisLettres[1] = 'e';
					moisLettres[2] = 'c';
					//moisLettres[3] = '.';
				}
				printf("   %s %c%c%c %s ",jour,moisLettres[0],moisLettres[1],moisLettres[2],date);
				strcat(resultat,lecture->d_name);

				printf(" %s\n",resultat);

			}
		}
		else if(l==1){
			struct dirent *lecture;
			DIR *rep;
			rep = opendir("." );
			char* path = calloc(500,sizeof(char));
			strcat(path,"./");
			while ((lecture = readdir(rep))) {
				if(strncmp(lecture->d_name,".",1)!=0){
					char* resultat = calloc(200,sizeof(char));
					struct stat fileStat;
					stat(lecture->d_name,&fileStat);
					printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
					printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
					printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
					printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
					printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
					printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
					printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
					printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
					printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
					printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
					printf(" %d",fileStat.st_nlink);
					printf(" %s",getpwuid(fileStat.st_uid)->pw_name);
					printf(" %d",fileStat.st_size);

					//Partie heure
					char *mois = calloc(10,sizeof(char));
					char *jour = calloc(10,sizeof(char));
					char *date = calloc(20,sizeof(char));
					strftime(jour, 50,"%d", localtime(&fileStat.st_mtime));
					strftime(mois, 50, "%m", localtime(&fileStat.st_mtime));
					strftime(date, 50, "%H:%M", localtime(&fileStat.st_mtime));


					char moisLettres[4];
					if(strcmp(mois,"01") ==0){
						moisLettres[0] = 'j';
						moisLettres[1] = 'a';
						moisLettres[2] = 'n';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"02") ==0){
						moisLettres[0] = 'f';
						moisLettres[1] = 'e';
						moisLettres[2] = 'v';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"03") ==0){
						moisLettres[0] = 'm';
						moisLettres[1] = 'a';
						moisLettres[2] = 'r';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"04") ==0){
						moisLettres[0] = 'a';
						moisLettres[1] = 'v';
						moisLettres[2] = 'r';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"05") ==0){
						moisLettres[0] = 'm';
						moisLettres[1] = 'a';
						moisLettres[2] = 'i';
					}
					else if(strcmp(mois,"06") ==0){
						moisLettres[0] = 'j';
						moisLettres[1] = 'u';
						moisLettres[2] = 'n';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"07") ==0){
						moisLettres[0] = 'j';
						moisLettres[1] = 'u';
						moisLettres[2] = 'l';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"08") ==0){
						moisLettres[0] = 'a';
						moisLettres[1] = 'o';
						moisLettres[2] = 'u';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"09") ==0){
						moisLettres[0] = 's';
						moisLettres[1] = 'e';
						moisLettres[2] = 'p';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"10") ==0){
						moisLettres[0] = 'o';
						moisLettres[1] = 'c';
						moisLettres[2] = 't';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"11") ==0){
						moisLettres[0] = 'n';
						moisLettres[1] = 'o';
						moisLettres[2] = 'v';
						//moisLettres[3] = '.';
					}
					else if(strcmp(mois,"12") ==0){
						moisLettres[0] = 'd';
						moisLettres[1] = 'e';
						moisLettres[2] = 'c';
						//moisLettres[3] = '.';
					}
					printf("   %s %c%c%c %s ",jour,moisLettres[0],moisLettres[1],moisLettres[2],date);
					strcat(resultat,lecture->d_name);

					printf(" %s\n",resultat);
				}
			}
		}
		else if(a==1){
			struct dirent *lecture;
			DIR *rep;
			rep = opendir("." );
			char* path = calloc(500,sizeof(char));
			strcat(path,"./");
			while ((lecture = readdir(rep))) {
				char* resultat = calloc(200,sizeof(char));
				strcat(resultat,lecture->d_name);
				printf("%s\n",resultat);
			}
		}
		else{
			fprintf(stderr,"Erreur d'argument");
		}
	}
	else{
		struct dirent *lecture;
		DIR *rep;
		rep = opendir("." );
		char* path = calloc(500,sizeof(char));
		strcat(path,"./");
		while ((lecture = readdir(rep))) {
			if(strncmp(lecture->d_name,".",1)!=0){
				char* resultat = calloc(200,sizeof(char));
				strcat(resultat,lecture->d_name);
				printf("%s\n",resultat);
			}
		}
	}
	return 1;
}

int lsh_cp(char **args){
	if(args[1] == NULL || args[2] == NULL){
		fprintf(stderr,"Necessite au moins deux arguments\n");
	}
	else {
		//On regarde si la source est un dossier ou un fichier simple
		struct stat s;
		lstat(args[1], &s);

		if(S_ISDIR(s.st_mode) == 1){ // notre fichier source est un dossier
			char *source = calloc(150,sizeof(char));
			char *dest = calloc(150,sizeof(char));


			if(strncmp(args[1],"/",1)==0 && strncmp(args[2],"/",1)==0){ //on a un chemin complet pour les deux
				//strcpy(source,args[1]);
				//strcpy(dest,args[2]);
			}
			else if(strncmp(args[1],"/",1)!=0 && strncmp(args[2],"/",1)==0){ //on a un chemin complet que pour le 2
				strcpy(source,".");
				//strcpy(dest,args[2]);
			}
			else if(strncmp(args[1],"/",1)==0 && strncmp(args[2],"/",1)!=0){ //chemin complet que pour le 1
				//strcpy(source,args[1]);
				strcpy(dest,".");
			}
			else{ //Aucun chemin complet
				strcpy(source,".");
				strcpy(dest,".");
			}
			copieDir(args[1],args[2],source,dest,1);
		}

		else if(S_ISREG(s.st_mode)==1){ //notre fichier source n'est ni un dossier ni un lien
			int fichierSource;
			int fichierDest;
			char buffer[LSH_RL_BUFSIZE];
			size_t buffer_read;
			if((fichierSource=open(args[1],O_RDONLY))==-1){
			    perror("open");
			}
			if((fichierDest=open(args[2], O_WRONLY|O_CREAT|O_TRUNC,s.st_mode))==-1){
			    perror("open");
			}
			while((buffer_read=read(fichierSource,buffer,LSH_RL_BUFSIZE))>0){
    				write(fichierDest,buffer,buffer_read);
			}
		}
	}
	return 1;
}

int (*builtin_func[]) (char **) =
{
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_changeEnv,
  &lsh_cat,
  &lsh_find,
  &lsh_ls,
  &lsh_cp
};

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
    int i;
    if (args[0] == NULL)
    {
    // An empty command was entered.
        return 1;
    }

    char ***commandes = calloc(1024, sizeof(char**)); // char*** contenant nos commandes ou delimiteurs

    for (i=0;i<30;i++) //on alloue
    {
		commandes[i]= (char**)calloc(1024, sizeof(char*));
		commandes[i][0] = (char*)calloc(1024, sizeof(char));
	}

    int position1 = 0;
    int position2 = 0;
    int k = 0;
    do
    {   //Si on trouve delimiteur, on ajoute et on change de char**
        if(strcmp(args[k],"|")==0)
        {
            commandes[position1+1][0] = "|";
            position1 += 2;
            position2 = 0;
        }
        else if(strcmp(args[k],">")==0)
        {
            commandes[position1+1][0] = ">";
            position1 += 2;
            position2 = 0;
        }
        else if(strcmp(args[k],"<")==0)
        {
            commandes[position1+1][0] = "<";
            position1 += 2;
            position2 = 0;
        }
        else if(strcmp(args[k],"2>")==0)
        {
            commandes[position1+1][0] = "2>";
            position1 += 2;
            position2 = 0;
        }
        else
        {
			if(strcmp(args[k],"\"")==0) //cas fichier avec espace mais inutile dans ce programme
			{
				k++;
				char* chaine = calloc(40,sizeof(char));
				while(strcmp(args[k],"\"")!=0)
				{
					strcat(chaine,args[k]);
					k++;
				}
			}
			else
			{
				commandes[position1][position2]=args[k];
				position2 += 1;
			}
        }
        k++;
    }while(args[k]!=NULL);
    if (position1 != 0) //on a trouve un delimiteur de redirection ou pipe
    {
        return lsh_launchSpeciale(commandes);
    }
    else //commande normale du systeme ou presente dans ce programme
    {
        for (i = 0; i < lsh_num_builtins(); i++)
        {
            if (strcmp(args[0], builtin_str[i]) == 0)
            {
                return (*builtin_func[i])(args);
            }
        }
    }
  return lsh_launch(args);
}


void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do
  {
    char* cwd;
    char buff[1000];
    char hostname[1024];
    gethostname(hostname, 2013);
    cwd = getcwd( buff,1000);

    printf("%s@%s:%s> ",getlogin(),hostname,cwd);
    line = lsh_read_line(); //on lit la ligne de commande
    args = lsh_split_line(line); //on sépare les arguments rentrés
    status = lsh_execute(args); //on execute la commande

    free(line);
    free(args);
  } while (status);
}





