// Characters.h -- joueur et monstres (port de classes/joueur.py, entite.py, monstre.py).
#pragma once
#include <vector>
#include "AssetStore.h"
#include "GameMap.h"
#include "IInput.h"
#include "IAudio.h"
#include "EthyndData.h"
#include "World.h"

// Definition d'un personnage dont les noms de sprites sont resolus en ImageId.
struct ResolvedChar {
    const CharDef* def = nullptr;
    ImageId img[4][3][kMaxFrames];
    void resolve( const CharDef* d, const AssetStore& a );
};

// Etat d'animation commun (compteur / frame / direction / mouvement), comme dans Ethynd.
struct Animator {
    int  compteur = 0, frame = 0;
    Dir  dir = DIR_BAS;
    Mov  mov = MOV_BASE;
    bool libre = true;
};

struct Monster {
    const ResolvedChar* rc = nullptr;
    Animator a;
    int wx = 0, wy = 0;              // position MONDE logique (coin haut-gauche, 32x32)
    int vie = 0, attaque = 0;
    int typeIdx = -1;                // index dans kEntityNames (compteur de victimes)
    int compteurAction = 0;          // direction choisie : 0 haut, 1 gauche, 2 bas, 3 droite
    ImageId sprite = kInvalidImageId;
    bool alive() const { return vie > 0; }
};

// PNJ : personnage immobile (variante recoloree du joueur, ou un animal) qui parle quand on appuie sur A.
struct Npc {
    const ResolvedChar* rc = nullptr;
    int wx = 0, wy = 0;              // coin haut-gauche monde, 32x32
    Dir dir = DIR_BAS;
    ImageId sprite = kInvalidImageId;
    std::vector<TalkRule> rules;
    Rect rect() const { return Rect{ wx, wy, 32, 32 }; }
};
// objet ramassable pose sur la carte
struct MapItem { int def = 0; int wx = 0, wy = 0; bool taken = false; Rect rect() const { return Rect{ wx + 4, wy + 4, 24, 24 }; } };

ImageId staticSprite( const ResolvedChar* rc, Dir d );     // image fixe (arret, sinon 1re image de marche)

class Player {
  public:
    void init( const ResolvedChar* rc );
    int  vie = kPlayerLife;

    // blockers : rectangles MONDE infranchissables en plus des tuiles (PNJ)
    void readKeys( const InputState& in, GameMap& map, const std::vector<Rect>& blockers );
    void idle() { a.mov = MOV_BASE; a.libre = true; }       // arret (dialogue)
    void face( Dir d ) { a.dir = d; }
    Dir  dir() const { return a.dir; }
    void heal( int n ) { vie += n; if ( vie > kPlayerLife ) vie = kPlayerLife; }
    // actualiser() de l'original : degats, animation, epee, sons
    void update( const GameMap& map, std::vector<Monster>& monsters, IAudio& audio );
    void draw( IRenderer& r ) const;
    // hitbox de l'epee a l'ecran logique (valide seulement pendant une attaque, sinon vide)
    bool swordActive() const { return attackActive; }
    const Rect& sword() const { return swordRect; }
    static Rect hitbox() { return Rect{ kLogicCx - 12, kPlayerHitCy - 10, kPlayerHitW, kPlayerHitH }; }

  private:
    const ResolvedChar* rc = nullptr;
    Animator a;
    bool blesser = false;
    bool attackActive = false;
    Rect swordRect { kLogicCx - 20, kLogicCy - 22, 32, 22 };   // rect "obsolete" initial d'Ethynd
    ImageId sprite = kInvalidImageId;
    void stepFrame();
    void updateSword();
};

// Generateur pseudo-aleatoire simple (xorshift), identique sur toutes les plateformes.
uint32_t gameRandom();
void gameRandomSeed( uint32_t s );

// Port de Entite.deplacement() + afficher() pour un monstre.
void monsterUpdate( Monster& m, const GameMap& map );
void monsterDraw( const Monster& m, const GameMap& map, IRenderer& r );
