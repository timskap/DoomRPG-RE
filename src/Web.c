/* Web.c -- Emscripten-only browser helpers (see Web.h). Empty on native. */
#ifdef __EMSCRIPTEN__

#include <SDL.h>
#include "DoomRPG.h"      /* before <emscripten.h>: it pulls in <stdbool.h>, */
#include "DoomCanvas.h"   /* which would clash with DoomRPG.h's boolean enum */
#include <emscripten.h>
#include "Web.h"

extern DoomRPG_t* doomRpg;

EM_JS(void, Web_syncSaves, (void), {
	if (typeof FS !== 'undefined' && FS.syncfs) {
		FS.syncfs(false, function (err) {
			if (err) console.warn('[DoomRPG] FS.syncfs(save) failed:', err);
		});
	}
});

EM_JS(void, Web_showError, (const char* msg), {
	var text = UTF8ToString(msg);
	console.error('[DoomRPG] ' + text);
	if (typeof document === 'undefined') return;
	var d = document.getElementById('doomrpg-error');
	if (!d) {
		d = document.createElement('div');
		d.id = 'doomrpg-error';
		d.style.cssText = 'position:fixed;left:0;right:0;top:0;z-index:9999;' +
			'background:#600;color:#fff;font:14px monospace;padding:12px;' +
			'white-space:pre-wrap;border-bottom:2px solid #f00;';
		document.body.appendChild(d);
	}
	d.textContent = 'DoomRPG Error:\n' + text;
});

/* --- Glasses 5-button input bridge (called from web/shell.html) --- */

/* Returns 1 while the 3D game is interactive (movement or combat), so the
 * on-screen action bar may be opened with Down; 0 in menus/dialogs/etc.,
 * where the arrow keys must pass through to the engine unchanged. */
EMSCRIPTEN_KEEPALIVE int Web_inGame(void)
{
	int s;
	if (!doomRpg || !doomRpg->doomCanvas) return 0;
	s = doomRpg->doomCanvas->state;
	return (s == ST_PLAYING || s == ST_COMBAT) ? 1 : 0;
}

/* Execute one action-bar entry; indices match the JS BAR_ACTIONS list. */
EMSCRIPTEN_KEEPALIVE void Web_doAction(int actionId)
{
	int avk;
	if (!doomRpg || !doomRpg->doomCanvas) return;
	switch (actionId) {
		case 0: avk = AVK_DOWN; break;                     /* Move backward */
		case 1: avk = AVK_PREVWEAPON; break;               /* Prev weapon   */
		case 2: avk = AVK_NEXTWEAPON; break;               /* Next weapon   */
		case 3: avk = AVK_PASSTURN; break;                 /* Pass turn     */
		case 4: avk = AVK_AUTOMAP; break;                  /* Automap       */
		case 5: avk = AVK_MENUOPEN | AVK_MENU_OPEN; break; /* Menu / back   */
		default: return;
	}
	DoomCanvas_keyPressed(doomRpg->doomCanvas, avk);
}

#endif /* __EMSCRIPTEN__ */
