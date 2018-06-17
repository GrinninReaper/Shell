#include "dir_function.h"
#include "lsh_function.h"

//boucle du programme
int main(int argc, char **argv)
{
    printf("\e[1;1H\e[2J");
    printf("--------------------------------------------------------------------------------\n");
    char s[100] = "Bienvenue dans notre mini-shell!\n";
    printf("%*s\n", strlen(s) + (80 - strlen(s)) / 2, s);
    char r[150] = "Pour toutes informations complémentaires ne pas nous demander et chercher dans le MAN\n";
    printf("%s\n",r);
    printf("--------------------------------------------------------------------------------\n");
    // Run command loop.
    lsh_loop();

    // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
