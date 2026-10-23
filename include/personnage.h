#ifndef PERSONNAGE_H
#define PERSONNAGE_H

/* Classes de personnages */
enum Classe { GUERRIER, MAGE, VOLEUR };

/* Un personnage du donjon */
struct Personnage {
    char nom[30];
    int pv;
    int atk;
    int def;
    enum Classe classe;
};
typedef struct Personnage Personnage;

/* Demande les informations a l'utilisateur et remplit *p */
void creerPersonnage(Personnage *p);
/* Initialise un personnage a partir d'un nom et d'une classe (stats selon la classe) */
void initPersonnage(Personnage *p, const char *nom, enum Classe classe);
/* Affiche un personnage (lecture seule -> const) */
void afficherPersonnage(const Personnage *p);
/* L'attaquant inflige des degats a la cible (modifie *cible via pointeur) */
void attaquer(Personnage *attaquant, Personnage *cible);
/* Nom de la classe sous forme de texte */
const char *nomClasse(enum Classe c);

#endif
