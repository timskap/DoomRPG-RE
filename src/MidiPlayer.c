/* MidiPlayer.c
 *
 * Implementation of the FluidSynth API shim declared in MidiPlayer.h, backed by
 * TinySoundFont + TinyMidiLoader, rendered through SDL_mixer's Mix_HookMusic.
 *
 * Compiled only for the Emscripten/Web build. On native builds this whole file
 * is empty and the real FluidSynth library is used instead.
 */
#ifdef __EMSCRIPTEN__

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"

#include "MidiPlayer.h"

/* gm.sf2 is preloaded at the MEMFS root; the working dir is /save (IDBFS). */
#define SOUNDFONT_PATH "/gm.sf2"
#define MIDI_SAMPLERATE 44100

/* The game only ever plays a single MIDI track at a time (the dedicated music
 * channel), so a single shared synth + "currently playing" pointer suffices. */
struct fluid_settings_t     { int _dummy; };
struct fluid_synth_t        { int _dummy; };
struct fluid_audio_driver_t { int _dummy; };

struct fluid_player_t
{
    tml_message* head;   /* first event (kept for free + loop restart) */
    tml_message* next;   /* next event to dispatch                     */
    double       msec;   /* current playback position in milliseconds  */
    int          loop;   /* -1 = loop forever, 0 = play once           */
    int          status; /* FLUID_PLAYER_*                             */
};

static tsf*             g_tsf          = NULL;
static fluid_player_t*  g_current      = NULL;  /* track being rendered */
static float            g_gain         = 1.0f;  /* linear volume (0..1) */
static int              g_hook_set     = 0;

