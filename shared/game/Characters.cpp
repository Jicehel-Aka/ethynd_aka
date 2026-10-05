#include "Characters.h"
#include <cstring>

// ---------------------------------------------------------------- aleatoire
static uint32_t rngState = 2463534242u;
void gameRandomSeed( uint32_t s ) { rngState = s ? s : 2463534242u; }
uint32_t gameRandom() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return rngState; }

// ---------------------------------------------------------------- definitions
void ResolvedChar::resolve( const CharDef* d, const AssetStore& a )
{
    def = d;
    for ( int dir = 0; dir < 4; dir++ )
        for ( int mv = 0; mv < 3; mv++ )
            for ( int f = 0; f < kMaxFrames; f++ ) {
                const SpriteAnim& sa = d->anim[dir][mv];
                img[dir][mv][f] = ( f < sa.count ) ? a.sprite( sa.names[f] ) : kInvalidImageId;
            }
}

static ImageId pickSprite( const ResolvedChar* rc, Dir d, Mov m, int frame )
{
    const SpriteAnim& sa = rc->def->anim[d][m];
    if ( sa.count == 0 ) return kInvalidImageId;
    if ( frame >= sa.count ) frame = 0;
    return rc->img[d][m][frame];
}

// Avance compteur/frame selon les timings (actualiser_frame d'Ethynd). Renvoie true quand
// une animation se termine (retour a la frame 0).
static bool stepAnimator( Animator& a, const CharDef* def )
{
    const AnimTiming& t = def->timing[a.mov];
    if ( t.tick < 0 ) { a.compteur = 0; a.frame = 0; return false; }
    if ( a.compteur < t.tick ) { a.compteur++; return false; }
    a.compteur = 0;
    if ( a.frame < t.last ) { a.frame++; return false; }
    a.frame = 0;
    a.libre = t.libre;
    if ( t.reset ) a.mov = MOV_BASE;
    return true;
}

// ---------------------------------------------------------------- joueur
void Player::init( const ResolvedChar* r ) { rc = r; a = Animator(); vie = kPlayerLife; blesser = false; attackActive = false;
                                              swordRect = Rect{ kLogicCx - 20, kLogicCy - 22, 32, 22 }; }

void Player::readKeys( const InputState& in, GameMap& map )
{
    if ( !a.libre ) return;
    struct Key { bool pressed; int dx, dy; int dir; Mov mov; bool libre; };
    // Ordre de priorite d'Ethynd : haut, gauche, bas, droite, puis attaque (touche X).
    // (dx, dy) est le deplacement de la CAMERA, donc l'inverse du deplacement du joueur.
    const Key keys[5] = {
        { in.up,     0,  kPlayerSpeed, DIR_HAUT,   MOV_MARCHE,  true  },
        { in.left,   kPlayerSpeed, 0,  DIR_GAUCHE, MOV_MARCHE,  true  },
        { in.down,   0, -kPlayerSpeed, DIR_BAS,    MOV_MARCHE,  true  },
        { in.right, -kPlayerSpeed, 0,  DIR_DROITE, MOV_MARCHE,  true  },
        { in.actionA, 0, 0,            -1,         MOV_ATTAQUE, false },
    };
    for ( const Key& k : keys ) {
        if ( !k.pressed ) continue;
        if ( k.dir >= 0 ) a.dir = (Dir)k.dir;
        if ( a.mov != k.mov ) { a.mov = k.mov; a.compteur = 0; a.frame = 0; }
        a.libre = k.libre;
        if ( !map.collidesScreen( hitbox(), map.camX + k.dx, map.camY + k.dy ) ) {
            map.camX += k.dx;
            map.camY += k.dy;
        }
        return;
    }
    a.mov = MOV_BASE;       // aucune touche
    a.libre = true;
}

static Rect screenRect( const Monster& m, const GameMap& map ) { return Rect{ m.wx + map.camX, m.wy + map.camY, 32, 32 }; }

