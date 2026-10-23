#include <stdio.h>
#include <stdlib.h>
#include "personnage.h"
#include "sauvegarde.h"

#define FICHIER "donjon.bin"

static int lireEntier(void) {
    char buf[64];
    if (fgets(buf, sizeof buf, stdin) == NULL) return 0;
    return (int)strtol(buf, NULL, 10);
}

int main(void) {
    Personnage *liste = NULL;   /* tableau dynamique */
    int n = 0;
    int choix;

    do {
        printf("\n===== LE DONJON - GESTION RPG =====\n");
        printf("1. Creer un personnage\n");
        printf("2. Afficher les personnages\n");
        printf("3. Lancer un combat\n");
        printf("4. Sauvegarder la partie\n");
        printf("5. Charger une partie\n");
        printf("0. Quitter\n> ");
        choix = lireEntier();

        switch (choix) {
        case 1: {
            /* realloc : agrandit le tableau d'une case (malloc si liste == NULL) */
            Personnage *tmp = realloc(liste, (size_t)(n + 1) * sizeof(Personnage));
            if (tmp == NULL) {
                perror("Erreur realloc");
                break;
            }
            liste = tmp;
            creerPersonnage(&liste[n]);
            n++;
            break;
        }
        case 2:
            if (n == 0) printf("Aucun personnage.\n");
            for (int i = 0; i < n; i++) {
                printf("[%d] ", i);
                afficherPersonnage(&liste[i]);
            }
            break;
        case 3: {
            if (n < 2) { printf("Il faut au moins 2 personnages.\n"); break; }
            printf("Index de l'attaquant : ");
            int a = lireEntier();
            printf("Index de la cible : ");
            int c = lireEntier();
            if (a < 0 || a >= n || c < 0 || c >= n || a == c) {
                printf("Index invalides.\n");
                break;
            }
            attaquer(&liste[a], &liste[c]);
            if (liste[c].pv <= 0) {
                printf("%s est elimine du donjon !\n", liste[c].nom);
                /* decale les elements suivants d'une case vers la gauche */
                for (int i = c; i < n - 1; i++)
                    liste[i] = liste[i + 1];
                n--;
                if (n > 0) {
                    Personnage *tmp = realloc(liste, (size_t)n * sizeof(Personnage));
                    if (tmp != NULL) liste = tmp;  /* si echec, on garde l'ancien tableau */
                }
            }
            break;
        }
        case 4:
            sauvegarder(FICHIER, liste, n);
            break;
        case 5:
            charger(FICHIER, &liste, &n);
            break;
        case 0:
            printf("Au revoir !\n");
            break;
        default:
            printf("Choix invalide.\n");
        }
    } while (choix != 0);

    free(liste);   /* libération obligatoire de la mémoire */
    return 0;
}
