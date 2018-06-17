#include "lsh_function.h"


#define LSH_RL_BUFSIZE 1024 //taille standart de notre bloc (read)

void copieDir(char* dossierSource,char* dossierDest,char* pathSource,char* pathDest,int debut){
	struct stat s;
	lstat(dossierSource, &s);

	if(debut==1){
		strcat(pathSource,"/");
		strcat(pathDest,"/");
	}
	char* fichSource = calloc(1000,sizeof(char));
	strcat(fichSource,pathSource);
	if(debut==0)
		strcat(fichSource,"/");
	strcat(fichSource,dossierSource);
	strcat(fichSource,"/");
	DIR* source = opendir(fichSource);


	char* fichDest = calloc(1000,sizeof(char));
	strcat(fichDest,pathDest);
	if(debut==0)
		strcat(fichDest,"/");
	strcat(fichDest,dossierDest);
	strcat(fichDest,"/");
	struct stat s2;
	lstat(fichSource, &s2);
	mkdir(fichDest,0777);
	DIR* dest = opendir(fichDest);
	if(source == NULL ){
		printf("Erreur dans l'ouverture du dossier sourcen");
	}
	if(dest == NULL){
		printf("Erreur dans l'ouverture du dossier destination\n");
	}
	if(dest != NULL && source != NULL){
		struct dirent* fichierEnCours = NULL;
		char buffer [4096];
		while((fichierEnCours = readdir(source)) != NULL){
			if(strcmp(fichierEnCours->d_name,".")!=0 && strcmp(fichierEnCours->d_name,"..")!=0 ){
				char* cheminSource = calloc(1000,sizeof(char));
				strcpy(cheminSource,pathSource);
				if(debut==0)
					strcat(cheminSource,"/");
				strcat(cheminSource,dossierSource);
				strcat(cheminSource,"/");
				strcat(cheminSource,fichierEnCours->d_name);

				char* cheminDestination = calloc(1000,sizeof(char));
				strcpy(cheminDestination,pathDest);
				if(debut==0)
					strcat(cheminDestination,"/");
				strcat(cheminDestination,dossierDest);
				strcat(cheminDestination,"/");
				strcat(cheminDestination,fichierEnCours->d_name);

				struct stat sPrime;
				if((lstat(cheminSource, &sPrime)) == -1){
					perror("lstat Fichier");
				}
				if(S_ISDIR(sPrime.st_mode) == 1){ //si le fichier lu est un dossier
					char* newPathSource = calloc(1000,sizeof(char));
					strcpy(newPathSource,pathSource);
					if(debut==0)
						strcat(newPathSource,"/");
					strcat(newPathSource,dossierSource);

					char* newPathDest = calloc(1000,sizeof(char));
					strcpy(newPathDest,pathDest);
					if(debut==0)
						strcat(newPathDest,"/");
					strcat(newPathDest,dossierDest);
					copieDir(fichierEnCours->d_name,fichierEnCours->d_name,newPathSource,newPathDest,0);
					free(newPathSource);
					free(newPathDest);
				}

				else if(S_ISREG(sPrime.st_mode) == 1){ // si le fichier lu est fichier
					int fichierSource;
					int fichierDest;
					char buffer[LSH_RL_BUFSIZE];
					size_t buffer_read;
					if((fichierSource=open(cheminSource,O_RDONLY))==-1){
						perror("open fichier Source");
					}
					if((fichierDest=open(cheminDestination, O_WRONLY|O_CREAT|O_TRUNC,s.st_mode))==-1){
						perror("open fichier Destination");
					}
					while((buffer_read=read(fichierSource,buffer,LSH_RL_BUFSIZE))>0){
							write(fichierDest,buffer,buffer_read);
					}
				}

				free(cheminSource);
				free(cheminDestination);
			}
		}
		closedir(source);
		closedir(dest);
	}
	free(fichSource);
	free(fichDest);
}


void lectureDir(char* cheminTampon,char* nameRepertoire,char* nomRecherche){
	struct dirent *lecture;
	DIR *rep;
	strcat(cheminTampon,nameRepertoire);
	rep = opendir(cheminTampon);
	if(rep == NULL){
		//fprintf(stderr,"Erreur ouverture fichier\n");
	}
	else{
		strcat(cheminTampon,"/");
		while ((lecture = readdir(rep))) {
			if(strcmp(lecture->d_name,nomRecherche)==0){
				char* resultat = calloc(300,sizeof(char));
				strcat(resultat,cheminTampon);
				strcat(resultat,lecture->d_name);
				if(strncmp(resultat,"./.",3) !=0)
					printf("%s\n",resultat);

			}

			if ((strchr(lecture->d_name, '.')) == NULL){ //Si c'est un dossier (pas d'extension .)
				lectureDir(cheminTampon,lecture->d_name,nomRecherche);
			}

		}
		closedir(rep);
	}
}

