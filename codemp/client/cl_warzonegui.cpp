#include "client.h"
#include "ui/keycodes.h"

namespace WarzoneGUI
{
	qboolean keyStatus[MAX_KEYS] = { qfalse };
	vec2_t mouseStatus = { 0.5, 0.5 };
	//qboolean menuOpen = qfalse;

	void MenuOpenEvent(qboolean open)
	{
		//menuOpen = open;

		if (open)
			Cvar_Set("ui_warzonegui", "1");
		else
			Cvar_Set("ui_warzonegui", "0");
	}

	void KeyEvent(int key, qboolean down)
	{
		keyStatus[key] = down;
	}

	void MouseEvent(int x, int y) {
#if 0
		static int actual_x = 0, actual_y = 0;

		actual_x += x;
		if (actual_x < 0) actual_x = 0;
		if (actual_x > cls.glconfig.vidWidth) actual_x = cls.glconfig.vidWidth;
		actual_y += y;
		if (actual_y < 0) actual_y = 0;
		if (actual_y > cls.glconfig.vidHeight) actual_y = cls.glconfig.vidHeight;

		mouseStatus[0] = actual_x;// x;
		mouseStatus[1] = actual_y;// y;
#else
		// update mouse screen position
		mouseStatus[0] += x;
		if (mouseStatus[0] < 0)
			mouseStatus[0] = 0;
		else if (mouseStatus[0] > SCREEN_WIDTH)
			mouseStatus[0] = SCREEN_WIDTH;

		mouseStatus[1] += y;
		if (mouseStatus[1] < 0)
			mouseStatus[1] = 0;
		else if (mouseStatus[1] > SCREEN_HEIGHT)
			mouseStatus[1] = SCREEN_HEIGHT;
#endif
	}

	void SendKeyboardStatusToRenderer(void)
	{
		//Com_Printf("CLIENT: Mouse is at %f %f.\n", mouseStatus[0], mouseStatus[1]);
		re->R_SendInputEvents(keyStatus, mouseStatus, (qboolean)Cvar_VariableIntegerValue("ui_warzonegui"));
	}
}
