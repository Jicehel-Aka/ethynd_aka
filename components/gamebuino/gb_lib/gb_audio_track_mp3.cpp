/*
Piste audio MP3 pour la bibliotheque Gamebuino-AKA (voir gb_audio_track_mp3.h).
*/
#include "gb_audio_track_mp3.h"
#include "gb_err.h"
#include "esp_log.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "minimp3.h"

static const char* TAG = "gb_mp3";

#define RING_MASK       (GB_MP3_RING_SAMPLES - 1)
#define MP3_DEC_DELAY   529     // retard fixe du decodeur (convention LAME/ffmpeg)

static_assert( (GB_MP3_RING_SAMPLES & (GB_MP3_RING_SAMPLES - 1)) == 0,
               "GB_MP3_RING_SAMPLES must be a power of 2" );

static inline uint32_t be32( const uint8_t* p )
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

//! informations d'un tag Xing/Info (+ extension LAME/Lavc)
typedef struct {
    bool     found;         // la premiere trame est une trame Xing/Info (a ne pas jouer)
    bool     has_delay;     // delai/bourrage de l'encodeur connus
    uint32_t frames;        // nombre de trames audio (hors trame Info), 0 si inconnu
    uint32_t delay;         // echantillons ajoutes au debut par l'encodeur
    uint32_t padding;       // echantillons ajoutes a la fin par l'encodeur
} mp3_info_tag_t;

static void parse_info_tag( const uint8_t* fr, int len, mp3_info_tag_t* t )
{
    memset( t, 0, sizeof(*t) );
    int p = -1;
    for ( int i = 4; i + 8 <= len && i <= 4 + 36 + 2; i++ )
        if ( !memcmp( fr + i, "Xing", 4 ) || !memcmp( fr + i, "Info", 4 ) ) { p = i; break; }
    if ( p < 0 )
        return;
    t->found = true;
    uint32_t flags = be32( fr + p + 4 );
    int q = p + 8;
    if ( flags & 1 ) { if ( q + 4 > len ) return; t->frames = be32( fr + q ); q += 4; }
    if ( flags & 2 ) q += 4;     // taille en octets
    if ( flags & 4 ) q += 100;   // table TOC
    if ( flags & 8 ) q += 4;     // qualite
        // extension : chaine encodeur sur 9 octets ("LAME3.100", "Lavc60.31"...),
        // puis delai (12 bits) et bourrage (12 bits) a l'octet 21
    if ( q + 24 <= len && isalpha(fr[q]) && isalpha(fr[q+1]) && isalpha(fr[q+2]) && isalpha(fr[q+3]) )
    {
        uint32_t d = ((uint32_t)fr[q+21] << 4) | (fr[q+22] >> 4);
        uint32_t pd = ((uint32_t)(fr[q+22] & 0x0F) << 8) | fr[q+23];
        if ( d <= 2304 && pd <= 4608 )
        {
            t->has_delay = true;
            t->delay = d;
            t->padding = pd;
        }
    }
}

