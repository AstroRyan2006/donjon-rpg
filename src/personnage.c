#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "personnage.h"

const char *nomClasse(enum Classe c) {
    switch (c) {
        case GUERRIER: return "Guerrier";
        case MAGE:     return "Mage";
        case VOLEUR:   return "Voleur";
    }
    return "Inconnu";
}

/* Lit un entier au clavier de facon securisee (fgets + strtol) */
static int lireEntier(void) {
    char buf[64];
    if (fgets(buf, sizeof buf, stdin) == NULL) return 0;
    return (int)strtol(buf, NULL, 10);
}

void initPersonnage(Personnage *p, const char *nom, enum Classe classe) {
    if (p == NULL) return;
    snprintf(p->nom, sizeof p->nom, "%s", nom ? nom : "");
    if (p->nom[0] == '\0') strcpy(p->nom, "Sans-nom");
    p->classe = classe;

    /* Statistiques selon la classe */
    switch (classe) {
        case GUERRIER: p->pv = 120; p->atk = 15; p->def = 10; break;
        case MAGE:     p->pv = 80;  p->atk = 25; p->def = 4;  break;
        case VOLEUR:   p->pv = 90;  p->atk = 18; p->def = 6;  break;
    }
}

void creerPersonnage(Personnage *p) {
    if (p == NULL) return;                       /* robustesse : pointeur NULL */

    char nom[30];
    printf("Nom : ");
    if (fgets(nom, sizeof nom, stdin) == NULL) nom[0] = '\0';
    nom[strcspn(nom, "\n")] = '\0';              /* enleve le retour a la ligne */

    int choix;
    do {
        printf("Classe (0=Guerrier, 1=Mage, 2=Voleur) : ");
        choix = lireEntier();
    } while (choix < 0 || choix > 2);

    initPersonnage(p, nom, (enum Classe)choix);
}

void afficherPersonnage(const Personnage *p) {
    if (p == NULL) return;
    printf("%-15s | %-8s | PV: %3d | ATK: %2d | DEF: %2d\n",
           p->nom, nomClasse(p->classe), p->pv, p->atk, p->def);
}

void attaquer(Personnage *attaquant, Personnage *cible) {
    if (attaquant == NULL || cible == NULL) return;
    if (attaquant->pv <= 0) {
        printf("%s est KO et ne peut pas attaquer.\n", attaquant->nom);
        return;
    }
    if (cible->pv <= 0) {
        printf("%s est deja KO.\n", cible->nom);
        return;
    }
    int degats = attaquant->atk - cible->def;
    if (degats < 1) degats = 1;                  /* minimum 1 degat */
    if (degats > cible->pv) degats = cible->pv;  /* on ne perd pas plus que ses PV */
    cible->pv -= degats;
    printf("%s attaque %s : -%d PV (reste %d)\n",
           attaquant->nom, cible->nom, degats, cible->pv);
}
