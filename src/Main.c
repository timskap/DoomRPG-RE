//Using SDL and standard IO
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <zlib.h>

#include "Z_Zone.h"
#include "DoomRPG.h"
#include "DoomCanvas.h"
#include "Player.h"
#include "Hud.h"
#include "MenuSystem.h"
#include "SDL_Video.h"
#include "Z_Zip.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <unistd.h>
#define DOOMRPG_ZIP_PATH "/DoomRPG.zip"
#else
#define DOOMRPG_ZIP_PATH "DoomRPG.zip"
#endif

extern DoomRPG_t* doomRpg;

// Loop state. Kept at file scope so it survives across the per-frame callback
// when driven by emscripten_set_main_loop (the browser cannot block in a
// while-loop), while the native build still drives it from a plain while-loop.
static int s_UpTime = 0;
static int s_mouseTime = 0;
static int s_key = 0;
static int s_oldKey = -1;
static const Uint8* s_state = NULL;

static void DoomRPG_mainLoop(void)
{
	SDL_Event ev;
	int mouse_Button;

	int currentTimeMillis = DoomRPG_GetUpTimeMS();

	mouse_Button = MOUSE_BUTTON_INVALID;

	while (SDL_PollEvent(&ev))
	{
		// check event type
		switch (ev.type) {

			// Mouse Event
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				Uint32 buttons = SDL_GetMouseState(NULL, NULL);

				if ((buttons & SDL_BUTTON_LMASK) != 0) {
					mouse_Button = MOUSE_BUTTON_LEFT;
				}
				else if ((buttons & SDL_BUTTON_MMASK) != 0) {
					mouse_Button = MOUSE_BUTTON_MIDDLE;
				}
				else if ((buttons & SDL_BUTTON_RMASK) != 0) {
					mouse_Button = MOUSE_BUTTON_RIGHT;
				}
				else if ((buttons & SDL_BUTTON_X1MASK) != 0) {
					mouse_Button = MOUSE_BUTTON_X1;
				}
				else if ((buttons & SDL_BUTTON_X2MASK) != 0) {
					mouse_Button = MOUSE_BUTTON_X2;
				}
				break;
			}

			case SDL_MOUSEWHEEL:
			{
				if (currentTimeMillis > s_mouseTime) {
					s_mouseTime = currentTimeMillis + 128;
					if (ev.wheel.y > 0) {
						mouse_Button = MOUSE_BUTTON_WHELL_UP;
					}
					else if (ev.wheel.y < 0) {
						mouse_Button = MOUSE_BUTTON_WHELL_DOWN;
					}
				}
				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (!doomRpg->menuSystem->setBind) {
					if (currentTimeMillis > s_mouseTime) {
						s_mouseTime = currentTimeMillis + 128;
						int x = 0, y = 0;
						SDL_GetRelativeMouseState(&x, &y);

						int sensivity = (doomRpg->doomCanvas->mouseSensitivity * 1000) / 100;

						if (x <= -sensivity) {
							mouse_Button = MOUSE_BUTTON_MOTION_LEFT;
						}
						else if (x >= sensivity) {
							mouse_Button = MOUSE_BUTTON_MOTION_RIGHT;
						}

						if (doomRpg->doomCanvas->mouseYMove) {
							if (y <= -sensivity) {
								mouse_Button = MOUSE_BUTTON_MOTION_UP;
							}
							else if (y >= sensivity) {
								mouse_Button = MOUSE_BUTTON_MOTION_DOWN;
							}
						}
					}
				}
				break;
			}

			case SDL_WINDOWEVENT:
			{
				if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
					//printf("MESSAGE:Resizing window...\n");
					//SDL_Rect rect = { 0,0,640,480};
					//SDL_RenderSetViewport(sdlVideo.renderer, &rect);
					//resizeWindow();
				}

				if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
					SDL_Log("Window %d closed", ev.window.windowID);
					doomRpg->closeApplet = true;
#ifdef __EMSCRIPTEN__
					emscripten_cancel_main_loop();
					return;
#else
					closeZipFile(&zipFile);
					DoomRPG_FreeAppData(doomRpg);
					DoomRPG_CloseAudio();
					SDL_Close();
					exit(0);
#endif
					break;
				}

				if (ev.window.event != SDL_WINDOWEVENT_CLOSE)
				{
					int w, h;
					SDL_GetWindowSize(sdlVideo.window, &w, &h);
					SDL_WarpMouseInWindow(sdlVideo.window, w / 2, h / 2);
					SDL_GetRelativeMouseState(NULL, NULL);
				}
				break;
			}

			case SDL_QUIT:
			{
				// shut down
				doomRpg->closeApplet = true;
#ifdef __EMSCRIPTEN__
				emscripten_cancel_main_loop();
				return;
#else
				exit(0);
#endif
				break;
			}
		}

		s_key = DoomRPG_getEventKey(mouse_Button, s_state);
		if (s_key != s_oldKey) {
			//printf("oldKey %d\n", s_oldKey);
			//printf("key %d\n", s_key);

			s_oldKey = s_key;
			if (!doomRpg->menuSystem->setBind) {
				DoomCanvas_keyPressed(doomRpg->doomCanvas, s_key);
			}
			else {
				goto setBind;
			}
		}
		else if (s_key == 0) {
		setBind:
			if (doomRpg->menuSystem->setBind) {
				DoomRPG_setBind(doomRpg, mouse_Button, s_state);
			}
		}
	}

	if (currentTimeMillis > s_UpTime) {
		s_UpTime = currentTimeMillis + 15;
		DoomRPG_loopGame(doomRpg);
	}

#ifdef __EMSCRIPTEN__
	if (doomRpg->closeApplet == true) {
		emscripten_cancel_main_loop();
	}
#endif
}

int main(int argc, char* args[])
{
#ifdef __EMSCRIPTEN__
	// Run with the IDBFS-backed save directory as the working directory so the
	// game's relative save files (Config/Player/Player2/World) persist; assets
	// are loaded by absolute path from the MEMFS root.
	chdir("/save");
#endif

	Z_Init();
	SDL_InitVideo();
	SDL_InitAudio();

	openZipFile(DOOMRPG_ZIP_PATH, &zipFile);

	if (DoomRPG_Init() == 0) {
		DoomRPG_Error("Failed to initialize Doom Rpg\n");
	}

	//Hud_addMessage(doomRpg->hud, "Bienvenido a Doom RPG por GEC...");

	s_state = SDL_GetKeyboardState(NULL);

	s_key = 0;
	s_oldKey = -1;

#ifdef __EMSCRIPTEN__
	// The browser owns the event loop: register a per-frame callback and return.
	emscripten_set_main_loop(DoomRPG_mainLoop, 0, 1);
#else
	while (doomRpg->closeApplet != true)
	{
		DoomRPG_mainLoop();
	}

	closeZipFile(&zipFile);
	DoomRPG_FreeAppData(doomRpg);
	DoomRPG_CloseAudio();
	SDL_Close();
#endif

	return 0;
}