//=============================================================================
//  API
//=============================================================================
int gb_audio_track_mp3::play_mp3( const char* pc_file_name, bool loop )
{
    if ( !pc_file_name || !*pc_file_name )
        return GB_ERR_PARAM;
    if ( strncmp( pc_file_name, MOUNT_POINT, strlen(MOUNT_POINT) ) != 0 || strstr( pc_file_name, ".." ) )
        return GB_ERR_PARAM;

    stop_playing();     // permet d'interrompre un morceau pour en lancer un autre

    FILE* f = fopen( pc_file_name, "rb" );
    if ( !f )
    {
        ESP_LOGE( TAG, "cannot open %s (errno %d)", pc_file_name, errno );
        return (errno == ENOENT) ? GB_ERR_NOT_FOUND : GB_ERR_IO;
    }

        // saute le tag ID3v2 eventuel
    uint8_t hdr[10];
    uint32_t start = 0;
    if ( fread( hdr, 1, sizeof(hdr), f ) == sizeof(hdr) && !memcmp( hdr, "ID3", 3 ) )
    {
        uint32_t size = ((uint32_t)(hdr[6] & 0x7F) << 21) | ((uint32_t)(hdr[7] & 0x7F) << 14)
                      | ((uint32_t)(hdr[8] & 0x7F) << 7)  |  (uint32_t)(hdr[9] & 0x7F);
        start = 10 + size + ((hdr[5] & 0x10) ? 10 : 0);
    }
    if ( fseek( f, start, SEEK_SET ) != 0 )
    {
        fclose( f );
        return GB_ERR_IO;
    }
        // verifie qu'une synchro de trame existe dans les premiers Ko
    bool sync = false;
    int prev = fgetc( f );
    for ( int i = 0; i < 8192 && prev != EOF; i++ )
    {
        int c = fgetc( f );
        if ( c == EOF ) break;
        if ( prev == 0xFF && (c & 0xE0) == 0xE0 && ((c >> 1) & 3) != 0 && ((c >> 3) & 3) != 1 )
        {
            sync = true;
            break;
        }
        prev = c;
    }
    if ( !sync )
    {
        ESP_LOGE( TAG, "%s : no MP3 frame found", pc_file_name );
        fclose( f );
        return GB_ERR_FORMAT;
    }
    fseek( f, start, SEEK_SET );

    _ring = (int16_t*)malloc( GB_MP3_RING_SAMPLES * sizeof(int16_t) );
    _task_done = xSemaphoreCreateBinary();
    if ( !_ring || !_task_done )
    {
        if ( _ring ) { free( _ring ); _ring = 0; }
        if ( _task_done ) { vSemaphoreDelete( _task_done ); _task_done = 0; }
        fclose( f );
        return GB_ERR_NO_SPACE;
    }

    _f = f;
    _data_start = start;
    _audio_start = start;
    _wr = 0;
    _rd = 0;
    _played = 0;
    _length_ms = 0;
    _done = false;
    _stop_req = false;
    _loop = loop;
    _rs_step = 65536;
    _rs_phase = 0;
    _rs_prev = 0;
    _playing = true;

    if ( xTaskCreate( decode_task_entry, "gb_mp3", GB_MP3_TASK_STACK, this, GB_MP3_TASK_PRIO, NULL ) != pdPASS )
    {
        ESP_LOGE( TAG, "cannot start decoder task" );
        _playing = false;
        fclose( _f ); _f = 0;
        free( _ring ); _ring = 0;
        vSemaphoreDelete( _task_done ); _task_done = 0;
        return GB_ERR_NO_SPACE;
    }
    return GB_OK;
}

void gb_audio_track_mp3::stop_playing()
{
    if ( _task_done )
    {
        _stop_req = true;
            // la tache surveille _stop_req a chaque trame et libere ses ressources
        xSemaphoreTake( _task_done, portMAX_DELAY );
        vSemaphoreDelete( _task_done );
        _task_done = 0;
    }
    if ( _ring )
    {
        free( _ring );
        _ring = 0;
    }
    _playing = false;
}

uint32_t gb_audio_track_mp3::position()
{
    return (uint32_t)( (uint64_t)_played.load() * 1000 / GB_AUDIO_SAMPLE_RATE );
}

int gb_audio_track_mp3::play_callback( int16_t* pi16_buffer, uint16_t u16_sample_count )
{
    if ( !_playing )
        return -1;

        // _done est lu AVANT _wr : si _done est vrai, tout ce que le decodeur
        // a produit est deja visible dans _wr (pas de fin de morceau tronquee)
    bool done = _done.load( std::memory_order_acquire );
    uint32_t rd = _rd.load( std::memory_order_relaxed );
    uint32_t wr = _wr.load( std::memory_order_acquire );
    uint32_t avail = wr - rd;
    uint32_t n = (avail < u16_sample_count) ? avail : u16_sample_count;

    for ( uint32_t i = 0; i < n; i++ )
        pi16_buffer[i] = _ring[(rd + i) & RING_MASK];
    _rd.store( rd + n, std::memory_order_release );
    for ( uint32_t i = n; i < u16_sample_count; i++ )
        pi16_buffer[i] = 0;         // sous-alimentation ou fin de morceau
    _played += n;

    if ( done && n < u16_sample_count )
    {
        stop_playing();             // fin de morceau : tampon vide et decodeur termine
        if ( n == 0 )
            return -1;
    }
    return 0;
}

