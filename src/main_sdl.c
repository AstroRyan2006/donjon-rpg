/* Interface graphique SDL du Donjon (texte via SDL_ttf, boutons a la souris)
   Clavier : Gauche/Droite = attaquant, Haut/Bas = cible, ESPACE = attaque,
             N = nouveau perso, R = rejouer, S = sauvegarder, L = charger, ECHAP = quitter
   Souris  : boutons a droite, clic gauche sur une ligne = attaquant, clic droit = cible */
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "personnage.h"
#include "sauvegarde.h"

#define FICHIER "donjon.bin"
#define MAX_PERSOS 10
#define LARGEUR_BARRE 200
#define NB_LOGS 4
#define LONG_LOG 90
#define TAILLE_NOM 16
#define NB_BOUTONS 5

static const SDL_Color BLANC = {255, 255, 255, 255};
static const SDL_Color GRIS  = {170, 170, 180, 255};
static const SDL_Color OR    = {255, 215,   0, 255};

/* Etat complet du jeu */
typedef struct {
    Personnage *liste;
    int n;
    int attaquant, cible;
    char journal[NB_LOGS][LONG_LOG];
    bool enCours;
    /* fenetre de creation de personnage */
    bool creation;
    char nomSaisi[TAILLE_NOM + 1];
    enum Classe classeChoisie;
    Uint32 tickOuverture;
} Jeu;

static const char *LIBELLES[NB_BOUTONS] = {
    "Nouveau perso", "Rejouer", "Sauvegarder", "Charger", "Quitter"
};

/* ---------- Petits utilitaires ---------- */

/* PV de depart d'une classe : sert de 100% pour la barre de vie */
static int pvMaxClasse(enum Classe c) {
    Personnage tmp;
    initPersonnage(&tmp, "", c);
    return tmp.pv;
}

static SDL_Color couleurClasse(enum Classe c) {
    SDL_Color rouge = {200, 60, 60, 255}, bleu = {70, 110, 230, 255}, vert = {80, 200, 90, 255};
    switch (c) {
        case GUERRIER: return rouge;
        case MAGE:     return bleu;
        default:       return vert;
    }
}

static const char *nomCouleur(enum Classe c) {
    switch (c) {
        case GUERRIER: return "rouge";
        case MAGE:     return "bleu";
        default:       return "vert";
    }
}

