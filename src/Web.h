/* Web.h
 *
 * Tiny browser-only helpers for the Emscripten build. On native builds these
 * compile to nothing, so call sites in the game code need no #ifdef guards.
 */
#ifndef WEB_H__
#define WEB_H__

#ifdef __EMSCRIPTEN__

/* Flush the IDBFS-backed save directory to IndexedDB so saves survive a page
 * reload. Cheap to call; safe to call often. */
void Web_syncSaves(void);

/* Display a fatal error message as a DOM overlay (replaces the blocking
 * SDL_ShowMessageBox, which is unsuitable for the browser event loop). */
void Web_showError(const char* msg);

#else

#define Web_syncSaves()    ((void)0)
#define Web_showError(msg) ((void)0)

#endif /* __EMSCRIPTEN__ */

#endif /* WEB_H__ */