//=============================================================================
//  Tampon circulaire + reechantillonnage (cote tache)
//=============================================================================
bool gb_audio_track_mp3::push_samples( const int16_t* s, uint32_t n )
{
    while ( n )
    {
        if ( _stop_req )
            return false;
        uint32_t wr = _wr.load( std::memory_order_relaxed );
        uint32_t rd = _rd.load( std::memory_order_acquire );
        uint32_t room = GB_MP3_RING_SAMPLES - (wr - rd);
        if ( room == 0 )
        {
            vTaskDelay( 1 );        // plein : on laisse le mixeur consommer
            continue;
        }
        uint32_t k = (room < n) ? room : n;
        for ( uint32_t i = 0; i < k; i++ )
            _ring[(wr + i) & RING_MASK] = s[i];
        _wr.store( wr + k, std::memory_order_release );
        s += k;
        n -= k;
    }
    return true;
}

bool gb_audio_track_mp3::emit_resampled( const int16_t* mono, uint32_t n )
{
    if ( _rs_step == 65536 )        // meme frequence que le mixeur
        return push_samples( mono, n );

    int16_t tmp[256];
    uint32_t t = 0;
    for ( uint32_t i = 0; i < n; i++ )
    {
        int32_t s = mono[i];
        while ( _rs_phase < 65536 )
        {
                // interpolation lineaire ; (phase >> 1) pour rester sur 32 bits
            tmp[t++] = (int16_t)( _rs_prev + ((((s - _rs_prev) * (int32_t)(_rs_phase >> 1))) >> 15) );
            _rs_phase += _rs_step;
            if ( t == 256 )
            {
                if ( !push_samples( tmp, t ) )
                    return false;
                t = 0;
            }
        }
        _rs_phase -= 65536;
        _rs_prev = s;
    }
    return t ? push_samples( tmp, t ) : true;
}

//=============================================================================
//  Tache de decodage
//=============================================================================
void gb_audio_track_mp3::decode_task_entry( void* arg )
{
    static_cast<gb_audio_track_mp3*>( arg )->decode_task();
}

void gb_audio_track_mp3::decode_task()
{
    mp3dec_t* dec = (mp3dec_t*)malloc( sizeof(mp3dec_t) );
    uint8_t*  in  = (uint8_t*)malloc( GB_MP3_IN_BUF );
    int16_t*  pcm = (int16_t*)malloc( MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t) );
    int16_t*  mono = (int16_t*)malloc( (MINIMP3_MAX_SAMPLES_PER_FRAME / 2) * sizeof(int16_t) );

    if ( dec && in && pcm && mono )
        decode_loop( dec, in, pcm, mono );
    else
        ESP_LOGE( TAG, "out of memory in decoder task" );

    free( dec ); free( in ); free( pcm ); free( mono );
    if ( _f ) { fclose( _f ); _f = 0; }
    _done.store( true, std::memory_order_release );
    xSemaphoreGive( _task_done );
    vTaskDelete( NULL );
}