static bool dans(SDL_Rect r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

static SDL_Rect rectBouton(int i)  { SDL_Rect r = {710, 60 + i * 55, 170, 42}; return r; }
static SDL_Rect rectLigne(int i)   { SDL_Rect r = {24, 50 + i * 40 - 5, 470, 38}; return r; }
static SDL_Rect rectClasse(int i)  { SDL_Rect r = {300 + i * 120, 270, 110, 36}; return r; }
static SDL_Rect rectValider(void)  { SDL_Rect r = {300, 350, 150, 40}; return r; }
static SDL_Rect rectAnnuler(void)  { SDL_Rect r = {470, 350, 150, 40}; return r; }

/* Dessine un texte a l'ecran */
static void texte(SDL_Renderer *r, TTF_Font *f, const char *s, int x, int y, SDL_Color c) {
    if (!s || !*s) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(f, s, c);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        SDL_Rect dest = { x, y, surf->w, surf->h };
        SDL_RenderCopy(r, tex, NULL, &dest);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

/* Texte centre dans un rectangle */
static void texteCentre(SDL_Renderer *r, TTF_Font *f, const char *s, SDL_Rect cadre, SDL_Color c) {
    int w = 0, h = 0;
    if (TTF_SizeUTF8(f, s, &w, &h) != 0) return;
    texte(r, f, s, cadre.x + (cadre.w - w) / 2, cadre.y + (cadre.h - h) / 2, c);
}

/* Dessine un bouton (fond, contour, libelle) */
static void dessinerBouton(SDL_Renderer *r, TTF_Font *f, SDL_Rect cadre, const char *label,
                           bool survol, SDL_Color fond, SDL_Color contour) {
    if (survol) {   /* eclaircit le fond sans depasser 255 */
        fond.r = (Uint8)(fond.r + 35 > 255 ? 255 : fond.r + 35);
        fond.g = (Uint8)(fond.g + 35 > 255 ? 255 : fond.g + 35);
        fond.b = (Uint8)(fond.b + 35 > 255 ? 255 : fond.b + 35);
    }
    SDL_SetRenderDrawColor(r, fond.r, fond.g, fond.b, 255);
    SDL_RenderFillRect(r, &cadre);
    SDL_SetRenderDrawColor(r, contour.r, contour.g, contour.b, 255);
    SDL_RenderDrawRect(r, &cadre);
    texteCentre(r, f, label, cadre, BLANC);
}

/* Cherche un fichier de police sur le systeme (fc-match, puis chemins connus) */
static bool trouverPolice(char *chemin, size_t taille) {
    FILE *p = popen("fc-match -f '%{file}' sans 2>/dev/null", "r");
    if (p) {
        if (fgets(chemin, (int)taille, p) != NULL && chemin[0] == '/') {
            pclose(p);
            return true;
        }
        pclose(p);
    }
    const char *candidats[] = {
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };
    for (size_t i = 0; i < sizeof candidats / sizeof candidats[0]; i++) {
        FILE *f = fopen(candidats[i], "rb");
        if (f) {
            fclose(f);
            snprintf(chemin, taille, "%s", candidats[i]);
            return true;
        }
    }
    return false;
}

/* ---------- Logique du jeu ---------- */

/* Ajoute un message dans le journal (les plus anciens remontent) */
static void ajouterLog(Jeu *j, const char *msg) {
    for (int i = 0; i < NB_LOGS - 1; i++)
        strcpy(j->journal[i], j->journal[i + 1]);
    snprintf(j->journal[NB_LOGS - 1], LONG_LOG, "%s", msg);
}

/* Supprime le personnage d'index c du tableau (decalage vers la gauche) */
static void supprimerPersonnage(Jeu *j, int c) {
    for (int i = c; i < j->n - 1; i++)
        j->liste[i] = j->liste[i + 1];
    j->n--;
}

/* Nouvelle partie : 3 personnages par defaut */
static bool nouvellePartie(Jeu *j) {
    Personnage *tmp = malloc(3 * sizeof(Personnage));
    if (!tmp) { perror("malloc"); return false; }
    free(j->liste);
    j->liste = tmp;
    initPersonnage(&tmp[0], "Conan",   GUERRIER);
    initPersonnage(&tmp[1], "Merlin",  MAGE);
    initPersonnage(&tmp[2], "Garrett", VOLEUR);
    j->n = 3;
    j->attaquant = 0;
    j->cible = 1;
    return true;
}

static void actionRejouer(Jeu *j) {
    memset(j->journal, 0, sizeof j->journal);
    if (!nouvellePartie(j)) { j->enCours = false; return; }
    ajouterLog(j, "Nouvelle partie ! Bonne chance.");
}

static void actionAttaquer(Jeu *j) {
    if (j->n < 2) {
        ajouterLog(j, "Il ne reste qu'un personnage : clique sur Rejouer.");
    } else if (j->attaquant == j->cible) {
        ajouterLog(j, "Choisis une cible differente de l'attaquant.");
    } else {
        char nomA[30], nomC[30], msg[LONG_LOG];
        snprintf(nomA, sizeof nomA, "%s", j->liste[j->attaquant].nom);
        snprintf(nomC, sizeof nomC, "%s", j->liste[j->cible].nom);
        int avant = j->liste[j->cible].pv;
        attaquer(&j->liste[j->attaquant], &j->liste[j->cible]);
        snprintf(msg, sizeof msg, "%s attaque %s : -%d PV", nomA, nomC, avant - j->liste[j->cible].pv);
        ajouterLog(j, msg);
        /* Si la cible est morte : on la supprime du tableau */
        if (j->liste[j->cible].pv <= 0) {
            snprintf(msg, sizeof msg, "%s est elimine du donjon !", nomC);
            ajouterLog(j, msg);
            supprimerPersonnage(j, j->cible);
            if (j->attaquant > j->cible) j->attaquant--;   /* les index ont decale */
            if (j->cible >= j->n) j->cible = 0;
            if (j->n > 1 && j->cible == j->attaquant) j->cible = (j->cible + 1) % j->n;
        }
    }
}

static void actionSauver(Jeu *j) {
    sauvegarder(FICHIER, j->liste, j->n);
    ajouterLog(j, "Partie sauvegardee.");
}

static void actionCharger(Jeu *j) {
    FILE *test = fopen(FICHIER, "rb");
    if (!test) { ajouterLog(j, "Aucune sauvegarde trouvee."); return; }
    fclose(test);
    charger(FICHIER, &j->liste, &j->n);
    if (j->n > MAX_PERSOS) j->n = MAX_PERSOS;
    if (j->n == 0 && !nouvellePartie(j)) { j->enCours = false; return; }
    j->attaquant = 0;
    j->cible = (j->n > 1) ? 1 : 0;
    ajouterLog(j, "Partie chargee.");
}

static void ouvrirCreation(Jeu *j) {
    if (j->n >= MAX_PERSOS) { ajouterLog(j, "Le donjon est plein !"); return; }
    j->creation = true;
    j->nomSaisi[0] = '\0';
    j->classeChoisie = GUERRIER;
    j->tickOuverture = SDL_GetTicks();
    SDL_StartTextInput();
}

static void fermerCreation(Jeu *j) {
    j->creation = false;
    SDL_StopTextInput();
}

static void validerCreation(Jeu *j) {
    Personnage *tmp = realloc(j->liste, (size_t)(j->n + 1) * sizeof(Personnage));
    if (!tmp) {
        perror("realloc");
        ajouterLog(j, "Erreur memoire : personnage non cree.");
        fermerCreation(j);
        return;
    }
    j->liste = tmp;
    initPersonnage(&j->liste[j->n], j->nomSaisi, j->classeChoisie);
    j->n++;
    char msg[LONG_LOG];
    snprintf(msg, sizeof msg, "%s rejoint le donjon !", j->liste[j->n - 1].nom);
    ajouterLog(j, msg);
    if (j->n > 1 && j->cible == j->attaquant) j->cible = (j->attaquant + 1) % j->n;
    fermerCreation(j);
}

/* Action associee a un bouton du menu */
static void executerBouton(Jeu *j, int i) {
    switch (i) {
        case 0: ouvrirCreation(j); break;
        case 1: actionRejouer(j);  break;
        case 2: actionSauver(j);   break;
        case 3: actionCharger(j);  break;
        case 4: j->enCours = false; break;
        default: break;
    }
}

/* ---------- Evenements ---------- */

static void evenementCreation(Jeu *j, const SDL_Event *e) {
    if (e->type == SDL_TEXTINPUT) {
        /* On ignore la lettre de la touche qui a ouvert la fenetre (N) */
        if (SDL_GetTicks() - j->tickOuverture < 50) return;
        size_t l = strlen(j->nomSaisi), t = strlen(e->text.text);
        if (l + t <= TAILLE_NOM) memcpy(j->nomSaisi + l, e->text.text, t + 1);
    } else if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_ESCAPE: fermerCreation(j); break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: validerCreation(j); break;
        case SDLK_BACKSPACE: {
            size_t l = strlen(j->nomSaisi);
            if (l > 0) {
                l--;
                while (l > 0 && ((unsigned char)j->nomSaisi[l] & 0xC0) == 0x80) l--;  /* caractere UTF-8 */
                j->nomSaisi[l] = '\0';
            }
            break;
        }
        case SDLK_LEFT:  j->classeChoisie = (enum Classe)((j->classeChoisie + 2) % 3); break;
        case SDLK_RIGHT: j->classeChoisie = (enum Classe)((j->classeChoisie + 1) % 3); break;
        default: break;
        }
    } else if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        int x = e->button.x, y = e->button.y;
        for (int c = 0; c < 3; c++)
            if (dans(rectClasse(c), x, y)) j->classeChoisie = (enum Classe)c;
        if (dans(rectValider(), x, y)) validerCreation(j);
        else if (dans(rectAnnuler(), x, y)) fermerCreation(j);
    }
}

