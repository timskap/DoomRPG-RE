/* MidiPlayer.h
 *
 * Drop-in FluidSynth API shim for the Emscripten/Web build, backed by
 * TinySoundFont (tsf.h) + TinyMidiLoader (tml.h).
 *
 * FluidSynth depends on glib and spins up its own audio thread, neither of
 * which is available under Emscripten. This header re-declares exactly the
 * subset of the FluidSynth API that DoomRPG-RE uses, so SDL_Video.c / Sound.c
 * compile unchanged. The implementation (MidiPlayer.c) renders the active MIDI
 * through SDL_mixer's Mix_HookMusic callback.
 *
 * Only compiled/used when __EMSCRIPTEN__ is defined; the native build still
 * includes the real <fluidsynth.h>.
 */
#ifndef MIDIPLAYER_H__
#define MIDIPLAYER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle types (used only as pointers by the game code). */
typedef struct fluid_settings_t     fluid_settings_t;
typedef struct fluid_synth_t        fluid_synth_t;
typedef struct fluid_audio_driver_t fluid_audio_driver_t;
typedef struct fluid_player_t       fluid_player_t;

/* fluid_player_status enum subset (only PLAYING is compared against). */
#define FLUID_PLAYER_READY   0
#define FLUID_PLAYER_PLAYING 1
#define FLUID_PLAYER_DONE    2

#define FLUID_OK   0
#define FLUID_FAILED -1

/* settings / synth / audio driver lifecycle */
fluid_settings_t*     new_fluid_settings(void);
void                  delete_fluid_settings(fluid_settings_t* settings);
fluid_synth_t*        new_fluid_synth(fluid_settings_t* settings);
void                  delete_fluid_synth(fluid_synth_t* synth);
fluid_audio_driver_t* new_fluid_audio_driver(fluid_settings_t* settings, fluid_synth_t* synth);
void                  delete_fluid_audio_driver(fluid_audio_driver_t* driver);

int  fluid_settings_setnum(fluid_settings_t* settings, const char* name, double val);

int  fluid_is_soundfont(const char* filename);
int  fluid_synth_sfload(fluid_synth_t* synth, const char* filename, int reset_presets);

/* MIDI player */
fluid_player_t* new_fluid_player(fluid_synth_t* synth);
void            delete_fluid_player(fluid_player_t* player);
int             fluid_player_add_mem(fluid_player_t* player, const void* buffer, size_t len);
int             fluid_player_play(fluid_player_t* player);
int             fluid_player_stop(fluid_player_t* player);
int             fluid_player_seek(fluid_player_t* player, int ticks);
int             fluid_player_set_loop(fluid_player_t* player, int loop);
int             fluid_player_get_status(fluid_player_t* player);

#ifdef __cplusplus
}
#endif

#endif /* MIDIPLAYER_H__ */
