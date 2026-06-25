/* Web.c -- Emscripten-only browser helpers (see Web.h). Empty on native. */
#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include "Web.h"

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

#endif /* __EMSCRIPTEN__ */