static void evenementJeu(Jeu *j, const SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_ESCAPE: j->enCours = false; break;
        case SDLK_RIGHT: j->attaquant = (j->attaquant + 1) % j->n; break;
        case SDLK_LEFT:  j->attaquant = (j->attaquant + j->n - 1) % j->n; break;
        case SDLK_DOWN:  j->cible = (j->cible + 1) % j->n; break;
        case SDLK_UP:    j->cible = (j->cible + j->n - 1) % j->n; break;
        case SDLK_SPACE: actionAttaquer(j); break;
        case SDLK_n: executerBouton(j, 0); break;
        case SDLK_r: executerBouton(j, 1); break;
        case SDLK_s: executerBouton(j, 2); break;
        case SDLK_l: executerBouton(j, 3); break;
        default: break;
        }
    } else if (e->type == SDL_MOUSEBUTTONDOWN) {
        int x = e->button.x, y = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) {
            for (int i = 0; i < NB_BOUTONS; i++)
                if (dans(rectBouton(i), x, y)) { executerBouton(j, i); return; }
        }
        /* clic sur une ligne : gauche = attaquant, droit = cible */
        for (int i = 0; i < j->n; i++) {
            if (dans(rectLigne(i), x, y)) {
                if (e->button.button == SDL_BUTTON_LEFT) j->attaquant = i;
                else if (e->button.button == SDL_BUTTON_RIGHT) j->cible = i;
            }
        }
    }
}