void Player::updateSword()
{
    if ( a.mov != MOV_ATTAQUE ) { attackActive = false; return; }
    int x, y, L, H;
    switch ( a.dir ) {
        case DIR_BAS:    x = kLogicCx + 10; y = kLogicCy + 30; L = 32; H = 22; break;
        case DIR_GAUCHE: x = kLogicCx - 10; y = kLogicCy + 20; L = 25; H = 32; break;
        case DIR_DROITE: x = kLogicCx + 28; y = kLogicCy + 20; L = 22; H = 32; break;
        default:         x = kLogicCx + 20; y = kLogicCy;      L = 32; H = 22; break;  // haut
    }
    // pg.Rect.center = (x - L/2, y - H/2) : centre tronque, puis coin = centre - taille // 2
    int ccx = ( 2 * x - L ) / 2, ccy = ( 2 * y - H ) / 2;
    swordRect = Rect{ ccx - L / 2, ccy - H / 2, L, H };
    attackActive = true;
}

void Player::update( const GameMap& map, std::vector<Monster>& monsters, IAudio& audio )
{
    // enlever_vie() : seuls les monstres dangereux (attaque > 0) blessent, voir Game.h
    bool touche = false, epeeTouche = false;
    for ( const Monster& m : monsters ) {
        if ( !m.alive() || m.attaque <= 0 ) continue;
        Rect r = screenRect( m, map );
        if ( rectsOverlap( hitbox(), r ) ) touche = true;
        if ( rectsOverlap( swordRect, r ) ) epeeTouche = true;     // rect de l'epee conserve apres l'attaque
    }
    if ( touche && !blesser ) {
        if ( !epeeTouche ) {
            audio.playSfx( Sfx::Hurt );
            blesser = true;
            vie -= kEnemyDamage;
        }
    } else if ( !touche && blesser ) {
        blesser = false;
    }

    stepAnimator( a, rc->def );
    sprite = pickSprite( rc, a.dir, a.mov, a.frame );
    updateSword();

    // actualiser_son()
    if ( a.mov == MOV_ATTAQUE ) {
        if ( a.compteur == 0 && a.frame == 1 ) audio.playSfx( Sfx::Attack );
    } else if ( !audio.playerSfxBusy() ) {
        if ( a.mov == MOV_MARCHE ) audio.playSfx( Sfx::Walk );
    }
}

void Player::draw( IRenderer& r ) const
{
    if ( sprite == kInvalidImageId ) return;
    int16_t w, h;
    r.getImageSize( sprite, w, h );
    r.drawImage( kScreenW / 2 - w / 2, kScreenH / 2 - h / 2, sprite );
}

// ---------------------------------------------------------------- monstres
void monsterUpdate( Monster& m, const GameMap& map )
{
    // Entite.deplacement() : la liste utilisee est toujours "base" (haut, gauche, bas, droite)
    static const int dx[4] = { 0, -kEntitySpeed, 0,  kEntitySpeed };
    static const int dy[4] = { -kEntitySpeed, 0, kEntitySpeed, 0 };
    static const Dir dirs[4] = { DIR_HAUT, DIR_GAUCHE, DIR_BAS, DIR_DROITE };
    if ( m.compteurAction >= 4 ) m.compteurAction = 0;
    m.a.mov = MOV_MARCHE;
    m.a.dir = dirs[m.compteurAction];
    int nx = m.wx + dx[m.compteurAction], ny = m.wy + dy[m.compteurAction];
    // Simplification : test en coordonnees monde (Ethynd testait a la position de l'image
    // precedente, decalee de la vitesse de la camera : au plus 3 px d'ecart).
    if ( !map.collidesWorld( Rect{ nx, ny, 32, 32 } ) ) { m.wx = nx; m.wy = ny; }

    // afficher() : animation puis choix du sprite ; direction aleatoire a chaque fin de cycle
    if ( stepAnimator( m.a, m.rc->def ) )
        m.compteurAction = (int)( gameRandom() & 3 );
    m.sprite = pickSprite( m.rc, m.a.dir, m.a.mov, m.a.frame );
}

void monsterDraw( const Monster& m, const GameMap& map, IRenderer& r )
{
    if ( !m.alive() || m.sprite == kInvalidImageId ) return;
    r.drawImage( map.originX() + floorDiv( m.wx * kTileShow, kTileLogic ),
                 map.originY() + floorDiv( m.wy * kTileShow, kTileLogic ), m.sprite );
}