void gb_audio_track_mp3::decode_loop( void* pdec, uint8_t* in, int16_t* pcm, int16_t* mono )
{
    mp3dec_t* dec = (mp3dec_t*)pdec;
    mp3dec_init( dec );

    int      buf_len = 0;
    bool     eof = false;
    uint32_t file_off = _data_start;    // offset fichier de in[0]
    bool     have_fmt = false;
    uint32_t hz = 0;
    uint32_t idx = 0;                   // echantillons decodes (par canal) depuis le debut
    uint32_t start_idx = 0;             // premiers echantillons a jeter
    uint32_t end_idx = 0xFFFFFFFFu;     // fin utile (exclue)
    uint32_t produced_pass = 0;         // echantillons emis pendant ce passage

    while ( !_stop_req )
    {
        bool eos = false;               // fin de flux atteinte

        if ( !eof && buf_len < GB_MP3_IN_BUF )
        {
            size_t want = GB_MP3_IN_BUF - buf_len;
            size_t got = fread( in + buf_len, 1, want, _f );
            buf_len += (int)got;
            if ( got < want )
                eof = true;
        }

        if ( buf_len == 0 )
        {
            eos = eof;
        }
        else
        {
            mp3dec_frame_info_t info;
            memset( &info, 0, sizeof(info) );   // hz == 0 si aucune trame complete trouvee
            int n = mp3dec_decode_frame( dec, in, buf_len, (mp3d_sample_t*)pcm, &info );

            bool skip_output = false;
            if ( !have_fmt && info.frame_bytes > 0 && info.hz > 0 )
            {
                    // premiere trame valide : format, tag Xing/Info, points de rognage
                have_fmt = true;
                hz = info.hz;
                if ( hz < 8000 || hz > 48000 )
                {
                    ESP_LOGE( TAG, "unsupported sample rate %u", (unsigned)hz );
                    return;
                }
                _rs_step = (uint32_t)( ((uint64_t)hz << 16) / GB_AUDIO_SAMPLE_RATE );

                mp3_info_tag_t tag;
                parse_info_tag( in + info.frame_offset, info.frame_bytes - info.frame_offset, &tag );
                    // echantillons par trame et par canal (couche III) : MPEG1 = 1152, sinon 576
                uint32_t spf = (in[info.frame_offset + 1] & 0x08) ? 1152 : 576;
                if ( tag.found )
                {
                    skip_output = true;                 // la trame Info est du silence
                    _audio_start = file_off + info.frame_bytes;
                    if ( tag.has_delay )
                        start_idx = tag.delay + MP3_DEC_DELAY;
                    if ( tag.frames )
                    {
                        uint32_t total = tag.frames * spf;
                        uint32_t e = total;
                        if ( tag.has_delay )
                            e = (total > tag.padding) ? total - tag.padding + MP3_DEC_DELAY : total;
                        if ( e > start_idx )
                        {
                            end_idx = e;
                            _length_ms = (uint32_t)( (uint64_t)(e - start_idx) * 1000 / hz );
                        }
                    }
                    ESP_LOGI( TAG, "Info tag: %u frames, delay %u, padding %u",
                              (unsigned)tag.frames, (unsigned)tag.delay, (unsigned)tag.padding );
                }
                else
                {
                    _audio_start = file_off + info.frame_offset;
                }
            }

            if ( info.frame_bytes == 0 )
            {
                if ( eof )
                    eos = true;                         // plus de trame complete
                else
                {
                    info.frame_bytes = (buf_len > 1) ? buf_len / 2 : 1;   // aucune trame dans tout le tampon : on avance
                }
            }

            if ( n > 0 && have_fmt && !skip_output )
            {
                int ch = info.channels;
                for ( int i = 0; i < n; i++ )
                    mono[i] = (ch == 1) ? pcm[i] : (int16_t)( ((int32_t)pcm[2*i] + pcm[2*i+1]) >> 1 );

                uint32_t fb = idx, fe = idx + (uint32_t)n;
                if ( end_idx <= fb )
                {
                    eos = true;
                }
                else
                {
                    uint32_t b = (start_idx > fb) ? start_idx - fb : 0;
                    if ( b > (uint32_t)n ) b = (uint32_t)n;
                    uint32_t e = (uint32_t)n;
                    if ( end_idx < fe )
                    {
                        e = end_idx - fb;
                        eos = true;
                    }
                    if ( e > b )
                    {
                        if ( !emit_resampled( mono + b, e - b ) )
                            return;
                        produced_pass += e - b;
                    }
                }
                idx = fe;
            }
                // (la trame Info n'est pas comptee : la numerotation commence apres elle,
                //  comme dans les formules de delai/bourrage LAME)

                // consomme les octets utilises
            if ( info.frame_bytes > 0 )
            {
                int used = info.frame_bytes;
                if ( used > buf_len ) used = buf_len;
                memmove( in, in + used, buf_len - used );
                buf_len -= used;
                file_off += used;
            }
        }

        if ( eos )
        {
            if ( _loop && produced_pass > 0 )
            {
                    // reboucle sans trou : retour a la premiere trame audio
                fseek( _f, _audio_start, SEEK_SET );
                buf_len = 0;
                eof = false;
                file_off = _audio_start;
                mp3dec_init( dec );
                idx = 0;
                produced_pass = 0;
            }
            else
            {
                return;
            }
        }
    }
}