/* ---------- Dessin ---------- */

static void dessinerFenetreCreation(SDL_Renderer *r, TTF_Font *fPetit, TTF_Font *fMoyen,
                                    const Jeu *j, int mx, int my) {
    SDL_Color fondPanneau = {45, 45, 65, 255}, sombre = {60, 60, 80, 255};
    SDL_Rect panneau = {200, 150, 500, 300};
    SDL_SetRenderDrawColor(r, fondPanneau.r, fondPanneau.g, fondPanneau.b, 255);
    SDL_RenderFillRect(r, &panneau);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &panneau);

    texte(r, fMoyen, "NOUVEAU PERSONNAGE", 230, 168, OR);

    /* Champ de saisie du nom */
    texte(r, fMoyen, "Nom :", 230, 222, BLANC);
    SDL_Rect champ = {300, 215, 360, 32};
    SDL_SetRenderDrawColor(r, 20, 20, 30, 255);
    SDL_RenderFillRect(r, &champ);
    SDL_SetRenderDrawColor(r, 170, 170, 180, 255);
    SDL_RenderDrawRect(r, &champ);
    char saisie[TAILLE_NOM + 4];
    snprintf(saisie, sizeof saisie, "%s%s", j->nomSaisi, (SDL_GetTicks() / 500) % 2 ? "_" : "");
    texte(r, fMoyen, saisie, 308, 220, BLANC);

    /* Choix de la classe */
    texte(r, fMoyen, "Classe :", 230, 277, BLANC);
    for (int c = 0; c < 3; c++) {
        SDL_Rect cadre = rectClasse(c);
        SDL_Color fond = (c == (int)j->classeChoisie) ? couleurClasse((enum Classe)c) : sombre;
        SDL_Color contour = (c == (int)j->classeChoisie) ? BLANC : GRIS;
        dessinerBouton(r, fPetit, cadre, nomClasse((enum Classe)c), dans(cadre, mx, my), fond, contour);
    }

    /* Apercu des statistiques */
    Personnage apercu;
    initPersonnage(&apercu, j->nomSaisi, j->classeChoisie);
    char stats[100];
    snprintf(stats, sizeof stats, "%s (%s) : PV %d   ATK %d   DEF %d", nomClasse(apercu.classe),
             nomCouleur(apercu.classe), apercu.pv, apercu.atk, apercu.def);
    texte(r, fPetit, stats, 300, 318, GRIS);

    SDL_Color vert = {40, 120, 60, 255}, rouge = {140, 50, 50, 255};
    dessinerBouton(r, fMoyen, rectValider(), "Valider", dans(rectValider(), mx, my), vert, BLANC);
    dessinerBouton(r, fMoyen, rectAnnuler(), "Annuler", dans(rectAnnuler(), mx, my), rouge, BLANC);

    texte(r, fPetit, "Tape le nom, choisis la classe (clic ou Gauche/Droite)", 230, 408, GRIS);
    texte(r, fPetit, "Entree = valider    Echap = annuler", 230, 426, GRIS);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* --- Donnees : on charge la sauvegarde, sinon nouvelle partie --- */
    Jeu jeu;
    memset(&jeu, 0, sizeof jeu);
    jeu.enCours = true;
    FILE *test = fopen(FICHIER, "rb");
    if (test) { fclose(test); charger(FICHIER, &jeu.liste, &jeu.n); }
    if (jeu.n > MAX_PERSOS) jeu.n = MAX_PERSOS;
    if (jeu.n == 0) {
        if (!nouvellePartie(&jeu)) return 1;
    } else {
        jeu.cible = (jeu.n > 1) ? 1 : 0;
    }

    /* --- Initialisation SDL + SDL_ttf --- */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init : %s\n", SDL_GetError());
        free(jeu.liste);
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init : %s\n", TTF_GetError());
        SDL_Quit(); free(jeu.liste); return 1;
    }
    char cheminPolice[512];
    if (!trouverPolice(cheminPolice, sizeof cheminPolice)) {
        fprintf(stderr, "Aucune police trouvee. Installe-en une : sudo pacman -S ttf-dejavu\n");
        TTF_Quit(); SDL_Quit(); free(jeu.liste); return 1;
    }
    TTF_Font *fPetit = TTF_OpenFont(cheminPolice, 15);
    TTF_Font *fMoyen = TTF_OpenFont(cheminPolice, 18);
    TTF_Font *fGrand = TTF_OpenFont(cheminPolice, 28);
    if (!fPetit || !fMoyen || !fGrand) {
        fprintf(stderr, "Impossible d'ouvrir la police %s : %s\n", cheminPolice, TTF_GetError());
        if (fPetit) TTF_CloseFont(fPetit);
        if (fMoyen) TTF_CloseFont(fMoyen);
        if (fGrand) TTF_CloseFont(fGrand);
        TTF_Quit(); SDL_Quit(); free(jeu.liste); return 1;
    }

    SDL_Window *fenetre = SDL_CreateWindow("Le Donjon",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 640, SDL_WINDOW_SHOWN);
    if (!fenetre) {
        fprintf(stderr, "SDL_CreateWindow : %s\n", SDL_GetError());
        TTF_CloseFont(fPetit); TTF_CloseFont(fMoyen); TTF_CloseFont(fGrand);
        TTF_Quit(); SDL_Quit(); free(jeu.liste); return 1;
    }
    SDL_Renderer *rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_ACCELERATED);
    if (!rendu) rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_SOFTWARE);
    if (!rendu) {
        fprintf(stderr, "SDL_CreateRenderer : %s\n", SDL_GetError());
        SDL_DestroyWindow(fenetre);
        TTF_CloseFont(fPetit); TTF_CloseFont(fMoyen); TTF_CloseFont(fGrand);
        TTF_Quit(); SDL_Quit(); free(jeu.liste); return 1;
    }

    ajouterLog(&jeu, "Bienvenue dans le donjon !");
    SDL_Event e;

    while (jeu.enCours) {
        /* --- Evenements (non bloquant) --- */
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) jeu.enCours = false;
            else if (jeu.creation) evenementCreation(&jeu, &e);
            else evenementJeu(&jeu, &e);
        }
        if (!jeu.enCours) break;

        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        /* --- Dessin --- */
        SDL_SetRenderDrawColor(rendu, 30, 30, 40, 255);
        SDL_RenderClear(rendu);

        texte(rendu, fGrand, "LE DONJON", 30, 4, BLANC);
        if (jeu.n == 1) {
            char v[80];
            snprintf(v, sizeof v, "VICTOIRE DE %s !", jeu.liste[0].nom);
            texte(rendu, fGrand, v, 300, 4, OR);
        }

        for (int i = 0; i < jeu.n; i++) {
            int y = 50 + i * 40;
            char ligne[100];

            /* Pastille de couleur selon la classe */
            SDL_Color cc = couleurClasse(jeu.liste[i].classe);
            SDL_Rect icone = { 30, y, 30, 30 };
            SDL_SetRenderDrawColor(rendu, cc.r, cc.g, cc.b, 255);
            SDL_RenderFillRect(rendu, &icone);

            /* Nom, classe et couleur */
            snprintf(ligne, sizeof ligne, "%s - %s (%s)", jeu.liste[i].nom,
                     nomClasse(jeu.liste[i].classe), nomCouleur(jeu.liste[i].classe));
            texte(rendu, fMoyen, ligne, 75, y - 6, BLANC);

            /* Fond de la barre (gris) */
            SDL_Rect fond = { 75, y + 20, LARGEUR_BARRE, 12 };
            SDL_SetRenderDrawColor(rendu, 80, 80, 80, 255);
            SDL_RenderFillRect(rendu, &fond);

            /* Barre de vie : largeur proportionnelle aux PV reels */
            int largeur = LARGEUR_BARRE * jeu.liste[i].pv / pvMaxClasse(jeu.liste[i].classe);
            if (largeur > LARGEUR_BARRE) largeur = LARGEUR_BARRE;
            SDL_Rect barre = { 75, y + 20, largeur, 12 };
            SDL_SetRenderDrawColor(rendu, 231, 76, 60, 255);
            SDL_RenderFillRect(rendu, &barre);

            snprintf(ligne, sizeof ligne, "PV %d/%d   ATK %d   DEF %d", jeu.liste[i].pv,
                     pvMaxClasse(jeu.liste[i].classe), jeu.liste[i].atk, jeu.liste[i].def);
            texte(rendu, fPetit, ligne, 285, y + 15, GRIS);

            /* Contour jaune = attaquant, blanc = cible */
            if (i == jeu.attaquant) {
                SDL_Rect c = rectLigne(i);
                SDL_SetRenderDrawColor(rendu, 255, 215, 0, 255);
                SDL_RenderDrawRect(rendu, &c);
                texte(rendu, fPetit, "ATTAQUANT", 510, y + 8, OR);
            }
            if (i == jeu.cible && jeu.n > 1) {
                SDL_Rect c = rectLigne(i);
                c.x -= 2; c.y -= 2; c.w += 4; c.h += 4;
                SDL_SetRenderDrawColor(rendu, 255, 255, 255, 255);
                SDL_RenderDrawRect(rendu, &c);
                texte(rendu, fPetit, "CIBLE", 600, y + 8, BLANC);
            }
        }

        /* Boutons du menu (Rejouer en dore quand la partie est finie) */
        SDL_Color fondBtn = {60, 60, 80, 255};
        for (int i = 0; i < NB_BOUTONS; i++) {
            SDL_Rect b = rectBouton(i);
            SDL_Color contour = (i == 1 && jeu.n == 1) ? OR : GRIS;
            dessinerBouton(rendu, fMoyen, b, LIBELLES[i], !jeu.creation && dans(b, mx, my), fondBtn, contour);
        }

        /* Journal des evenements */
        texte(rendu, fPetit, "Journal :", 30, 462, GRIS);
        for (int k = 0; k < NB_LOGS; k++)
            texte(rendu, fPetit, jeu.journal[k], 30, 482 + k * 20, BLANC);

        /* Mode d'emploi */
        texte(rendu, fPetit, "Gauche/Droite : attaquant   Haut/Bas : cible   ESPACE : attaquer   (ou clic gauche / clic droit sur un perso)", 30, 590, GRIS);
        texte(rendu, fPetit, "N : nouveau perso   R : rejouer   S : sauvegarder   L : charger   ECHAP : quitter", 30, 610, GRIS);

        if (jeu.creation) dessinerFenetreCreation(rendu, fPetit, fMoyen, &jeu, mx, my);

        SDL_RenderPresent(rendu);
        SDL_Delay(16);   /* ~60 images/s, evite de saturer le CPU */
    }

    /* --- Nettoyage --- */
    free(jeu.liste);
    TTF_CloseFont(fPetit);
    TTF_CloseFont(fMoyen);
    TTF_CloseFont(fGrand);
    SDL_DestroyRenderer(rendu);
    SDL_DestroyWindow(fenetre);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
