#include "Characters.h"
#include <cstring>
#include <cstdlib>

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

ImageId staticSprite( const ResolvedChar* rc, Dir d )
{
    ImageId i = pickSprite( rc, d, MOV_BASE, 0 );
    return i != kInvalidImageId ? i : pickSprite( rc, d, MOV_MARCHE, 0 );
}

// ---------------------------------------------------------------- joueur
void Player::init( const ResolvedChar* r ) { rc = r; a = Animator(); vie = kPlayerLife; blesser = false; attackActive = false;
                                              swordRect = Rect{ kLogicCx - 20, kLogicCy - 22, 32, 22 }; }

void Player::readKeys( const InputState& in, GameMap& map, const std::vector<Rect>& blockers )
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
        bool blocked = map.collidesScreen( hitbox(), map.camX + k.dx, map.camY + k.dy );
        if ( !blocked ) {
            Rect wr{ hitbox().x - ( map.camX + k.dx ), hitbox().y - ( map.camY + k.dy ), kPlayerHitW, kPlayerHitH };
            for ( const Rect& b : blockers ) if ( rectsOverlap( wr, b ) ) { blocked = true; break; }
        }
        if ( !blocked ) {
            map.camX += k.dx;
            map.camY += k.dy;
        }
        return;
    }
    a.mov = MOV_BASE;       // aucune touche
    a.libre = true;
}

static Rect screenRect( const Monster& m, const GameMap& map ) { return Rect{ m.wx + map.camX, m.wy + map.camY, m.size, m.size }; }

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
    // Degats = attaque / 2 (au moins 1) : attaque 2 = 1 point comme Ethynd, attaque 4 (dragon) = 2 points.
    bool touche = false, epeeTouche = false;
    int dmg = 1;
    for ( const Monster& m : monsters ) {
        if ( !m.alive() || m.attaque <= 0 ) continue;
        Rect r = screenRect( m, map );
        if ( rectsOverlap( hitbox(), r ) ) { touche = true; if ( m.attaque / 2 > dmg ) dmg = m.attaque / 2; }
        // Immunite pendant l'attaque : l'epee qui touche un monstre annule ses degats (Ethynd). Differences avec l'original :
        // seulement PENDANT l'attaque (l'original gardait le dernier rectangle d'epee et protegeait de facon incoherente apres),
        // et jamais contre un boss (sinon marteler A a son contact le rendrait inoffensif).
        if ( attackActive && !m.boss() && rectsOverlap( swordRect, r ) ) epeeTouche = true;
    }
    if ( touche && !blesser ) {
        if ( !epeeTouche ) {
            audio.playSfx( Sfx::Hurt );
            blesser = true;
            hurtTimer = 0;
            vie -= dmg;
        }
    } else if ( touche && blesser ) {                              // contact prolonge : coup repete (nouveau : l'original n'en donnait qu'un)
        if ( ++hurtTimer >= kHurtRepeat && !epeeTouche ) {
            audio.playSfx( Sfx::Hurt );
            hurtTimer = 0;
            vie -= dmg;
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
static long long sq( long long v ) { return v * v; }

void monsterUpdate( Monster& m, const GameMap& map, const std::vector<Rect>& blockers, const Rect& player )
{
    // Entite.deplacement() : la liste utilisee est toujours "base" (haut, gauche, bas, droite)
    static const int dx[4] = { 0, -kEntitySpeed, 0,  kEntitySpeed };
    static const int dy[4] = { -kEntitySpeed, 0, kEntitySpeed, 0 };
    static const Dir dirs[4] = { DIR_HAUT, DIR_GAUCHE, DIR_BAS, DIR_DROITE };
    if ( m.compteurAction >= 4 ) m.compteurAction = 0;
    m.a.mov = MOV_MARCHE;

    const int half = m.size / 2;
    const int cx = m.wx + half, cy = m.wy + half;                      // centre du monstre
    const int hx = m.homeX + half, hy = m.homeY + half;                // centre de son point d'attache
    long long dHome = sq( cx - hx ) + sq( cy - hy );
    int first = m.compteurAction, second = -1;
    bool chasing = false;
    if ( m.boss() ) {
        // Gardien de zone : poursuit le joueur s'il est dans sa zone (rayon + 48 px) et assez proche, sinon rentre
        // au point d'attache s'il en est sorti, sinon rode au hasard. Il ne quitte jamais sa zone.
        const int px = player.x + player.w / 2, py = player.y + player.h / 2;
        const bool aggro = sq( px - hx ) + sq( py - hy ) <= sq( m.radius + 48 ) && sq( px - cx ) + sq( py - cy ) <= sq( 240 );
        int tx = 0, ty = 0;
        if ( aggro ) { tx = px - cx; ty = py - cy; chasing = true; }
        else if ( dHome > sq( m.radius ) ) { tx = hx - cx; ty = hy - cy; chasing = true; }
        if ( chasing ) {
            bool horiz = std::abs( tx ) > std::abs( ty );
            first  = horiz ? ( tx > 0 ? 3 : 1 ) : ( ty > 0 ? 2 : 0 );
            second = horiz ? ( ty > 0 ? 2 : 0 ) : ( tx > 0 ? 3 : 1 );
        }
    }
    m.a.dir = dirs[first];

    // Simplification : test en coordonnees monde (Ethynd testait a la position de l'image
    // precedente, decalee de la vitesse de la camera : au plus 3 px d'ecart).
    auto tryMove = [&]( int idx ) {
        int nx = m.wx + dx[idx], ny = m.wy + dy[idx];
        Rect target{ nx, ny, m.size, m.size };
        if ( map.collidesWorld( target ) ) return false;
        for ( const Rect& b : blockers ) if ( rectsOverlap( target, b ) ) return false;      // les PNJ sont des obstacles
        if ( m.boss() ) {                                              // jamais plus loin de son point d'attache
            long long dn = sq( nx + half - hx ) + sq( ny + half - hy );
            if ( dn > sq( m.radius ) && dn > dHome ) return false;
        }
        m.wx = nx; m.wy = ny; m.a.dir = dirs[idx];
        return true;
    };
    if ( !tryMove( first ) && second >= 0 ) tryMove( second );          // poursuite : contourne l'obstacle par l'autre axe

    // afficher() : animation puis choix du sprite ; direction aleatoire a chaque fin de cycle
    // (un monstre en poursuite garde son cap)
    if ( stepAnimator( m.a, m.rc->def ) && !chasing )
        m.compteurAction = (int)( gameRandom() & 3 );
    m.sprite = pickSprite( m.rc, m.a.dir, m.a.mov, m.a.frame );
}

void monsterDraw( const Monster& m, const GameMap& map, IRenderer& r )
{
    if ( !m.alive() || m.sprite == kInvalidImageId ) return;
    r.drawImage( map.originX() + floorDiv( m.wx * kTileShow, kTileLogic ),
                 map.originY() + floorDiv( m.wy * kTileShow, kTileLogic ), m.sprite );
}
