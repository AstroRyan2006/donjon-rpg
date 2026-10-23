#include <stdio.h>
#include <stdlib.h>
#include "sauvegarde.h"

void sauvegarder(const char *nomFichier, Personnage *liste, int n) {
    FILE *f = fopen(nomFichier, "wb");
    if (f == NULL) {
        perror("Erreur ouverture fichier (ecriture)");
        return;
    }
    /* On ecrit d'abord le nombre de personnages, puis le tableau */
    if (fwrite(&n, sizeof(int), 1, f) != 1 ||
        fwrite(liste, sizeof(Personnage), (size_t)n, f) != (size_t)n) {
        perror("Erreur d'ecriture");
    } else {
        printf("Partie sauvegardee (%d personnage(s)).\n", n);
    }
    fclose(f);
}

void charger(const char *nomFichier, Personnage **liste, int *n) {
    FILE *f = fopen(nomFichier, "rb");
    if (f == NULL) {
        perror("Erreur ouverture fichier (lecture)");
        return;
    }
    int nb;
    if (fread(&nb, sizeof(int), 1, f) != 1 || nb < 0) {
        fprintf(stderr, "Fichier de sauvegarde invalide.\n");
        fclose(f);
        return;
    }
    Personnage *nouveau = NULL;
    if (nb > 0) {
        nouveau = malloc((size_t)nb * sizeof(Personnage));
        if (nouveau == NULL) {
            perror("Erreur malloc");
            fclose(f);
            return;
        }
        if (fread(nouveau, sizeof(Personnage), (size_t)nb, f) != (size_t)nb) {
            fprintf(stderr, "Fichier corrompu.\n");
            free(nouveau);
            fclose(f);
            return;
        }
    }
    fclose(f);
    free(*liste);          /* libere l'ancien tableau */
    *liste = nouveau;
    *n = nb;
    printf("Partie chargee (%d personnage(s)).\n", nb);
}