/* ------------------------------------------------------------------------- */
/* SDL_mixer music hook: render the active MIDI into the music stream.        */
/* In a non-pthread Emscripten build this runs on the main thread, so no      */
/* locking is needed between this and the fluid_player_* calls.               */
/* ------------------------------------------------------------------------- */
static void MidiPlayer_mixCallback(void* udata, Uint8* stream, int len)
{
    short* out = (short*)stream;
    int frames = len / (2 * (int)sizeof(short)); /* interleaved stereo */
    fluid_player_t* p = g_current;
    int block;

    (void)udata;

    if (g_tsf == NULL || p == NULL || p->status != FLUID_PLAYER_PLAYING) {
        SDL_memset(stream, 0, len);
        return;
    }

    for (; frames > 0; frames -= block, out += block * 2) {
        block = (frames < TSF_RENDER_EFFECTSAMPLEBLOCK) ? frames : TSF_RENDER_EFFECTSAMPLEBLOCK;

        /* dispatch all events whose timestamp has been reached */
        for (p->msec += block * (1000.0 / (double)MIDI_SAMPLERATE);
             p->next && p->msec >= p->next->time;
             p->next = p->next->next)
        {
            tml_message* m = p->next;
            switch (m->type) {
                case TML_PROGRAM_CHANGE:
                    tsf_channel_set_presetnumber(g_tsf, m->channel, m->program, (m->channel == 9));
                    break;
                case TML_NOTE_ON:
                    tsf_channel_note_on(g_tsf, m->channel, m->key, m->velocity / 127.0f);
                    break;
                case TML_NOTE_OFF:
                    tsf_channel_note_off(g_tsf, m->channel, m->key);
                    break;
                case TML_PITCH_BEND:
                    tsf_channel_set_pitchwheel(g_tsf, m->channel, m->pitch_bend);
                    break;
                case TML_CONTROL_CHANGE:
                    tsf_channel_midi_control(g_tsf, m->channel, m->control, m->control_value);
                    break;
                default:
                    break;
            }
        }

        tsf_render_short(g_tsf, out, block, 0);

        /* end of track reached */
        if (p->next == NULL) {
            if (p->loop != 0) {
                if (p->loop > 0) p->loop--;
                p->next = p->head;
                p->msec = 0.0;
                tsf_reset(g_tsf);
            } else {
                p->status = FLUID_PLAYER_DONE;
                tsf_note_off_all(g_tsf);
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/* settings / synth / audio driver                                            */
/* ------------------------------------------------------------------------- */
fluid_settings_t* new_fluid_settings(void)
{
    return (fluid_settings_t*)SDL_calloc(1, sizeof(fluid_settings_t));
}

void delete_fluid_settings(fluid_settings_t* settings)
{
    SDL_free(settings);
}

fluid_synth_t* new_fluid_synth(fluid_settings_t* settings)
{
    (void)settings;
    return (fluid_synth_t*)SDL_calloc(1, sizeof(fluid_synth_t));
}

void delete_fluid_synth(fluid_synth_t* synth)
{
    if (g_tsf) {
        tsf_close(g_tsf);
        g_tsf = NULL;
    }
    SDL_free(synth);
}

fluid_audio_driver_t* new_fluid_audio_driver(fluid_settings_t* settings, fluid_synth_t* synth)
{
    (void)settings; (void)synth;
    /* The actual audio output is wired through Mix_HookMusic the first time a
     * track is played (Mix_OpenAudio runs after this point during init). */
    return (fluid_audio_driver_t*)SDL_calloc(1, sizeof(fluid_audio_driver_t));
}

void delete_fluid_audio_driver(fluid_audio_driver_t* driver)
{
    if (g_hook_set) {
        Mix_HookMusic(NULL, NULL);
        g_hook_set = 0;
    }
    g_current = NULL;
    SDL_free(driver);
}

int fluid_settings_setnum(fluid_settings_t* settings, const char* name, double val)
{
    (void)settings;
    if (name && SDL_strcmp(name, "synth.gain") == 0) {
        g_gain = (float)val;
        if (g_tsf) {
            tsf_set_volume(g_tsf, g_gain);
        }
    }
    return FLUID_OK;
}

int fluid_is_soundfont(const char* filename)
{
    /* The soundfont is bundled at build time; assume present. If loading
     * actually fails, music is silently skipped (see fluid_synth_sfload). */
    (void)filename;
    return 1;
}

int fluid_synth_sfload(fluid_synth_t* synth, const char* filename, int reset_presets)
{
    (void)synth; (void)filename; (void)reset_presets;
    if (g_tsf) {
        tsf_close(g_tsf);
        g_tsf = NULL;
    }
    g_tsf = tsf_load_filename(SOUNDFONT_PATH);
    if (g_tsf == NULL) {
        printf("MidiPlayer: failed to load soundfont %s (music disabled)\n", SOUNDFONT_PATH);
        return FLUID_FAILED;
    }
    tsf_set_output(g_tsf, TSF_STEREO_INTERLEAVED, MIDI_SAMPLERATE, 0.0f);
    tsf_set_volume(g_tsf, g_gain);
    /* Pre-allocate voices so note-on never reallocs from the audio callback. */
    tsf_set_max_voices(g_tsf, 48);
    return 0; /* soundfont id (game ignores it) */
}

/* ------------------------------------------------------------------------- */
/* MIDI player                                                                */
/* ------------------------------------------------------------------------- */
fluid_player_t* new_fluid_player(fluid_synth_t* synth)
{
    fluid_player_t* p;
    (void)synth;
    p = (fluid_player_t*)SDL_calloc(1, sizeof(fluid_player_t));
    if (p) {
        p->status = FLUID_PLAYER_READY;
    }
    return p;
}

void delete_fluid_player(fluid_player_t* player)
{
    if (player == NULL) return;
    if (g_current == player) {
        g_current = NULL;
    }
    if (player->head) {
        tml_free(player->head);
    }
    SDL_free(player);
}

int fluid_player_add_mem(fluid_player_t* player, const void* buffer, size_t len)
{
    if (player == NULL) return FLUID_FAILED;
    if (player->head) {
        tml_free(player->head);
        player->head = NULL;
    }
    player->head = tml_load_memory(buffer, (int)len);
    player->next = player->head;
    player->msec = 0.0;
    return (player->head != NULL) ? FLUID_OK : FLUID_FAILED;
}

int fluid_player_play(fluid_player_t* player)
{
    if (player == NULL || player->head == NULL) return FLUID_FAILED;

    /* (Re)start from the beginning. */
    player->next   = player->head;
    player->msec   = 0.0;
    player->status = FLUID_PLAYER_PLAYING;

    if (g_tsf) {
        tsf_reset(g_tsf);
    }
    g_current = player;

    /* Lazily install the music hook once the mixer has been opened. */
    if (!g_hook_set) {
        Mix_HookMusic(MidiPlayer_mixCallback, NULL);
        g_hook_set = 1;
    }
    return FLUID_OK;
}

int fluid_player_stop(fluid_player_t* player)
{
    if (player == NULL) return FLUID_FAILED;
    player->status = FLUID_PLAYER_DONE;
    if (g_current == player && g_tsf) {
        tsf_note_off_all(g_tsf);
    }
    return FLUID_OK;
}

int fluid_player_seek(fluid_player_t* player, int ticks)
{
    if (player == NULL) return FLUID_FAILED;
    /* The game only ever seeks to 0 (rewind). */
    (void)ticks;
    player->next = player->head;
    player->msec = 0.0;
    return FLUID_OK;
}

int fluid_player_set_loop(fluid_player_t* player, int loop)
{
    if (player == NULL) return FLUID_FAILED;
    player->loop = loop;
    return FLUID_OK;
}

int fluid_player_get_status(fluid_player_t* player)
{
    if (player == NULL) return FLUID_PLAYER_DONE;
    return player->status;
}

#endif /* __EMSCRIPTEN__ */
