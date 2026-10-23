#ifndef SAUVEGARDE_H
#define SAUVEGARDE_H

#include "personnage.h"

/* Ecrit le tableau de personnages dans un fichier binaire */
void sauvegarder(const char *nomFichier, Personnage *liste, int n);
/* Relit le fichier binaire et realloue dynamiquement le tableau */
void charger(const char *nomFichier, Personnage **liste, int *n);

#endif
