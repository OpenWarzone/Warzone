/* nuklear - v1.05 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "imgui/imgui_api.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_IMPLEMENTATION
//#define NK_BUTTON_TRIGGER_ON_RELEASE

//
// Warzone GUI Basic Defines...
//

//#define __DEMOS__					// Enables demo example widgets...

//
// Warzone GUI Theme Defines...
//

//#define __GUI_SKINNED__			// Uses skinning... Would need to edit the file warzone/gui/skins/warzone.png to use this.
#define __GUI_THEMED__				// Uses themeing... Enable one below, as well.

#if defined(__GUI_THEMED__) && !defined(__GUI_SKINNED__)
	//#define __GUI_THEME_BLACK__
	//#define __GUI_THEME_WHITE__
	//#define __GUI_THEME_RED__
	//#define __GUI_THEME_BLUE__
	//#define __GUI_THEME_DARK__
#define __GUI_THEME_WARZONE__
#endif

//
//
//

#include "nuklear.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "tr_local.h"

/* macros */
#define FBO_WIDTH (glConfig.vidWidth * r_superSampleMultiplier->value)
#define FBO_HEIGHT (glConfig.vidHeight * r_superSampleMultiplier->value)

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
//#define MIN(a,b) ((a) < (b) ? (a) : (b))
//#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])

#ifdef __APPLE__
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif

qboolean keyStatus[MAX_KEYS] = { qfalse };
vec2_t mouseStatus = { 0.5, 0.5 };
qboolean menuOpen = qfalse;

void RE_SendInputEvents(qboolean clientKeyStatus[MAX_KEYS], vec2_t clientMouseStatus, qboolean open)
{
	memcpy(keyStatus, clientKeyStatus, sizeof(keyStatus));
	mouseStatus[0] = clientMouseStatus[0];
	mouseStatus[1] = clientMouseStatus[1];

	if (mouseStatus[0] > SCREEN_WIDTH) mouseStatus[0] = SCREEN_WIDTH;
	if (mouseStatus[1] > SCREEN_HEIGHT) mouseStatus[1] = SCREEN_HEIGHT;
	if (mouseStatus[0] < 0) mouseStatus[0] = 0;
	if (mouseStatus[1] < 0) mouseStatus[1] = 0;

	menuOpen = open;
}


struct media {
	struct nk_font *font_14;
	struct nk_font *font_15;
	struct nk_font *font_16;
	struct nk_font *font_17;
	struct nk_font *font_18;
	struct nk_font *font_19;
	struct nk_font *font_20;
	struct nk_font *font_21;
	struct nk_font *font_22;
	struct nk_font *font_24;
	struct nk_font *font_28;

	struct nk_image unchecked;
	struct nk_image checked;
	struct nk_image rocket;
	struct nk_image cloud;
	struct nk_image pen;
	struct nk_image play;
	struct nk_image pause;
	struct nk_image stop;
	struct nk_image prev;
	struct nk_image next;
	struct nk_image tools;
	struct nk_image dir;
	struct nk_image copy;
	struct nk_image convert;
	struct nk_image del;
	struct nk_image edit;
	struct nk_image volume;
	struct nk_image inventory[64];
	struct nk_image inventoryBlank;
	struct nk_image menu[6];

	struct nk_image iconMainMenu;

	struct nk_image iconAbilities;
	struct nk_image iconCharacter;
	struct nk_image iconInventory;
	struct nk_image iconPowers;
	struct nk_image iconRadio;
	struct nk_image iconSettings;

#if defined(__GUI_SKINNED__)
	// Skinning...
	GLint skin;
	struct nk_image menuSkin;
	struct nk_image checkSkin;
	struct nk_image check_cursorSkin;
	struct nk_image optionSkin;
	struct nk_image option_cursorSkin;
	struct nk_image headerSkin;
	struct nk_image windowSkin;
	struct nk_image scrollbar_inc_buttonSkin;
	struct nk_image scrollbar_inc_button_hoverSkin;
	struct nk_image scrollbar_dec_buttonSkin;
	struct nk_image scrollbar_dec_button_hoverSkin;
	struct nk_image buttonSkin;
	struct nk_image button_hoverSkin;
	struct nk_image button_activeSkin;
	struct nk_image tab_minimizeSkin;
	struct nk_image tab_maximizeSkin;
	struct nk_image sliderSkin;
	struct nk_image slider_hoverSkin;
	struct nk_image slider_activeSkin;
#endif
};

#if defined(__GUI_THEMED__) && !defined(__GUI_SKINNED__)
enum theme { THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK, THEME_WARZONE
};

void
set_style(struct nk_context *ctx, enum theme theme)
{
	struct nk_color table[NK_COLOR_COUNT];
	if (theme == THEME_WHITE) {
		table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
		table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
		table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
		table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
		table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
		table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
		table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
		table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
		table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
		table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
		table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
		table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
		table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
		table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
		table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
		table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
		table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
		table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
		table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
		nk_style_from_table(ctx, table);
	}
	else if (theme == THEME_RED) {
		table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
		table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
		table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
		table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
		table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
		table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
		table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
		table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
		table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
		table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
		table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
		table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
		table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
		table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
		table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
		table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
		table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
		table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
		nk_style_from_table(ctx, table);
	}
	else if (theme == THEME_BLUE) {
		table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
		table[NK_COLOR_WINDOW] = nk_rgba(202, 212, 214, 215);
		table[NK_COLOR_HEADER] = nk_rgba(137, 182, 224, 220);
		table[NK_COLOR_BORDER] = nk_rgba(140, 159, 173, 255);
		table[NK_COLOR_BUTTON] = nk_rgba(137, 182, 224, 255);
		table[NK_COLOR_BUTTON_HOVER] = nk_rgba(142, 187, 229, 255);
		table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(147, 192, 234, 255);
		table[NK_COLOR_TOGGLE] = nk_rgba(177, 210, 210, 255);
		table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(182, 215, 215, 255);
		table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(137, 182, 224, 255);
		table[NK_COLOR_SELECT] = nk_rgba(177, 210, 210, 255);
		table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(137, 182, 224, 255);
		table[NK_COLOR_SLIDER] = nk_rgba(177, 210, 210, 255);
		table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(137, 182, 224, 245);
		table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(142, 188, 229, 255);
		table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(147, 193, 234, 255);
		table[NK_COLOR_PROPERTY] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_EDIT] = nk_rgba(210, 210, 210, 225);
		table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
		table[NK_COLOR_COMBO] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_CHART] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_CHART_COLOR] = nk_rgba(137, 182, 224, 255);
		table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		table[NK_COLOR_SCROLLBAR] = nk_rgba(190, 200, 200, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
		table[NK_COLOR_TAB_HEADER] = nk_rgba(156, 193, 220, 255);
		nk_style_from_table(ctx, table);
	}
	else if (theme == THEME_DARK) {
		table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
		table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
		table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
		table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
		table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
		table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
		table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
		table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
		table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
		table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
		nk_style_from_table(ctx, table);
	}
	else if (theme == THEME_WARZONE) {
		table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_WINDOW] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_HEADER] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*220*/);
		table[NK_COLOR_BORDER] = nk_rgba(16, 16, 16, gui_windowTransparancy->value*255);
		table[NK_COLOR_BUTTON] = nk_rgba(0, 0, 0, 0);
		table[NK_COLOR_BUTTON_HOVER] = nk_rgba(32, 32, 64, gui_windowTransparancy->value * 255);
		table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(0, 0, 0, 0); // nk_rgba(96, 96, 96, gui_windowTransparancy->value * 255);
		table[NK_COLOR_TOGGLE] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 111, 255);
		table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
		table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, gui_windowTransparancy->value*255.0/*245*/);
		table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		table[NK_COLOR_PROPERTY] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_EDIT] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
		table[NK_COLOR_COMBO] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_CHART] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255.0/*215*/);
		table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		table[NK_COLOR_TAB_HEADER] = nk_rgba(0, 0, 0, gui_windowTransparancy->value*255);
		nk_style_from_table(ctx, table);
	}
	else {
		nk_style_default(ctx);
	}
}
#endif

/* ===============================================================
*
*                          CUSTOM WIDGET
*
* ===============================================================*/
static int
ui_piemenu(struct nk_context *ctx, struct nk_vec2 pos, float radius,
	struct nk_image *icons, int item_count)
{
	int ret = -1;
	struct nk_rect total_space;
	struct nk_rect bounds;
	int active_item = 0;

	/* pie menu popup */
	struct nk_color border = ctx->style.window.border_color;
	struct nk_style_item background = ctx->style.window.fixed_background;
	ctx->style.window.fixed_background = nk_style_item_hide();
	ctx->style.window.border_color = nk_rgba(0, 0, 0, 0);

	total_space = nk_window_get_content_region(ctx);
	ctx->style.window.spacing = nk_vec2(0, 0);
	ctx->style.window.padding = nk_vec2(0, 0);

	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "piemenu", NK_WINDOW_NO_SCROLLBAR,
		nk_rect(pos.x - total_space.x - radius, pos.y - radius - total_space.y,
			2 * radius, 2 * radius)))
	{
		int i = 0;
		struct nk_command_buffer* out = nk_window_get_canvas(ctx);
		const struct nk_input *in = &ctx->input;

		total_space = nk_window_get_content_region(ctx);
		ctx->style.window.spacing = nk_vec2(4, 4);
		ctx->style.window.padding = nk_vec2(8, 8);
		nk_layout_row_dynamic(ctx, total_space.h, 1);
		nk_widget(&bounds, ctx);

		/* outer circle */
		nk_fill_circle(out, bounds, nk_rgb(50, 50, 50));
		{
			/* circle buttons */
			float step = (2 * 3.141592654f) / (float)(MAX(1, item_count));
			float a_min = 0; float a_max = step;

			struct nk_vec2 center = nk_vec2(bounds.x + bounds.w / 2.0f, bounds.y + bounds.h / 2.0f);
			struct nk_vec2 drag = nk_vec2(in->mouse.pos.x - center.x, in->mouse.pos.y - center.y);
			float angle = (float)atan2(drag.y, drag.x);
			if (angle < -0.0f) angle += 2.0f * 3.141592654f;
			active_item = (int)(angle / step);

			for (i = 0; i < item_count; ++i) {
				struct nk_rect content;
				float rx, ry, dx, dy, a;
				nk_fill_arc(out, center.x, center.y, (bounds.w / 2.0f),
					a_min, a_max, (active_item == i) ? nk_rgb(45, 100, 255) : nk_rgb(60, 60, 60));

				/* separator line */
				rx = bounds.w / 2.0f; ry = 0;
				dx = rx * (float)cos(a_min) - ry * (float)sin(a_min);
				dy = rx * (float)sin(a_min) + ry * (float)cos(a_min);
				nk_stroke_line(out, center.x, center.y,
					center.x + dx, center.y + dy, 1.0f, nk_rgb(50, 50, 50));

				/* button content */
				a = a_min + (a_max - a_min) / 2.0f;
				rx = bounds.w / 2.5f; ry = 0;
				content.w = 30; content.h = 30;
				content.x = center.x + ((rx * (float)cos(a) - ry * (float)sin(a)) - content.w / 2.0f);
				content.y = center.y + (rx * (float)sin(a) + ry * (float)cos(a) - content.h / 2.0f);
				nk_draw_image(out, content, &icons[i], nk_rgb(255, 255, 255));
				a_min = a_max; a_max += step;
			}
		}
		{
			/* inner circle */
			struct nk_rect inner;
			inner.x = bounds.x + bounds.w / 2 - bounds.w / 4;
			inner.y = bounds.y + bounds.h / 2 - bounds.h / 4;
			inner.w = bounds.w / 2; inner.h = bounds.h / 2;
			nk_fill_circle(out, inner, nk_rgb(45, 45, 45));

			/* active icon content */
			bounds.w = inner.w / 2.0f;
			bounds.h = inner.h / 2.0f;
			bounds.x = inner.x + inner.w / 2 - bounds.w / 2;
			bounds.y = inner.y + inner.h / 2 - bounds.h / 2;
			nk_draw_image(out, bounds, &icons[active_item], nk_rgb(255, 255, 255));
		}
		nk_layout_space_end(ctx);
		if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_RIGHT)) {
			nk_popup_close(ctx);
			ret = active_item;
		}
	}
	else ret = -2;
	ctx->style.window.spacing = nk_vec2(4, 4);
	ctx->style.window.padding = nk_vec2(8, 8);
	nk_popup_end(ctx);

	ctx->style.window.fixed_background = background;
	ctx->style.window.border_color = border;
	return ret;
}

static int
uq_piemenu(struct nk_context *ctx, struct nk_vec2 pos, float radius,
	struct nk_image *icons, int item_count, struct nk_rect total_space)
{
	int ret = -1;
	//struct nk_rect total_space;
	struct nk_rect bounds;
	int active_item = 0;

	/* pie menu popup */
	struct nk_color border = ctx->style.window.border_color;
	struct nk_style_item background = ctx->style.window.fixed_background;
	ctx->style.window.fixed_background = nk_style_item_hide();
	ctx->style.window.border_color = nk_rgba(0, 0, 0, 0);

	//total_space = nk_window_get_content_region(ctx);
	ctx->style.window.spacing = nk_vec2(0, 0);
	ctx->style.window.padding = nk_vec2(0, 0);

	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "piemenu", NK_WINDOW_NO_SCROLLBAR,
		nk_rect(pos.x - total_space.x - radius, pos.y - radius - total_space.y,
			2 * radius, 2 * radius)))
	{
		int i = 0;
		struct nk_command_buffer* out = nk_window_get_canvas(ctx);
		const struct nk_input *in = &ctx->input;

		total_space = nk_window_get_content_region(ctx);
		ctx->style.window.spacing = nk_vec2(4, 4);
		ctx->style.window.padding = nk_vec2(8, 8);
		nk_layout_row_dynamic(ctx, total_space.h, 1);
		nk_widget(&bounds, ctx);

		/* outer circle */
		nk_fill_circle(out, bounds, nk_rgb(50, 50, 50));
		{
			/* circle buttons */
			float step = (2 * 3.141592654f) / (float)(MAX(1, item_count));
			float a_min = 0; float a_max = step;

			struct nk_vec2 center = nk_vec2(bounds.x + bounds.w / 2.0f, bounds.y + bounds.h / 2.0f);
			struct nk_vec2 drag = nk_vec2(in->mouse.pos.x - center.x, in->mouse.pos.y - center.y);
			float angle = (float)atan2(drag.y, drag.x);
			if (angle < -0.0f) angle += 2.0f * 3.141592654f;
			active_item = (int)(angle / step);

			for (i = 0; i < item_count; ++i) {
				struct nk_rect content;
				float rx, ry, dx, dy, a;
				nk_fill_arc(out, center.x, center.y, (bounds.w / 2.0f),
					a_min, a_max, (active_item == i) ? nk_rgb(45, 100, 255) : nk_rgb(60, 60, 60));

				/* separator line */
				rx = bounds.w / 2.0f; ry = 0;
				dx = rx * (float)cos(a_min) - ry * (float)sin(a_min);
				dy = rx * (float)sin(a_min) + ry * (float)cos(a_min);
				nk_stroke_line(out, center.x, center.y,
					center.x + dx, center.y + dy, 1.0f, nk_rgb(50, 50, 50));

				/* button content */
				a = a_min + (a_max - a_min) / 2.0f;
				rx = bounds.w / 2.5f; ry = 0;
				content.w = 30; content.h = 30;
				content.x = center.x + ((rx * (float)cos(a) - ry * (float)sin(a)) - content.w / 2.0f);
				content.y = center.y + (rx * (float)sin(a) + ry * (float)cos(a) - content.h / 2.0f);
				nk_draw_image(out, content, &icons[i], nk_rgb(255, 255, 255));
				a_min = a_max; a_max += step;
			}
		}
		{
			/* inner circle */
			struct nk_rect inner;
			inner.x = bounds.x + bounds.w / 2 - bounds.w / 4;
			inner.y = bounds.y + bounds.h / 2 - bounds.h / 4;
			inner.w = bounds.w / 2; inner.h = bounds.h / 2;
			nk_fill_circle(out, inner, nk_rgb(45, 45, 45));

			/* active icon content */
			bounds.w = inner.w / 2.0f;
			bounds.h = inner.h / 2.0f;
			bounds.x = inner.x + inner.w / 2 - bounds.w / 2;
			bounds.y = inner.y + inner.h / 2 - bounds.h / 2;
			nk_draw_image(out, bounds, &icons[active_item], nk_rgb(255, 255, 255));
		}
		nk_layout_space_end(ctx);
		if (!nk_input_is_mouse_down(&ctx->input, NK_BUTTON_RIGHT)) {
			nk_popup_close(ctx);
			ret = active_item;
		}
	}
	else ret = -2;
	ctx->style.window.spacing = nk_vec2(4, 4);
	ctx->style.window.padding = nk_vec2(8, 8);
	nk_popup_end(ctx);

	ctx->style.window.fixed_background = background;
	ctx->style.window.border_color = border;
	return ret;
}

/* ===============================================================
*
*                          GRID
*
* ===============================================================*/
static void
grid_demo(struct nk_context *ctx, struct media *media)
{
	static char text[3][64];
	static int text_len[3];
	static const char *items[] = { "Item 0","item 1","item 2" };
	static int selected_item = 0;
	static int check = 1;

	int i;
	nk_style_set_font(ctx, &media->font_20->handle);
	if (nk_begin(ctx, "Grid Demo", nk_rect(600, 350, 275, 250),
		NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE |
		NK_WINDOW_NO_SCROLLBAR))
	{
		nk_style_set_font(ctx, &media->font_18->handle);
		nk_layout_row_dynamic(ctx, 30, 2);
		nk_label(ctx, "Floating point:", NK_TEXT_RIGHT);
		nk_edit_string(ctx, NK_EDIT_FIELD, text[0], &text_len[0], 64, nk_filter_float);
		nk_label(ctx, "Hexadecimal:", NK_TEXT_RIGHT);
		nk_edit_string(ctx, NK_EDIT_FIELD, text[1], &text_len[1], 64, nk_filter_hex);
		nk_label(ctx, "Binary:", NK_TEXT_RIGHT);
		nk_edit_string(ctx, NK_EDIT_FIELD, text[2], &text_len[2], 64, nk_filter_binary);
		nk_label(ctx, "Checkbox:", NK_TEXT_RIGHT);
		nk_checkbox_label(ctx, "Check me", &check);
		nk_label(ctx, "Combobox:", NK_TEXT_RIGHT);
		if (nk_combo_begin_label(ctx, items[selected_item], nk_vec2(nk_widget_width(ctx), 200))) {
			nk_layout_row_dynamic(ctx, 25, 1);
			for (i = 0; i < 3; ++i)
				if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
					selected_item = i;
			nk_combo_end(ctx);
		}
	}
	nk_end(ctx);
	nk_style_set_font(ctx, &media->font_14->handle);
}

/* ===============================================================
*
*                          BUTTON DEMO
*
* ===============================================================*/
static void
ui_header(struct nk_context *ctx, struct media *media, const char *title)
{
	nk_style_set_font(ctx, &media->font_18->handle);
	nk_layout_row_dynamic(ctx, 20, 1);
	nk_label(ctx, title, NK_TEXT_LEFT);
}

static void
ui_widget(struct nk_context *ctx, struct media *media, float height)
{
	static const float ratio[] = { 0.15f, 0.85f };
	nk_style_set_font(ctx, &media->font_22->handle);
	nk_layout_row(ctx, NK_DYNAMIC, height, 2, ratio);
	nk_spacing(ctx, 1);
}

static void
ui_widget_centered(struct nk_context *ctx, struct media *media, float height)
{
	static const float ratio[] = { 0.15f, 0.50f, 0.35f };
	nk_style_set_font(ctx, &media->font_22->handle);
	nk_layout_row(ctx, NK_DYNAMIC, height, 3, ratio);
	nk_spacing(ctx, 1);
}

#ifdef __DEMOS__ // Here for examples...
static void
button_demo(struct nk_context *ctx, struct media *media)
{
	static int option = 1;
	static int toggle0 = 1;
	static int toggle1 = 0;
	static int toggle2 = 1;

	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "Button Demo", nk_rect(50, 50, 255, 610),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE);

	/*------------------------------------------------
	*                  MENU
	*------------------------------------------------*/
	nk_menubar_begin(ctx);
	{
		/* toolbar */
		nk_layout_row_static(ctx, 40, 40, 4);
		if (nk_menu_begin_image(ctx, "Music", media->play, nk_vec2(110, 120)))
		{
			/* settings */
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_menu_item_image_label(ctx, media->play, "Play", NK_TEXT_RIGHT);
			nk_menu_item_image_label(ctx, media->stop, "Stop", NK_TEXT_RIGHT);
			nk_menu_item_image_label(ctx, media->pause, "Pause", NK_TEXT_RIGHT);
			nk_menu_item_image_label(ctx, media->next, "Next", NK_TEXT_RIGHT);
			nk_menu_item_image_label(ctx, media->prev, "Prev", NK_TEXT_RIGHT);
			nk_menu_end(ctx);
		}
		nk_button_image(ctx, media->tools);
		nk_button_image(ctx, media->cloud);
		nk_button_image(ctx, media->pen);
	}
	nk_menubar_end(ctx);

	/*------------------------------------------------
	*                  BUTTON
	*------------------------------------------------*/
	ui_header(ctx, media, "Push buttons");
	ui_widget(ctx, media, 35);
	if (nk_button_label(ctx, "Push me"))
		fprintf(stdout, "pushed!\n");
	ui_widget(ctx, media, 35);
	if (nk_button_image_label(ctx, media->rocket, "Styled", NK_TEXT_CENTERED))
		fprintf(stdout, "rocket!\n");

	/*------------------------------------------------
	*                  REPEATER
	*------------------------------------------------*/
	ui_header(ctx, media, "Repeater");
	ui_widget(ctx, media, 35);
	if (nk_button_label(ctx, "Press me"))
		fprintf(stdout, "pressed!\n");

	/*------------------------------------------------
	*                  TOGGLE
	*------------------------------------------------*/
	ui_header(ctx, media, "Toggle buttons");
	ui_widget(ctx, media, 35);
	if (nk_button_image_label(ctx, (toggle0) ? media->checked : media->unchecked, "Toggle", NK_TEXT_LEFT))
		toggle0 = !toggle0;

	ui_widget(ctx, media, 35);
	if (nk_button_image_label(ctx, (toggle1) ? media->checked : media->unchecked, "Toggle", NK_TEXT_LEFT))
		toggle1 = !toggle1;

	ui_widget(ctx, media, 35);
	if (nk_button_image_label(ctx, (toggle2) ? media->checked : media->unchecked, "Toggle", NK_TEXT_LEFT))
		toggle2 = !toggle2;

	/*------------------------------------------------
	*                  RADIO
	*------------------------------------------------*/
	ui_header(ctx, media, "Radio buttons");
	ui_widget(ctx, media, 35);
	if (nk_button_symbol_label(ctx, (option == 0) ? NK_SYMBOL_CIRCLE_OUTLINE : NK_SYMBOL_CIRCLE_SOLID, "Select", NK_TEXT_LEFT))
		option = 0;
	ui_widget(ctx, media, 35);
	if (nk_button_symbol_label(ctx, (option == 1) ? NK_SYMBOL_CIRCLE_OUTLINE : NK_SYMBOL_CIRCLE_SOLID, "Select", NK_TEXT_LEFT))
		option = 1;
	ui_widget(ctx, media, 35);
	if (nk_button_symbol_label(ctx, (option == 2) ? NK_SYMBOL_CIRCLE_OUTLINE : NK_SYMBOL_CIRCLE_SOLID, "Select", NK_TEXT_LEFT))
		option = 2;

	/*------------------------------------------------
	*                  CONTEXTUAL
	*------------------------------------------------*/
	nk_style_set_font(ctx, &media->font_18->handle);
	if (nk_contextual_begin(ctx, NK_WINDOW_NO_SCROLLBAR, nk_vec2(150, 300), nk_window_get_bounds(ctx))) {
		nk_layout_row_dynamic(ctx, 30, 1);
		if (nk_contextual_item_image_label(ctx, media->copy, "Clone", NK_TEXT_RIGHT))
			fprintf(stdout, "pressed clone!\n");
		if (nk_contextual_item_image_label(ctx, media->del, "Delete", NK_TEXT_RIGHT))
			fprintf(stdout, "pressed delete!\n");
		if (nk_contextual_item_image_label(ctx, media->convert, "Convert", NK_TEXT_RIGHT))
			fprintf(stdout, "pressed convert!\n");
		if (nk_contextual_item_image_label(ctx, media->edit, "Edit", NK_TEXT_RIGHT))
			fprintf(stdout, "pressed edit!\n");
		nk_contextual_end(ctx);
	}
	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);
}

/* ===============================================================
*
*                          BASIC DEMO
*
* ===============================================================*/
static void
basic_demo(struct nk_context *ctx, struct media *media)
{
	static int image_active;
	static int check0 = 1;
	static int check1 = 0;
	static size_t prog = 80;
	static int selected_item = 0;
	static int selected_image = 3;
	static int selected_icon = 0;
	static const char *items[] = { "Item 0","item 1","item 2" };
	static int piemenu_active = 0;
	static struct nk_vec2 piemenu_pos;

	int i = 0;
	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "Basic Demo", nk_rect(320, 50, 275, 610),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE);

	/*------------------------------------------------
	*                  POPUP BUTTON
	*------------------------------------------------*/
	ui_header(ctx, media, "Popup & Scrollbar & Images");
	ui_widget(ctx, media, 35);
	if (nk_button_image_label(ctx, media->dir, "Images", NK_TEXT_CENTERED))
		image_active = !image_active;

	/*------------------------------------------------
	*                  SELECTED IMAGE
	*------------------------------------------------*/
	ui_header(ctx, media, "Selected Image");
	ui_widget_centered(ctx, media, 100);
	nk_image(ctx, media->inventory[selected_image]);

	/*------------------------------------------------
	*                  IMAGE POPUP
	*------------------------------------------------*/
	if (image_active) {
		struct nk_panel popup;
		if (nk_popup_begin(ctx, NK_POPUP_STATIC, "Image Popup", 0, nk_rect(265, 0, 320, 220))) {
			nk_layout_row_static(ctx, 82, 82, 3);
			for (i = 0; i < 9; ++i) {
				if (nk_button_image(ctx, media->inventory[i])) {
					selected_image = i;
					image_active = 0;
					nk_popup_close(ctx);
				}
			}
			nk_popup_end(ctx);
		}
	}
	/*------------------------------------------------
	*                  COMBOBOX
	*------------------------------------------------*/
	ui_header(ctx, media, "Combo box");
	ui_widget(ctx, media, 40);
	if (nk_combo_begin_label(ctx, items[selected_item], nk_vec2(nk_widget_width(ctx), 200))) {
		nk_layout_row_dynamic(ctx, 35, 1);
		for (i = 0; i < 3; ++i)
			if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
				selected_item = i;
		nk_combo_end(ctx);
	}

	ui_widget(ctx, media, 40);
	if (nk_combo_begin_image_label(ctx, items[selected_icon], media->inventory[selected_icon], nk_vec2(nk_widget_width(ctx), 200))) {
		nk_layout_row_dynamic(ctx, 35, 1);
		for (i = 0; i < 3; ++i)
			if (nk_combo_item_image_label(ctx, media->inventory[i], items[i], NK_TEXT_RIGHT))
				selected_icon = i;
		nk_combo_end(ctx);
	}

	/*------------------------------------------------
	*                  CHECKBOX
	*------------------------------------------------*/
	ui_header(ctx, media, "Checkbox");
	ui_widget(ctx, media, 30);
	nk_checkbox_label(ctx, "Flag 1", &check0);
	ui_widget(ctx, media, 30);
	nk_checkbox_label(ctx, "Flag 2", &check1);

	/*------------------------------------------------
	*                  PROGRESSBAR
	*------------------------------------------------*/
	ui_header(ctx, media, "Progressbar");
	ui_widget(ctx, media, 35);
	nk_progress(ctx, &prog, 100, nk_true);

	/*------------------------------------------------
	*                  PIEMENU
	*------------------------------------------------*/
	if (nk_input_is_mouse_click_down_in_rect(&ctx->input, NK_BUTTON_RIGHT,
		nk_window_get_bounds(ctx), nk_true)) {
		piemenu_pos = ctx->input.mouse.pos;
		piemenu_active = 1;
	}

	if (piemenu_active) {
		int ret = ui_piemenu(ctx, piemenu_pos, 140, &media->menu[0], 6);
		if (ret == -2) piemenu_active = 0;
		if (ret != -1) {
			fprintf(stdout, "piemenu selected: %d\n", ret);
			piemenu_active = 0;
		}
	}
	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);
}
#endif

/* ===============================================================
*
*                        Inventory Window
*
* ===============================================================*/


typedef enum {
	QUALITY_GREY,		// on weapons/sabers/items - 0 base stats and 0 mod slots.
	QUALITY_WHITE,		// on weapons/sabers/items - 1 base stats and 0 mod slots.
	QUALITY_GREEN,		// on weapons/sabers/items - 2 base stats and 0 mod slots.
	QUALITY_BLUE,		// on weapons/sabers/items - 3 base stats and 1 mod slots.
	QUALITY_PURPLE,		// on weapons/sabers/items - 3 base stats and 2 mod slots.
	QUALITY_ORANGE,		// on weapons/sabers/items - 3 base stats and 3 mod slots.
	QUALITY_GOLD		// on weapons/sabers/items - 3 base stats and 3 mod slots. 1.5x Stats Modifier.
} itemQuality_t;

nk_color ColorForQuality(int quality)
{
	nk_color color = nk_rgba(64, 64, 64, 255);

	switch (quality)
	{
	case QUALITY_WHITE:
		color = nk_rgba(128, 128, 128, 255);
		break;
	case QUALITY_GREEN:
		color = nk_rgba(32, 128, 32, 255);
		break;
	case QUALITY_BLUE:
		color = nk_rgba(32, 32, 128, 255);
		break;
	case QUALITY_PURPLE:
		color = nk_rgba(128, 32, 192, 255);
		break;
	case QUALITY_ORANGE:
		color = nk_rgba(192, 128, 32, 255);
		break;
	case QUALITY_GOLD:
		color = nk_rgba(255, 255, 32, 255);
		break;
	default:
	case QUALITY_GREY:
		color = nk_rgba(72, 72, 72, 255);
		break;
	}

	return color;
}

const char *ColorStringForQuality(int quality)
{
	switch (quality)
	{// TODO: Not use Q3 strings so we can have more colors... The GUI lib we use probably allows for color and formatting stings anyway...
	case QUALITY_GREY:
		return "^8";
		break;
	case QUALITY_WHITE:
		return "^7";
		break;
	case QUALITY_GREEN:
		return "^2";
		break;
	case QUALITY_BLUE:
		return "^4";
		break;
	case QUALITY_PURPLE:
		return "^0";
		break;
	case QUALITY_ORANGE:
		return "^9";
		break;
	case QUALITY_GOLD:
		return "^3";
		break;
	default:
		break;
	}

	return "^5";
}

const char *itemQualityNames[] = {
	"Poor",
	"Common",
	"Uncommon",
	"Rare",
	"Epic",
	"Legendary",
	"Artifact",
};

NK_API int
uq_tooltip_begin(struct nk_context *ctx, float textWidth, float maxHeight, struct nk_rect *finalBounds)
{
	int x, y, w, h;
	struct nk_window *win;
	const struct nk_input *in;
	int ret;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	if (!ctx || !ctx->current || !ctx->current->layout)
		return 0;

	/* make sure that no nonblocking popup is currently active */
	win = ctx->current;
	in = &ctx->input;
	if (win->popup.win && (win->popup.type & NK_PANEL_SET_NONBLOCK))
		return 0;

	struct nk_rect bounds;
	w = nk_iceilf(textWidth);
	h = nk_iceilf(nk_null_rect.h);
	x = nk_ifloorf(in->mouse.pos.x + 32) - (int)win->layout->clip.x;
	y = nk_ifloorf(in->mouse.pos.y + 32) - (int)win->layout->clip.y;

	// If the tooltip would leave the screen area, move it to the top left of the mouse instead of bottom right...
	if (nk_ifloorf(ctx->input.mouse.pos.x + 32) + w > FBO_WIDTH)
	{
		x = (in->mouse.pos.x - 16) - w - (int)win->layout->clip.x;
	}

	if (nk_ifloorf(ctx->input.mouse.pos.y + 32) + nk_iceilf(maxHeight) > FBO_HEIGHT)
	{
		y = (in->mouse.pos.y - 16) - nk_iceilf(maxHeight) - (int)win->layout->clip.y;
	}

	bounds.x = (float)x;
	bounds.y = (float)y;
	bounds.w = (float)w;
	bounds.h = (float)h + (ctx->style.window.padding.y * 2.0);
	
	finalBounds->x = bounds.x;
	finalBounds->y = bounds.y;
	finalBounds->w = bounds.w;
	finalBounds->h = bounds.h;

	ret = nk_popup_begin(ctx, NK_POPUP_DYNAMIC,
		"__##Tooltip##__", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER, bounds);
	if (ret) win->layout->flags &= ~(nk_flags)NK_WINDOW_ROM;
	win->popup.type = NK_PANEL_TOOLTIP;
	ctx->current->layout->type = NK_PANEL_TOOLTIP;
	return ret;
}

NK_API void
uq_tooltip(struct nk_context *ctx, const char *text, struct media *media, struct nk_image *icon)
{
	const struct nk_style *style;
	struct nk_vec2 padding;

	NK_ASSERT(ctx);
	NK_ASSERT(ctx->current);
	NK_ASSERT(ctx->current->layout);
	NK_ASSERT(text);
	if (!ctx || !ctx->current || !ctx->current->layout || !text)
		return;

	/* fetch configuration data */
	style = &ctx->style;
	padding = style->window.padding;


	NK_ASSERT(ctx);
	NK_ASSERT(text);

	if (!ctx)
	{
		//ri->Printf(PRINT_ALL, "!ctx\n");
		return;
	}

	if (!text)
	{
		//ri->Printf(PRINT_ALL, "!text\n");
		return;
	}

	//int text_len;
	float text_width;
	float text_height;

	/* fetch configuration data */
	style = &ctx->style;
	padding = style->window.padding;

	char convertedStrings[64][256] = { 0 };

	int currentCount = 0;
	int stringCount = 0;
	int longest = 0;
	int longestCount = 0;

	int startLen = nk_strlen(text);

	//ri->Printf(PRINT_ALL, "text (count %i): %s\n", startLen, text);

	for (int i = 0; i < startLen; i++)
	{
		if (currentCount + 1 > longestCount)
		{
			longest = stringCount;
			longestCount = currentCount + 1;
		}

		if (text[i] == '\n')
		{
			convertedStrings[stringCount][currentCount] = '\0';
			stringCount++;
			currentCount = 0;
			continue;
		}

		convertedStrings[stringCount][currentCount] = text[i];
		currentCount++;
	}

	//ri->Printf(PRINT_ALL, "final lineCount: %i.\n", stringCount);
	//ri->Printf(PRINT_ALL, "longest line was %i (count %i): %s.\n", longest, longestCount, convertedStrings[longest]);

	//nk_style_set_font(ctx, &media->font_20->handle);

	float fontSize = 14.0;

	if (gui_tooltipSize->integer >= 6)
	{
		nk_style_set_font(ctx, &media->font_20->handle);
		fontSize = 20.0;
	}
	else if (gui_tooltipSize->integer >= 5)
	{
		nk_style_set_font(ctx, &media->font_19->handle);
		fontSize = 19.0;
	}
	else if (gui_tooltipSize->integer >= 4)
	{
		nk_style_set_font(ctx, &media->font_18->handle);
		fontSize = 18.0;
	}
	else if (gui_tooltipSize->integer >= 3)
	{
		nk_style_set_font(ctx, &media->font_17->handle);
		fontSize = 17.0;
	}
	else if (gui_tooltipSize->integer >= 2)
	{
		nk_style_set_font(ctx, &media->font_16->handle);
		fontSize = 16.0;
	}
	else if (gui_tooltipSize->integer >= 1)
	{
		nk_style_set_font(ctx, &media->font_15->handle);
		fontSize = 15.0;
	}
	else
	{
		fontSize = 14.0;
		nk_style_set_font(ctx, &media->font_14->handle);
	}

	text_height = 3.0f + (2.0f * padding.y);

	int texLen = nk_strlen(convertedStrings[longest]);
	int strippedLength = 0;
	char strippedText[1024] = { 0 };

	for (int i = 0; i < texLen; i++)
	{
		if (i < texLen - 1
			&& convertedStrings[longest][i] == '^'
			&& (convertedStrings[longest][i + 1] == '0' || convertedStrings[longest][i + 1] == '1' || convertedStrings[longest][i + 1] == '2' || convertedStrings[longest][i + 1] == '3'
				|| convertedStrings[longest][i + 1] == '4' || convertedStrings[longest][i + 1] == '5' || convertedStrings[longest][i + 1] == '6' || convertedStrings[longest][i + 1] == '7'
				|| convertedStrings[longest][i + 1] == '8' || convertedStrings[longest][i + 1] == '9' || convertedStrings[longest][i + 1] == '0'
				|| convertedStrings[longest][i + 1] == 'P' || convertedStrings[longest][i + 1] == 'N' || convertedStrings[longest][i + 1] == 'B' || convertedStrings[longest][i + 1] == 'b'))
		{
			i++;
			continue;
		}

		strippedText[strippedLength] = convertedStrings[longest][i];
		strippedLength++;
	}
	strippedText[strippedLength] = 0;
	strippedLength++;

	//ri->Printf(PRINT_ALL, "stripped longest line (count %i): %s.\n", strippedLength, strippedText);

	text_width = style->font->width(style->font->userdata, fontSize /** 0.77*/, strippedText, strippedLength);
	//text_width += (4.0 * padding.x);
	text_width += (2.0 * padding.x);

	int max_height = (int)(((float)stringCount * text_height) + (stringCount * (1.1f * ctx->style.window.spacing.y)));

	struct nk_rect finalBounds;

	/* execute tooltip and fill with text */
	if (uq_tooltip_begin(ctx, (float)text_width, (float)max_height, &finalBounds))
	{
		/*if (icon)
		{
			struct nk_rect iconBounds;
			iconBounds.x = finalBounds.w - 53 - 8;
			iconBounds.y = 8;
			iconBounds.w = 53;
			iconBounds.h = 53;

			nk_layout_space_push(ctx, iconBounds);
			nk_image(ctx, *icon);
		}*/

		nk_layout_row_dynamic(ctx, (float)text_height, 1);

		for (int i = 0; i < stringCount; i++)
		{
			/*if (i == 0)
			{
				if (gui_tooltipCentered->integer)
					nk_button_image_label(ctx, *icon, convertedStrings[i], NK_TEXT_CENTERED);
				else
					nk_button_image_label(ctx, *icon, convertedStrings[i], NK_TEXT_LEFT);
			}
			else*/
			{
				if (gui_tooltipCentered->integer)
					nk_text(ctx, convertedStrings[i], nk_strlen(convertedStrings[i]), NK_TEXT_CENTERED);
				else
					nk_text(ctx, convertedStrings[i], nk_strlen(convertedStrings[i]), NK_TEXT_LEFT);
			}
		}

		nk_tooltip_end(ctx);
	}
}

static void
uq_header(struct nk_context *ctx, struct media *media, const char *title)
{
	nk_style_set_font(ctx, &media->font_18->handle);
	nk_layout_row_dynamic(ctx, 20, 1);
	nk_label(ctx, title, NK_TEXT_LEFT);
}

static void
uq_widget(struct nk_context *ctx, struct media *media, float height, int columns)
{
	static const float ratio[] = { 0.15f, 0.85f };
	nk_style_set_font(ctx, &media->font_22->handle);
	nk_layout_row(ctx, NK_DYNAMIC, height, 2 + columns, ratio);
	nk_spacing(ctx, 1 + columns);
}

static void
uq_widget_centered(struct nk_context *ctx, struct media *media, float height)
{
	static const float ratio[] = { 0.15f, 0.50f, 0.35f };
	nk_style_set_font(ctx, &media->font_22->handle);
	nk_layout_row(ctx, NK_DYNAMIC, height, 3, ratio);
	nk_spacing(ctx, 1);
}

qboolean uq_checkbox(struct nk_context *ctx, struct media *media, char *label, qboolean currentSetting)
{
	bool checked = (currentSetting == qtrue);

	/*------------------------------------------------
	*                  TOGGLE
	*------------------------------------------------*/

	nk_style_set_font(ctx, &media->font_14->handle);

	uq_widget(ctx, media, 35, 0);
	//uq_header(ctx, media, label);

	if (nk_button_image_label(ctx, (checked) ? media->checked : media->unchecked, label, NK_TEXT_LEFT))
		checked = !checked;

	return (qboolean)checked;
}

int uq_radio(struct nk_context *ctx, struct media *media, char *label, int currentSetting, int maxSetting)
{
	int setting = currentSetting;

	/*------------------------------------------------
	*                  RADIO
	*------------------------------------------------*/

#if 0
	nk_style_set_font(ctx, &media->font_14->handle);

	uq_widget(ctx, media, 35, maxSetting);

	uq_header(ctx, media, label);

	for (int i = 0; i <= maxSetting; i++)
	{
		int selected = nk_button_symbol_label(ctx, (setting == i) ? NK_SYMBOL_CIRCLE_SOLID : NK_SYMBOL_CIRCLE_OUTLINE, va("%i", i), NK_TEXT_LEFT);
		
		if (selected)
			setting = i;
	}
#else
	//nk_layout_row_dynamic(ctx, 30, maxSetting+2);
	nk_layout_row_dynamic(ctx, 30, 2);
	nk_label(ctx, label, NK_TEXT_LEFT);
	
	nk_layout_row_dynamic(ctx, 30, maxSetting + 1);
	for (int i = 0; i <= maxSetting; i++)
	{
		if (nk_option_label(ctx, va("%i", i), (currentSetting == i) ? 1 : 0))
		{
			setting = i;
		}
	}
#endif

	return setting;
}

//
//
//
//
//
typedef enum nuklearMenus_s {
	MENU_NONE,
	MENU_ABILITIES,
	MENU_CHARACTER,
	MENU_INVENTORY,
	MENU_POWERS,
	MENU_RADIO,
	MENU_SETTINGS,
	MENU_MAX
} nuklearMenus_t;

typedef struct nuklearWindow_s
{
	nuklearMenus_t		id;
	int					flag;
	char				windowName[64];
	char				tooltip[1024];
} nuklearWindow_t;

nuklearWindow_t nuklearWindows[] = {
//  MENU ID,			FLAG,	WINDOW NAME,	TOOLTIP TEXT,
	MENU_NONE,			0,		"None",			"",
	MENU_ABILITIES,		1,		"Abilities",	"^8Open the ^3abilities ^8menu.\n\nHere you can select your player's abilities and\nupgrades.\n",
	MENU_CHARACTER,		2,		"Character",	"^8Open the ^3character ^8menu.\n\nHere you can see your character's stats, and equip\nitems and abilities.\n",
	MENU_INVENTORY,		4,		"Inventory",	"^8Open the ^3inventory ^8menu.\n\nThis is all your player's current items. They can\nbe dragged to the quickbar or to the character screen.\n",
	MENU_POWERS,		8,		"Powers",		"^8Open the ^3powers ^8menu.\n\nThis is all your player's current skills. They can\nbe dragged to the quickbar or to the character screen.\n",
	MENU_RADIO,			16,		"Radio",		"^8Open the ^3radio ^8menu.\n\nThis is your holonet radio uplink. You can select a\nradio station here to listen to from a list of available\nstations on this world.\n",
	MENU_SETTINGS,		32,		"Settings",		"^8Open the ^3settings ^8menu.\n\nThis is the game settings window. You can\nconfigure game settings here.\n",
	MENU_MAX,			0,		"Max",			"",
};


bool	mainMenuOpen		= false;
int		menuNoChangeTime	= 0; // To make sure we dont instantly close a window when openning (next frame)...
int		windowsOpen			= MENU_NONE;

qboolean GUI_IsWindowOpen(struct nk_context *ctx, nuklearMenus_t menu)
{
	if (menu <= MENU_NONE || menu >= MENU_MAX) return qfalse;

	if (windowsOpen & nuklearWindows[menu].flag)
		return qtrue;

	return qfalse;
}

void GUI_OpenWindow(struct nk_context *ctx, nuklearMenus_t menu)
{
	if (menu <= MENU_NONE || menu >= MENU_MAX) return;

	if (!(windowsOpen & nuklearWindows[menu].flag))
	{
		windowsOpen |= nuklearWindows[menu].flag;

		nk_window_set_focus(ctx, nuklearWindows[menu].windowName);
	}
}

void GUI_CloseWindow(struct nk_context *ctx, nuklearMenus_t menu)
{
	if (menu <= MENU_NONE || menu >= MENU_MAX) return;

	if (windowsOpen & nuklearWindows[menu].flag)
	{
		windowsOpen &= ~nuklearWindows[menu].flag;
	}

	nk_window_show(ctx, nuklearWindows[menu].windowName, NK_HIDDEN);

	for (int i = MENU_NONE + 1; i < MENU_MAX; i++)
	{// Make sure another window has focus, if there is one...
		nk_window *win = nk_window_find(ctx, nuklearWindows[i].windowName);

		if (win && GUI_IsWindowOpen(ctx, (nuklearMenus_t)i))
		{
			nk_window_set_focus(ctx, nuklearWindows[menu].windowName);
			break;
		}
	}
}

void GUI_CloseAllWindows(struct nk_context *ctx)
{
	for (int i = MENU_NONE + 1; i < MENU_MAX; i++)
	{
		GUI_CloseWindow(ctx, (nuklearMenus_t)i);
	}

	// Probably could just do this, but just in case...
	windowsOpen = MENU_NONE;
}

void GUI_ToggleWindow(struct nk_context *ctx, nuklearMenus_t menu)
{
	// To make sure we dont instantly close a window when openning (next frame)...
	if (menuNoChangeTime > backEnd.refdef.time)
	{
		return;
	}

	menuNoChangeTime = backEnd.refdef.time + 100;

	if (!GUI_IsWindowOpen(ctx, menu))
		GUI_OpenWindow(ctx, menu);
	else
		GUI_CloseWindow(ctx, menu);

	mainMenuOpen = false;
}

void GUI_UpdateClosedWindows(struct nk_context *ctx)
{// Sync our window management with nuklear's complete mess...
	for (int i = MENU_NONE + 1; i < MENU_MAX; i++)
	{
		if (!GUI_IsWindowOpen(ctx, (nuklearMenus_t)i))
			continue;

		nk_window *win = nk_window_find(ctx, nuklearWindows[i].windowName);

		if (win && nk_window_is_hidden(ctx, nuklearWindows[i].windowName))
		{
			GUI_CloseWindow(ctx, (nuklearMenus_t)i);
		}
	}
}

void GUI_CheckOpenWindows(struct nk_context *ctx)
{
	GUI_UpdateClosedWindows(ctx);

	if (keyStatus[A_LOW_A] || keyStatus[A_CAP_A])
	{// Open/Close Abilities Window...
		GUI_ToggleWindow(ctx, MENU_ABILITIES);
		ri->Printf(PRINT_WARNING, "Abilities Window is currently unimplemented.\n");
	}

	if (keyStatus[A_LOW_C] || keyStatus[A_CAP_C])
	{// Open/Close Character Window...
		GUI_ToggleWindow(ctx, MENU_CHARACTER);
		ri->Printf(PRINT_WARNING, "Character Window is currently unimplemented.\n");
	}

	if (keyStatus[A_LOW_I] || keyStatus[A_CAP_I])
	{// Open/Close Inventory Window...
		GUI_ToggleWindow(ctx, MENU_INVENTORY);
	}

	if (keyStatus[A_LOW_P] || keyStatus[A_CAP_P])
	{// Open/Close Powers Window...
		GUI_ToggleWindow(ctx, MENU_POWERS);
		ri->Printf(PRINT_WARNING, "Powers Window is currently unimplemented.\n");
	}

	if (keyStatus[A_LOW_R] || keyStatus[A_CAP_R])
	{// Open/Close Radio Window...
		GUI_ToggleWindow(ctx, MENU_RADIO);
	}

	if (keyStatus[A_LOW_S] || keyStatus[A_CAP_S])
	{// Open/Close Settings Window...
		GUI_ToggleWindow(ctx, MENU_SETTINGS);
		ri->Printf(PRINT_WARNING, "Settings Window is currently unimplemented.\n");
	}
}

static void
GUI_MenuHoverTooltip(struct nk_context *ctx, struct media *media, char *tooltipText, struct nk_image icon)
{
	if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER
		&& backEnd.ui_MouseCursor <= 0)
	{// Hoverred...
		uq_tooltip(ctx, tooltipText, media, &icon);
		nk_style_set_font(ctx, &media->font_20->handle);
	}
}

static void
GUI_MainMenu(struct nk_context *ctx, struct media *media)
{
	if (gui_useMenu->integer)
	{
		float size[2];
		size[0] = 40.0;
		size[1] = 40.0;

		int i = 0;
		nk_style_set_font(ctx, &media->font_20->handle);
		struct nk_rect rect = nk_rect(((float)FBO_WIDTH - size[0]) - 4.0, ((float)FBO_HEIGHT - size[1]) - 2.0, size[0], size[1]);

		nk_color origbackground = ctx->style.window.background;
		nk_color origtextbackground = ctx->style.window.background;
		ctx->style.window.background = nk_rgba(0, 0, 0, 0); // Invisible background...
		ctx->style.button.text_background = nk_rgba(0, 0, 0, 0); // Invisible background...
		nk_begin(ctx, "MainMenu", rect, NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND | NK_WINDOW_DYNAMIC);

		/*------------------------------------------------
		*                MAIN MENU DISPLAY
		*------------------------------------------------*/
		nk_style_set_font(ctx, &media->font_14->handle);
		nk_layout_row_static(ctx, 24, 24, 1);

		int ret = nk_button_image_label(ctx, media->iconMainMenu, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);

		nk_style_set_font(ctx, &media->font_14->handle);
		nk_end(ctx);
		ctx->style.window.background = origbackground;
		ctx->style.button.text_background = origtextbackground;

		if (ret)
		{
			mainMenuOpen = true;
		}

		if (mainMenuOpen)
		{
			int		NUM_MENU_ITEMS = MENU_MAX + 1; // only +1 and not +2 because MENU_MAX is +1 of actual last entry...
			float	cWidth = 140.0;
			float	cHeight = NUM_MENU_ITEMS * 24.0 + 6.0;

			struct nk_rect bounds2;
			bounds2.x = (rect.x + size[0]) - cWidth;
			bounds2.y = rect.y - cHeight;
			bounds2.w = cWidth;
			bounds2.h = cHeight;

			nk_begin(ctx, "Context", bounds2, NK_WINDOW_NO_SCROLLBAR);

			int ret2 = nk_contextual_begin(ctx, NK_WINDOW_NO_SCROLLBAR, nk_vec2(bounds2.w, bounds2.h), rect);

			/* settings */
			nk_style_set_font(ctx, &media->font_20->handle);
			nk_layout_row_dynamic(ctx, 20, 1);

			//
			// Main Menu Items...
			//
			nk_contextual_item_label(ctx, "^7Warzone Menu", NK_TEXT_CENTERED);
			nk_contextual_item_label(ctx, "^8---------------------------------------------", NK_TEXT_CENTERED);

			int abilities = nk_contextual_item_image_label(ctx, media->iconAbilities, "^8^BA^b^8bilities", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[1].tooltip, media->iconAbilities);

			int character = nk_contextual_item_image_label(ctx, media->iconCharacter, "^8^BC^b^8haracter", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[2].tooltip, media->iconCharacter);

			int inventory = nk_contextual_item_image_label(ctx, media->iconInventory, "^7^BI^b^7nventory", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[3].tooltip, media->iconInventory);

			int powers = nk_contextual_item_image_label(ctx, media->iconPowers, "^8^BP^b^8owers", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[4].tooltip, media->iconPowers);

			int radio = nk_contextual_item_image_label(ctx, media->iconRadio, "^7^BR^b^7adio", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[5].tooltip, media->iconRadio);

			int settings = nk_contextual_item_image_label(ctx, media->iconSettings, "^8^BS^b^8ettings", NK_TEXT_LEFT);
			GUI_MenuHoverTooltip(ctx, media, nuklearWindows[6].tooltip, media->iconSettings);

			nk_contextual_end(ctx);

			if (abilities)
			{// For perk tree...
				GUI_ToggleWindow(ctx, MENU_ABILITIES);
				ri->Printf(PRINT_WARNING, "Abilities Window is currently unimplemented.\n");
			}
			if (character)
			{
				GUI_ToggleWindow(ctx, MENU_CHARACTER);
				ri->Printf(PRINT_WARNING, "Character Window is currently unimplemented.\n");
			}
			if (inventory)
			{
				GUI_ToggleWindow(ctx, MENU_INVENTORY);
			}
			if (powers)
			{// For spell book...
				GUI_ToggleWindow(ctx, MENU_POWERS);
				ri->Printf(PRINT_WARNING, "Powers Window is currently unimplemented.\n");
			}
			if (radio)
			{
				GUI_ToggleWindow(ctx, MENU_RADIO);
			}
			if (settings)
			{
				GUI_ToggleWindow(ctx, MENU_SETTINGS);
				ri->Printf(PRINT_WARNING, "Settings Window is currently unimplemented.\n");
			}

			nk_style_set_font(ctx, &media->font_14->handle);

			nk_end(ctx);

			return;
		}
	}
	else
	{
		int		NUM_MENU_ITEMS = MENU_MAX - 1;

		float itemSize = 34.0;
		float size[2];
		size[0] = NUM_MENU_ITEMS * itemSize + 4.0;
		size[1] = itemSize;
		float iconSize = itemSize - 6.0;

		int i = 0;
		nk_style_set_font(ctx, &media->font_20->handle);
		struct nk_rect rect = nk_rect(((float)FBO_WIDTH - size[0]) - 4.0, ((float)FBO_HEIGHT - size[1]) - 2.0, size[0], size[1]);
		nk_begin(ctx, "MainMenuBar", rect, NK_WINDOW_MOVABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND | NK_WINDOW_DYNAMIC);

		/*------------------------------------------------
		*                QUICK BAR DISPLAY
		*------------------------------------------------*/
		nk_style_set_font(ctx, &media->font_14->handle);
		nk_layout_row_static(ctx, iconSize, iconSize, NUM_MENU_ITEMS);

		//ret = nk_button_image_label(ctx, media->inventory[quickBarSelections[i]], label, NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		int abilities = nk_button_image_label(ctx, media->iconAbilities, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[1].tooltip, media->iconAbilities);

		int character = nk_button_image_label(ctx, media->iconCharacter, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[2].tooltip, media->iconCharacter);

		int inventory = nk_button_image_label(ctx, media->iconInventory, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[3].tooltip, media->iconInventory);

		int powers = nk_button_image_label(ctx, media->iconPowers, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[4].tooltip, media->iconPowers);

		int radio = nk_button_image_label(ctx, media->iconRadio, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[5].tooltip, media->iconRadio);

		int settings = nk_button_image_label(ctx, media->iconSettings, "", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_TOP);
		GUI_MenuHoverTooltip(ctx, media, nuklearWindows[6].tooltip, media->iconSettings);

		if (abilities)
		{// For perk tree...
			GUI_ToggleWindow(ctx, MENU_ABILITIES);
			ri->Printf(PRINT_WARNING, "Abilities Window is currently unimplemented.\n");
		}
		if (character)
		{
			GUI_ToggleWindow(ctx, MENU_CHARACTER);
			ri->Printf(PRINT_WARNING, "Character Window is currently unimplemented.\n");
		}
		if (inventory)
		{
			GUI_ToggleWindow(ctx, MENU_INVENTORY);
		}
		if (powers)
		{// For spell book...
			GUI_ToggleWindow(ctx, MENU_POWERS);
			ri->Printf(PRINT_WARNING, "Powers Window is currently unimplemented.\n");
		}
		if (radio)
		{
			GUI_ToggleWindow(ctx, MENU_RADIO);
		}
		if (settings)
		{
			GUI_ToggleWindow(ctx, MENU_SETTINGS);
			ri->Printf(PRINT_WARNING, "Settings Window is currently unimplemented.\n");
		}

		nk_style_set_font(ctx, &media->font_14->handle);
		nk_end(ctx);
	}
}

//
// Post Process Settings...
//

int			GUI_PostProcessNumCvars = 0;
cvar_t		*GUI_PostProcessCvars[128] = { 0 };
int			GUI_PostProcessMax[128] = { 1 };
int			GUI_PostProcessValue[128] = { 1 };

void GUI_PostProcessClearFrame() {
	GUI_PostProcessNumCvars = 0;
}

void GUI_PostProcessAddCvar(cvar_t *cvar, int max) {
	GUI_PostProcessCvars[GUI_PostProcessNumCvars] = cvar;
	GUI_PostProcessMax[GUI_PostProcessNumCvars] = max;
	GUI_PostProcessValue[GUI_PostProcessNumCvars] = cvar->integer;
	GUI_PostProcessNumCvars++;
}

void GUI_PostProcessUpdateCvars() {
	for (int i = 0; i < GUI_PostProcessNumCvars; i++)
	{
		if (GUI_PostProcessValue[i] > GUI_PostProcessMax[i])
			GUI_PostProcessValue[i] = GUI_PostProcessMax[i];

		if (GUI_PostProcessValue[i] != GUI_PostProcessCvars[i]->integer)
		{
			ri->Cvar_Set(GUI_PostProcessCvars[i]->name, va("%i", GUI_PostProcessValue[i]));
		}
	}
}

int GUI_PostProcessUpdateUI(struct nk_context *ctx, struct media *media) {
	int hovered = -1;

	//nk_layout_row_dynamic(ctx, 35, 1);

	for (int i = 0; i < GUI_PostProcessNumCvars; i++)
	{
#if 0
		if (GUI_PostProcessMax[i] == 1)
		{
			if (GUI_PostProcessCvars[i]->displayInfoSet && GUI_PostProcessCvars[i]->displayName && GUI_PostProcessCvars[i]->displayName[0])
			{
				GUI_PostProcessValue[i] = uq_checkbox(ctx, media, GUI_PostProcessCvars[i]->displayName, (qboolean)GUI_PostProcessValue[i]);
			}
			else
			{
				GUI_PostProcessValue[i] = uq_checkbox(ctx, media, GUI_PostProcessCvars[i]->name, (qboolean)GUI_PostProcessValue[i]);
			}
		}
		else
		{
			if (GUI_PostProcessCvars[i]->displayInfoSet && GUI_PostProcessCvars[i]->displayName && GUI_PostProcessCvars[i]->displayName[0])
			{
				GUI_PostProcessValue[i] = uq_radio(ctx, media, GUI_PostProcessCvars[i]->displayName, (qboolean)GUI_PostProcessValue[i], GUI_PostProcessMax[i]);
			}
			else
			{
				GUI_PostProcessValue[i] = uq_radio(ctx, media, GUI_PostProcessCvars[i]->name, (qboolean)GUI_PostProcessValue[i], GUI_PostProcessMax[i]);
			}
		}
#else
		if (GUI_PostProcessCvars[i]->displayInfoSet && GUI_PostProcessCvars[i]->displayName && GUI_PostProcessCvars[i]->displayName[0])
		{
			GUI_PostProcessValue[i] = uq_radio(ctx, media, GUI_PostProcessCvars[i]->displayName, (qboolean)GUI_PostProcessValue[i], GUI_PostProcessMax[i]);
		}
		else
		{
			GUI_PostProcessValue[i] = uq_radio(ctx, media, GUI_PostProcessCvars[i]->name, (qboolean)GUI_PostProcessValue[i], GUI_PostProcessMax[i]);
		}
#endif

		if ((ctx->last_widget_state & NK_WIDGET_STATE_HOVER))
		{// Hoverred...
			hovered = i;
		}
	}

	return hovered;
}

void GUI_PostProcessMakeCvarList() {
	GUI_PostProcessAddCvar(r_shadowBlur, 1);
	GUI_PostProcessAddCvar(r_dynamicGlow, 1);
	GUI_PostProcessAddCvar(r_bloom, 2);
	GUI_PostProcessAddCvar(r_anamorphic, 1);
	GUI_PostProcessAddCvar(r_ssdm, 2);
	GUI_PostProcessAddCvar(r_ao, 3);
	GUI_PostProcessAddCvar(r_cartoon, 3);
	//GUI_PostProcessAddCvar(r_ssdo, 1);
	//GUI_PostProcessAddCvar(r_sss, 1);
	GUI_PostProcessAddCvar(r_deferredLighting, 1);
	//GUI_PostProcessAddCvar(r_ssr, 1);
	//GUI_PostProcessAddCvar(r_sse, 1);
	GUI_PostProcessAddCvar(r_magicdetail, 1);
	//GUI_PostProcessAddCvar(r_hbao, 1);
	GUI_PostProcessAddCvar(r_glslWater, 5);
	GUI_PostProcessAddCvar(r_fogPost, 1);
	GUI_PostProcessAddCvar(r_multipost, 1);
	GUI_PostProcessAddCvar(r_dof, 3);
	GUI_PostProcessAddCvar(r_lensflare, 1);
	GUI_PostProcessAddCvar(r_testshader, 1);
	GUI_PostProcessAddCvar(r_colorCorrection, 1);
	GUI_PostProcessAddCvar(r_esharpening, 1);
	//GUI_PostProcessAddCvar(r_esharpening2, 1);
	GUI_PostProcessAddCvar(r_darkexpand, 1);
	GUI_PostProcessAddCvar(r_distanceBlur, 5);
	GUI_PostProcessAddCvar(r_volumeLight, 1);
	GUI_PostProcessAddCvar(r_cloudshadows, 2);
	GUI_PostProcessAddCvar(r_fxaa, 1);
	GUI_PostProcessAddCvar(r_txaa, 1);
	GUI_PostProcessAddCvar(r_showdepth, 1);
	GUI_PostProcessAddCvar(r_shownormals, 4);
	GUI_PostProcessAddCvar(r_trueAnaglyph, 2);
	GUI_PostProcessAddCvar(r_occlusion, 1);
}

static void
GUI_Settings(struct nk_context *ctx, struct media *media)
{
	float size[2] = { 640.0, 480.0 };

	int i = 0;
	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "Settings - Post Processing", nk_rect(128.0, 128.0, size[0], size[1]), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE);

	/*------------------------------------------------
	*           POSTPROCESS SETTINGS DISPLAY
	*------------------------------------------------*/
	GUI_PostProcessClearFrame();
	GUI_PostProcessMakeCvarList();
	int hovered = GUI_PostProcessUpdateUI(ctx, media);
	if (hovered >= 0)
	{// Something is hovered, do the tooltip...
		if (GUI_PostProcessCvars[hovered]->displayInfoSet && GUI_PostProcessCvars[hovered]->description && GUI_PostProcessCvars[hovered]->description[0])
		{
			uq_tooltip(ctx, GUI_PostProcessCvars[hovered]->description, media, NULL);
		}
	}

	GUI_PostProcessUpdateCvars();

	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);
}

extern char currentMapName[128];

static struct nk_image icon_load(const char *filename); // below

//
// Radio UI...
//
qboolean			RADIO_CUSTOM_STATION_INITIALIZED = qfalse;
char				RADIO_CUSTOM_STATION_NAME[512] = { 0 };
char				RADIO_CUSTOM_STATION_ICON[512] = { 0 };
char				RADIO_CUSTOM_STATION_MAPNAME[128] = { 0 };

qboolean			RADIO_ICONS_INITIALIZED = qfalse;
struct nk_image		RADIO_ICONS[7];

void GUI_Radio_IniticonRadios(void)
{
	if (!RADIO_ICONS_INITIALIZED)
	{
		RADIO_ICONS[0] = icon_load("Warzone/gui/images/radioStations/galacticRadio.png");
		RADIO_ICONS[1] = icon_load("Warzone/gui/images/radioStations/mindWormRadio.png");
		RADIO_ICONS[2] = icon_load("Warzone/gui/images/radioStations/relaxingRadio.png");
		RADIO_ICONS[3] = icon_load("Warzone/gui/images/radioStations/mapRadio.png");
		RADIO_ICONS[4] = icon_load("Warzone/gui/images/radioStations/customRadio.png");
		RADIO_ICONS[5] = icon_load("Warzone/gui/images/radioStations/radioOff.png");

		RADIO_ICONS[6] = RADIO_ICONS[3]; // Backup of icon to save reloading later...
	}

	RADIO_ICONS_INITIALIZED = qtrue;
}

void GUI_Radio_InitCustomStations(void)
{
	GUI_Radio_IniticonRadios();

	if (strcmp(RADIO_CUSTOM_STATION_MAPNAME, currentMapName))
	{// Mapname has changed... Init...
		memset(RADIO_CUSTOM_STATION_NAME, 0, sizeof(RADIO_CUSTOM_STATION_NAME));
		memset(RADIO_CUSTOM_STATION_MAPNAME, 0, sizeof(RADIO_CUSTOM_STATION_MAPNAME));
		memset(RADIO_CUSTOM_STATION_ICON, 0, sizeof(RADIO_CUSTOM_STATION_MAPNAME));
		strcpy(RADIO_CUSTOM_STATION_MAPNAME, currentMapName);
		RADIO_CUSTOM_STATION_INITIALIZED = qfalse;
	}

	if (RADIO_CUSTOM_STATION_INITIALIZED) return;

	char radioIniName[512] = { 0 };
	char radioMapStrippedName[64] = { 0 };
	COM_StripExtension(RADIO_CUSTOM_STATION_MAPNAME, radioMapStrippedName, sizeof(radioMapStrippedName));
	sprintf(radioIniName, "maps/%s.radio", radioMapStrippedName);
	strcpy(RADIO_CUSTOM_STATION_NAME, IniRead(radioIniName, "STATION", "NAME", ""));
	strcpy(RADIO_CUSTOM_STATION_ICON, IniRead(radioIniName, "STATION", "ICON", ""));

	if (RADIO_CUSTOM_STATION_ICON[0] != 0)
	{
		char icon[512] = { 0 };
		sprintf(icon, "Warzone/gui/images/radioStations/%s.png", RADIO_CUSTOM_STATION_ICON);
		RADIO_ICONS[3] = icon_load(icon);
	}
	else
		RADIO_ICONS[3] = RADIO_ICONS[6];

	RADIO_CUSTOM_STATION_INITIALIZED = qtrue;
}

static int radio_piemenu_active = 0;
static struct nk_vec2 radio_piemenu_pos;

static void
GUI_Radio(struct nk_context *ctx, struct media *media)
{
	GUI_Radio_InitCustomStations();

	float size[2] = { (strlen(RADIO_CUSTOM_STATION_NAME) > 0) ? 616.0f : 516.0f, 170.0f };
	char tooltipString[512] = { 0 };
	memset(tooltipString, 0, sizeof(tooltipString));

	int i = 0;
	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "Radio", nk_rect(64.0, ((float)FBO_HEIGHT - size[1]) - 64.0, size[0], size[1]), NK_WINDOW_CLOSABLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR);


	// Icon...
	nk_window *win = nk_window_find(ctx, "Radio");
	win->hasIcon = true;
	win->icon = media->iconRadio;



	if (strlen(RADIO_CUSTOM_STATION_NAME) > 0)
		nk_layout_row_static(ctx, 96, 96, 6);
	else
		nk_layout_row_static(ctx, 96, 96, 5);


	//
	nk_color bgColor = nk_rgba(64, 64, 64, 255);
	ctx->style.button.border_color = bgColor;
	ctx->style.button.border = 4.0;
	ctx->style.button.image_padding = nk_vec2(-1.0, -1.0);
	ctx->style.button.rounding = 4.0;
	
	int currentRadioSelection = ri->Cvar_VariableValue("s_musicSelection");

	if (currentRadioSelection == 0)
	{
		bgColor = ColorForQuality(QUALITY_BLUE);
		ctx->style.button.border_color = bgColor;
	}
	else
	{
		bgColor = nk_rgba(64, 64, 64, 255);
		ctx->style.button.border_color = bgColor;
	}

	if (nk_button_image_label(ctx, RADIO_ICONS[0], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
		ri->Cvar_Set("s_musicSelection", "0");

	if (currentRadioSelection == 1)
	{
		bgColor = ColorForQuality(QUALITY_BLUE);
		ctx->style.button.border_color = bgColor;
	}
	else
	{
		bgColor = nk_rgba(64, 64, 64, 255);
		ctx->style.button.border_color = bgColor;
	}

	if (nk_button_image_label(ctx, RADIO_ICONS[1], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
		ri->Cvar_Set("s_musicSelection", "1");

	if (currentRadioSelection == 2)
	{
		bgColor = ColorForQuality(QUALITY_BLUE);
		ctx->style.button.border_color = bgColor;
	}
	else
	{
		bgColor = nk_rgba(64, 64, 64, 255);
		ctx->style.button.border_color = bgColor;
	}

	if (nk_button_image_label(ctx, RADIO_ICONS[2], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
		ri->Cvar_Set("s_musicSelection", "2");


	
	if (strlen(RADIO_CUSTOM_STATION_NAME) > 0)
	{
		if (currentRadioSelection == 3)
		{
			bgColor = ColorForQuality(QUALITY_BLUE);
			ctx->style.button.border_color = bgColor;
		}
		else
		{
			bgColor = nk_rgba(64, 64, 64, 255);
			ctx->style.button.border_color = bgColor;
		}

		if (nk_button_image_label(ctx, RADIO_ICONS[3], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
			ri->Cvar_Set("s_musicSelection", "3");


		if (currentRadioSelection == 4)
		{
			bgColor = ColorForQuality(QUALITY_BLUE);
			ctx->style.button.border_color = bgColor;
		}
		else
		{
			bgColor = nk_rgba(64, 64, 64, 255);
			ctx->style.button.border_color = bgColor;
		}

		if (nk_button_image_label(ctx, RADIO_ICONS[4], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
			ri->Cvar_Set("s_musicSelection", "4");


		if (currentRadioSelection == 5)
		{
			bgColor = ColorForQuality(QUALITY_BLUE);
			ctx->style.button.border_color = bgColor;
		}
		else
		{
			bgColor = nk_rgba(64, 64, 64, 255);
			ctx->style.button.border_color = bgColor;
		}

		if (nk_button_image_label(ctx, RADIO_ICONS[5], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
			ri->Cvar_Set("s_musicSelection", "5");
	}
	else
	{
		if (currentRadioSelection == 4)
		{
			bgColor = ColorForQuality(QUALITY_BLUE);
			ctx->style.button.border_color = bgColor;
		}
		else
		{
			bgColor = nk_rgba(64, 64, 64, 255);
			ctx->style.button.border_color = bgColor;
		}

		if (nk_button_image_label(ctx, RADIO_ICONS[4], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
			ri->Cvar_Set("s_musicSelection", "4");


		if (currentRadioSelection == 5)
		{
			bgColor = ColorForQuality(QUALITY_BLUE);
			ctx->style.button.border_color = bgColor;
		}
		else
		{
			bgColor = nk_rgba(64, 64, 64, 255);
			ctx->style.button.border_color = bgColor;
		}

		if (nk_button_image_label(ctx, RADIO_ICONS[5], "", NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED))
			ri->Cvar_Set("s_musicSelection", "5");
	}




	if (strlen(RADIO_CUSTOM_STATION_NAME) > 0)
		nk_layout_row_static(ctx, 28, 96, 6);
	else
		nk_layout_row_static(ctx, 28, 96, 5);

	nk_style_set_font(ctx, &media->font_14->handle);
	nk_text(ctx, "Galactic Radio", strlen("Galactic Radio"), NK_TEXT_CENTERED);
	nk_text(ctx, "Mind Worm Radio", strlen("Mind Worm Radio"), NK_TEXT_CENTERED);
	nk_text(ctx, "Relaxing In The Rim", strlen("Relaxing In The Rim"), NK_TEXT_CENTERED);
	if (strlen(RADIO_CUSTOM_STATION_NAME) > 0)
	{
		nk_text(ctx, RADIO_CUSTOM_STATION_NAME, strlen(RADIO_CUSTOM_STATION_NAME), NK_TEXT_CENTERED);
		nk_text(ctx,  "Custom Radio", strlen("Custom Radio"), NK_TEXT_CENTERED);
		nk_text(ctx, "Radio Off", strlen("Radio Off"), NK_TEXT_CENTERED);
	}
	else
	{
		nk_text(ctx, "Custom Radio", strlen("Custom Radio"), NK_TEXT_CENTERED);
		nk_text(ctx, "Radio Off", strlen("Radio Off"), NK_TEXT_CENTERED);
	}
	//

	if (strlen(RADIO_CUSTOM_STATION_NAME) > 0)
		nk_layout_row_static(ctx, 28, 96, 6);
	else
		nk_layout_row_static(ctx, 28, 96, 5);

	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);
}

qboolean	quickBarInitialized = qfalse;
int			quickBarSelections[12] = { { -1 } };

static void 
GUI_InitQuickBar(void)
{
	if (!quickBarInitialized)
	{
		for (int i = 0; i < 12; i++)
		{
			quickBarSelections[i] = -1;
		}

		quickBarInitialized = qtrue;
	}
}

static void
GUI_QuickBar(struct nk_context *ctx, struct media *media)
{
	GUI_InitQuickBar();

	float size[2];
	size[0] = 550.0;// 552.0;
	size[1] = 53.0;

	int i = 0;
	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "QuickBar", nk_rect(((float)(FBO_WIDTH / 2.0) - (size[0] / 2.0)) /*- 8.0*/, ((float)FBO_HEIGHT - size[1]) - 2.0, size[0], size[1]), NK_WINDOW_MOVABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND | NK_WINDOW_DYNAMIC);

	/*------------------------------------------------
	*                QUICK BAR DISPLAY
	*------------------------------------------------*/
	nk_style_set_font(ctx, &media->font_14->handle);
	nk_layout_row_static(ctx, 41, 41, 12);

	for (i = 0; i < 12; ++i)
	{
		char label[4] = { 0 };

		/*if (i < 9)
			strcpy(label, va("^7%i", i + 1));
		else if (i == 9)
			strcpy(label, "^70");
		else if (i == 10)
			strcpy(label, "^7-");
		else if (i == 11)
			strcpy(label, "^7=");*/

		int ret = 0;

		if (quickBarSelections[i] >= 0)
		{
			int quality = QUALITY_GOLD - Q_clamp(QUALITY_GREY, quickBarSelections[i] / QUALITY_GOLD, QUALITY_GOLD);
			//nk_color bgColor = ColorForQuality(quality);
			nk_color bgColor = nk_rgba(0, 0, 0, 0);
			ctx->style.button.border_color = bgColor;
			ctx->style.button.border = 4.0;
			ctx->style.button.image_padding = nk_vec2(-3.0, -2.0);
			ctx->style.button.rounding = 4.0;
			ret = nk_button_image_label(ctx, media->inventory[quickBarSelections[i]], label, NK_TEXT_CENTERED);

			if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER
				&& backEnd.ui_MouseCursor <= 0)
			{// Hoverred...
				char tooltipString[1024] = { 0 };

				sprintf(tooltipString, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
					, va("%s^BMy schwartz is bigger than yours (%s)^b\n", ColorStringForQuality(quality), itemQualityNames[quality])
					, "^POne handed weapon, Lightsaber\n"
					, " \n"
					, "^7Scaling Attribute: ^PStrength\n"
					, "^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n"
					, "^7Attacks per Second: ^P0.45\n"
					, "^7Crit Chance: ^P+11.5%\n"
					, "^7Crit Power: ^P+41.0%\n"
					, " \n"
					, "^0Purple Crystal: ^P+12.0% ^4electric^0, and ^P+12.0% ^Nheat ^0damage.\n"
					, "^N-42.0% ^7Dexterity.\n"
					, "^N-97.0% ^7Intelligence.\n"
					, "^N+33.0% ^7Weight.\n"
					, " \n"
					, "^P+15.0% ^2bonus to trip over your own feet.\n"
					, "^P+50.0% ^2bonus to asking dumb questions.\n"
					, "^P+20.0% ^2bonus to epeen trolling.\n"
					, " \n"
					, "^5Value: Priceless.\n");

				uq_tooltip(ctx, tooltipString, media, &media->inventory[i]);
			}
		}
		else
		{// Blank slot...
			nk_color color = nk_rgba(0, 0, 0, 0);
			ctx->style.button.border_color = color;
			ctx->style.button.border = 4.0;
			ctx->style.button.image_padding = nk_vec2(-3.0, -2.0);
			ctx->style.button.rounding = 4.0;
			ret = nk_button_image_label(ctx, media->inventoryBlank, label, NK_TEXT_CENTERED);
		}

		if (ret != 0 && backEnd.ui_MouseCursor > 0)
		{// Have something selected for drag, also clicked this quick slot, place the selected item here...
			//ri->Printf(PRINT_ALL, "Drop item handle %i...\n", backEnd.ui_MouseCursor);

			for (int j = 0; j < 64; ++j)
			{
				if (media->inventory[j].handle.id == backEnd.ui_MouseCursor)
				{
					ri->Printf(PRINT_ALL, "Dropped inventory item %i into quickbar slot %i...\n", j, i);
					quickBarSelections[i] = j;
					backEnd.ui_MouseCursor = -1;
					break;
				}
			}
		}
	}

	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);
}

int PREVIOUS_INVENTORY_TOOLTIP = -1;
int PREVIOUS_INVENTORY_TOOLTIP_QUALITY = 0;
int PREVIOUS_INVENTORY_TOOLTIP_TIME = 0;

int selectedContextItem = 0;
int noSelectTime = 0;
struct nk_rect selectedContextBounds;

static void
GUI_Inventory(struct nk_context *ctx, struct media *media)
{
	static int image_active;
	static int selected_item = 0;
	static int selected_image = 3;
	static int selected_icon = 0;

	float size[2] = { 380.0, 406.0 };
	
	int i = 0;
	nk_style_set_font(ctx, &media->font_20->handle);
	nk_begin(ctx, "Inventory", nk_rect(((float)FBO_WIDTH - size[0]) - 64.0, ((float)FBO_HEIGHT - size[1]) - 64.0, size[0], size[1]), NK_WINDOW_CLOSABLE | NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR);
	//nk_window_set_focus(ctx, "Inventory");

	// Icon...
	nk_window *win = nk_window_find(ctx, "Inventory");
	win->hasIcon = true;
	win->icon = media->iconInventory;

	/*------------------------------------------------
	*                INVENTORY DISPLAY
	*------------------------------------------------*/
	nk_style_set_font(ctx, &media->font_14->handle);
	nk_layout_row_static(ctx, 41, 41, 8);
	//nk_spacing(ctx, 1);

	int tooltipNum = -1;
	int tooltipQuality = QUALITY_GREY;
	
	for (i = 0; i < 64; ++i)
	{
		int ret = 0;
		int quality = QUALITY_GREY;

		if (media->inventory[i].handle.id != media->inventoryBlank.handle.id)
		{
			quality = QUALITY_GOLD - Q_clamp(QUALITY_GREY, i / QUALITY_GOLD, QUALITY_GOLD);
			nk_color bgColor = /*nk_rgba(0, 0, 0, 0); //*/ColorForQuality(quality);

			ctx->style.button.border_color = bgColor;
			ctx->style.button.border = 2.0;
			ctx->style.button.padding = nk_vec2(2.0, 2.0);
			//ctx->style.button.image_padding = nk_vec2(2.0, 2.0);
			ctx->style.button.rounding = 4.0;

			ret = nk_button_image_label(ctx, media->inventory[i], "", NK_TEXT_CENTERED);
		}
		else
		{// Blank slot...
			nk_color color = nk_rgba(0, 0, 0, 0);
			ctx->style.button.border_color = color;
			ctx->style.button.border = 2.0;
			ctx->style.button.padding = nk_vec2(2.0, 2.0);
			//ctx->style.button.image_padding = nk_vec2(2.0, 2.0);
			ctx->style.button.rounding = 4.0;
			ret = nk_button_image_label(ctx, media->inventoryBlank, "", NK_TEXT_CENTERED);
		}

		const struct nk_mouse_button btn1 = ctx->input.mouse.buttons[NK_BUTTON_LEFT];
		const struct nk_mouse_button btn2 = ctx->input.mouse.buttons[NK_BUTTON_RIGHT];
		const struct nk_mouse_button btn3 = ctx->input.mouse.buttons[NK_BUTTON_MIDDLE];
		bool leftClick = (btn1.clicked || btn1.down == nk_true) ? true : false;
		bool rightClick = (btn2.clicked || btn2.down == nk_true) ? true : false;
		bool middleClick = (btn3.clicked || btn3.down == nk_true) ? true : false;
		bool hovered = false;

		if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER
			&& !selectedContextItem
			&& backEnd.ui_MouseCursor <= 0
			&& media->inventory[i].handle.id != media->inventoryBlank.handle.id)
		{// Hoverred...
			tooltipNum = i;
			tooltipQuality = quality;
			hovered = true;

			char tooltipString[1024] = { 0 };

			sprintf(tooltipString, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
				, va("%s^BMy schwartz is bigger than yours (%s)^b\n", ColorStringForQuality(tooltipQuality), itemQualityNames[tooltipQuality])
				, "^POne handed weapon, Lightsaber\n"
				, " \n"
				, "^7Scaling Attribute: ^PStrength\n"
				, "^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n"
				, "^7Attacks per Second: ^P0.45\n"
				, "^7Crit Chance: ^P+11.5%\n"
				, "^7Crit Power: ^P+41.0%\n"
				, " \n"
				, "^0Purple Crystal: ^P+12.0% ^4electric^0, and ^P+12.0% ^Nheat ^0damage.\n"
				, "^N-42.0% ^7Dexterity.\n"
				, "^N-97.0% ^7Intelligence.\n"
				, "^N+33.0% ^7Weight.\n"
				, " \n"
				, "^P+15.0% ^2bonus to trip over your own feet.\n"
				, "^P+50.0% ^2bonus to asking dumb questions.\n"
				, "^P+20.0% ^2bonus to epeen trolling.\n"
				, " \n"
				, "^5Value: Priceless.\n");

			uq_tooltip(ctx, tooltipString, media, &media->inventory[i]);
		}
		
		if (hovered 
			&& !selectedContextItem 
			&& backEnd.ui_MouseCursor <= 0 
			&& noSelectTime <= backEnd.refdef.time
			&& menuNoChangeTime <= backEnd.refdef.time)
		{
			if (leftClick)
			{
				backEnd.ui_MouseCursor = media->inventory[i].handle.id;
				noSelectTime = backEnd.refdef.time + 100;
			}
			else if (rightClick)
			{
				selectedContextItem = i + 1;
				nk_widget(&selectedContextBounds, ctx);
				noSelectTime = backEnd.refdef.time + 100;
			}
			else if (middleClick)
			{
				selectedContextItem = i + 1;
				nk_widget(&selectedContextBounds, ctx);
				noSelectTime = backEnd.refdef.time + 100;
			}
		}
	}

	nk_style_set_font(ctx, &media->font_14->handle);
	nk_end(ctx);

	if (backEnd.ui_MouseCursor > 0)
	{// Have something selected for drag, skip tooltip stuff...
		selectedContextItem = 0;
		tooltipNum = -1;
		tooltipQuality = 0;
		return;
	}
	else if (selectedContextItem)
	{
		tooltipNum = -1;
		tooltipQuality = 0;
		backEnd.ui_MouseCursor = -1;

		struct nk_rect bounds2;
		bounds2.x = selectedContextBounds.x;
		bounds2.y = selectedContextBounds.y;
		bounds2.w = 120.0;
		bounds2.h = 340.0;// 292.0;

		nk_begin(ctx, "Context", bounds2, NK_WINDOW_NO_SCROLLBAR);

		//nk_window_set_focus(ctx, "Context");

		int ret2 = nk_contextual_begin(ctx, NK_WINDOW_NO_SCROLLBAR, nk_vec2(bounds2.w, bounds2.h), selectedContextBounds);

		/* settings */
		nk_layout_row_dynamic(ctx, 20, 1);

		int eRight = nk_contextual_item_image_label(ctx, media->play, "Equip Right", NK_TEXT_RIGHT);
		int eLeft = nk_contextual_item_image_label(ctx, media->play, "Equip Left", NK_TEXT_RIGHT);
		int qb1 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 1", NK_TEXT_RIGHT);
		int qb2 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 2", NK_TEXT_RIGHT);
		int qb3 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 3", NK_TEXT_RIGHT);
		int qb4 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 4", NK_TEXT_RIGHT);
		int qb5 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 5", NK_TEXT_RIGHT);
		int qb6 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 6", NK_TEXT_RIGHT);
		int qb7 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 7", NK_TEXT_RIGHT);
		int qb8 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 8", NK_TEXT_RIGHT);
		int qb9 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 9", NK_TEXT_RIGHT);
		int qb10 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 10", NK_TEXT_RIGHT);
		int qb11 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 11", NK_TEXT_RIGHT);
		int qb12 = nk_contextual_item_image_label(ctx, media->stop, "Quickbar 12", NK_TEXT_RIGHT);
		nk_contextual_end(ctx);

		int selectedItem = selectedContextItem-1;

		if (eRight || eLeft || qb1 || qb2 || qb3 || qb4 || qb5 || qb6 || qb7 || qb8 || qb9 || qb10 || qb11 || qb12)
		{
			if (eRight)
			{

			}
			else if (eLeft)
			{

			}
			else if (qb1)
			{
				quickBarSelections[0] = selectedItem;
			}
			else if (qb2)
			{
				quickBarSelections[1] = selectedItem;
			}
			else if (qb3)
			{
				quickBarSelections[2] = selectedItem;
			}
			else if (qb4)
			{
				quickBarSelections[3] = selectedItem;
			}
			else if (qb5)
			{
				quickBarSelections[4] = selectedItem;
			}
			else if (qb6)
			{
				quickBarSelections[5] = selectedItem;
			}
			else if (qb7)
			{
				quickBarSelections[6] = selectedItem;
			}
			else if (qb8)
			{
				quickBarSelections[7] = selectedItem;
			}
			else if (qb9)
			{
				quickBarSelections[8] = selectedItem;
			}
			else if (qb10)
			{
				quickBarSelections[9] = selectedItem;
			}
			else if (qb11)
			{
				quickBarSelections[10] = selectedItem;
			}
			else if (qb12)
			{
				quickBarSelections[11] = selectedItem;
			}

			selectedContextItem = 0;
			PREVIOUS_INVENTORY_TOOLTIP = -1;

			noSelectTime = backEnd.refdef.time + 500;
		}

		nk_end(ctx);
		return;
	}

	selectedContextItem = 0;
	backEnd.ui_MouseCursor = -1;
}

/* ===============================================================
*
*                          DEVICE
*
* ===============================================================*/
struct nk_glfw_vertex {
	float position[2];
	float uv[2];
	nk_byte col[4];
};

struct device {
	struct nk_buffer cmds;
	struct nk_draw_null_texture null;
	GLuint vbo, vao, ebo;
	GLuint prog;
	GLuint vert_shdr;
	GLuint frag_shdr;
	GLint attrib_pos;
	GLint attrib_uv;
	GLint attrib_col;
	GLint uniform_tex;
	GLint uniform_proj;
	GLuint font_tex;
};

static void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputs("\n", stderr);
	exit(EXIT_FAILURE);
}

static GLuint
image_load(const char *filename)
{
	int x, y, n;
	GLuint tex;
	unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
	
	ri->Printf(PRINT_WARNING, "Loading %s.\n", filename);

	if (!data) {
		//die("[SDL]: failed to load image: %s", filename);
		ri->Printf(PRINT_ERROR, "[SDL]: failed to load image: %s. Using default image.", filename);
		return image_load("Warzone/gui/skins/warzone.png");
	}

	qglGenTextures(1, &tex);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	qglGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	ri->Printf(PRINT_WARNING, "Loaded %s.\n", filename);
	return tex;
}

static struct nk_image
icon_load(const char *filename)
{
	int x, y, n;
	GLuint tex;
	unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
	if (!data) {
		//die("[SDL]: failed to load image: %s", filename);
		ri->Printf(PRINT_ERROR, "[SDL]: failed to load image: %s. Using error image.", filename);
		return icon_load("Warzone/gui/icons/error.png");
	}

	qglGenTextures(1, &tex);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	qglGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return nk_image_id((int)tex);
}

static void
device_init(struct device *dev)
{
	GLint status;
	static const GLchar *vertex_shader =
		NK_SHADER_VERSION
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 TexCoord;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main() {\n"
		"   Frag_UV = TexCoord;\n"
		"   Frag_Color = Color;\n"
		"   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
		"}\n";
	static const GLchar *fragment_shader =
		NK_SHADER_VERSION
		"precision mediump float;\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 out_Color;\n"
		"void main(){\n"
		"   out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
		"}\n";

	nk_buffer_init_default(&dev->cmds);
	dev->prog = qglCreateProgram();
	dev->vert_shdr = qglCreateShader(GL_VERTEX_SHADER);
	dev->frag_shdr = qglCreateShader(GL_FRAGMENT_SHADER);
	qglShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
	qglShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
	qglCompileShader(dev->vert_shdr);
	qglCompileShader(dev->frag_shdr);
	qglGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
	assert(status == GL_TRUE);
	qglGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
	assert(status == GL_TRUE);
	qglAttachShader(dev->prog, dev->vert_shdr);
	qglAttachShader(dev->prog, dev->frag_shdr);
	qglLinkProgram(dev->prog);
	qglGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
	assert(status == GL_TRUE);

	dev->uniform_tex = qglGetUniformLocation(dev->prog, "Texture");
	dev->uniform_proj = qglGetUniformLocation(dev->prog, "ProjMtx");
	dev->attrib_pos = qglGetAttribLocation(dev->prog, "Position");
	dev->attrib_uv = qglGetAttribLocation(dev->prog, "TexCoord");
	dev->attrib_col = qglGetAttribLocation(dev->prog, "Color");

	{
		/* buffer setup */
		GLsizei vs = sizeof(struct nk_glfw_vertex);
		size_t vp = offsetof(struct nk_glfw_vertex, position);
		size_t vt = offsetof(struct nk_glfw_vertex, uv);
		size_t vc = offsetof(struct nk_glfw_vertex, col);

		qglGenBuffers(1, &dev->vbo);
		qglGenBuffers(1, &dev->ebo);
		qglGenVertexArrays(1, &dev->vao);

		qglBindVertexArray(dev->vao);
		qglBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

		qglEnableVertexAttribArray((GLuint)dev->attrib_pos);
		qglEnableVertexAttribArray((GLuint)dev->attrib_uv);
		qglEnableVertexAttribArray((GLuint)dev->attrib_col);

		qglVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
		qglVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
		qglVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
	}

	qglBindTexture(GL_TEXTURE_2D, 0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	qglBindVertexArray(0);
}

static void
device_upload_atlas(struct device *dev, const void *image, int width, int height)
{
	qglGenTextures(1, &dev->font_tex);
	qglBindTexture(GL_TEXTURE_2D, dev->font_tex);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static void
device_shutdown(struct device *dev)
{
	qglDetachShader(dev->prog, dev->vert_shdr);
	qglDetachShader(dev->prog, dev->frag_shdr);
	qglDeleteShader(dev->vert_shdr);
	qglDeleteShader(dev->frag_shdr);
	qglDeleteProgram(dev->prog);
	qglDeleteTextures(1, &dev->font_tex);
	qglDeleteBuffers(1, &dev->vbo);
	qglDeleteBuffers(1, &dev->ebo);
	nk_buffer_free(&dev->cmds);
}


#include "imgui/include_imgui.h"

void RE_CharEvent(int key) {
	// basically just this: https://github.com/ocornut/imgui/blob/69e700f8694f89707b7aec91551f4a9546684040/examples/directx9_example/imgui_impl_dx9.cpp#L273
	// with a round trip from client.exe to renderer.dll
	// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
	if (key > 0 && key < 0x10000) {
		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharacter((unsigned short)key);
	}
}

void RE_MouseWheelEvent(float dir) {
	imgui_mouse_wheel(dir);
}

#include "imgui_openjk/imgui_openjk_default_docks.h"

void RE_RenderImGui() {
	float width = FBO_WIDTH;
	float height = FBO_HEIGHT;
		
	if ( ! (ri->Key_GetCatcher() & KEYCATCH_IMGUI))
		return;

	// copy over all keys to imgui
	ImGuiIO& io = ImGui::GetIO();
	for (int i=0; i<MAX_KEYS; i++)
		io.KeysDown[i] = (keyStatus[i] == qtrue);

	// Read keyboard modifiers inputs
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;

	/* High DPI displays */
	struct nk_vec2 scale;

	scale.x = 1.0;// (float)display_width / (float)width;
	scale.y = 1.0;// (float)display_height / (float)height;
	
	float x, y;
#if 0
	nk_input_begin(&GUI_ctx);

	nk_input_key(&GUI_ctx, NK_KEY_DEL, keyStatus[A_DELETE]);
	nk_input_key(&GUI_ctx, NK_KEY_ENTER, keyStatus[A_ENTER]);
	nk_input_key(&GUI_ctx, NK_KEY_TAB, keyStatus[A_TAB]);
	nk_input_key(&GUI_ctx, NK_KEY_BACKSPACE, keyStatus[A_BACKSPACE]);
	nk_input_key(&GUI_ctx, NK_KEY_LEFT, keyStatus[A_CURSOR_LEFT]);
	nk_input_key(&GUI_ctx, NK_KEY_RIGHT, keyStatus[A_CURSOR_RIGHT]);
	nk_input_key(&GUI_ctx, NK_KEY_UP, keyStatus[A_CURSOR_UP]);
	nk_input_key(&GUI_ctx, NK_KEY_DOWN, keyStatus[A_CURSOR_DOWN]);
	nk_input_key(&GUI_ctx, NK_KEY_SHIFT, keyStatus[A_SHIFT]);

	if (keyStatus[A_CTRL] || keyStatus[A_CTRL2])
	{
	nk_input_key(&GUI_ctx, NK_KEY_COPY, keyStatus[A_CAP_C] || keyStatus[A_LOW_C]);
	nk_input_key(&GUI_ctx, NK_KEY_PASTE, keyStatus[A_CAP_P] || keyStatus[A_LOW_P]);
	nk_input_key(&GUI_ctx, NK_KEY_CUT, keyStatus[A_CAP_X] || keyStatus[A_LOW_X]);
	nk_input_key(&GUI_ctx, NK_KEY_CUT, keyStatus[A_CAP_E] || keyStatus[A_LOW_E]);
	nk_input_key(&GUI_ctx, NK_KEY_SHIFT, 1);
	}
	else
	{
	nk_input_key(&GUI_ctx, NK_KEY_COPY, 0);
	nk_input_key(&GUI_ctx, NK_KEY_PASTE, 0);
	nk_input_key(&GUI_ctx, NK_KEY_CUT, 0);
	nk_input_key(&GUI_ctx, NK_KEY_SHIFT, 0);

	if (keyStatus[A_CTRL] || keyStatus[A_CTRL2])
	{
	nk_input_key(&GUI_ctx, NK_KEY_CTRL, 0);
	}
	}
#endif

#if 1//defined(__GNUC__) || defined(MACOS_X)
	vec2_t ratio;
	ratio[0] = (float)glConfig.vidWidth / (float)SCREEN_WIDTH;
	ratio[1] = (float)glConfig.vidHeight / (float)SCREEN_HEIGHT;
	x = mouseStatus[0] * ratio[0];
	y = mouseStatus[1] * ratio[1];
#else
	POINT p;
	if (GetCursorPos(&p))
	{//cursor position now in p.x and p.y
		//HANDLE hwnd = GetCurrentProcess();
		HWND hwnd = GetActiveWindow();
		if (ScreenToClient(hwnd, &p))
		{
			//p.x and p.y are now relative to hwnd's client area
			x = p.x;
			y = p.y;
		}
	}
#endif

	imgui_set_mousepos((int)x, (int)y);
	imgui_set_widthheight(width, height);
	imgui_mouse_set_button(NK_BUTTON_LEFT, keyStatus[A_MOUSE1]);
	imgui_mouse_set_button(NK_BUTTON_RIGHT, keyStatus[A_MOUSE2]);
	imgui_mouse_set_button(NK_BUTTON_MIDDLE, keyStatus[A_MOUSE3]);

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */
#if 0
	for (int i = A_CAP_A; i <= A_CAP_Z; i++)
	{
		if (keyStatus[i])
		{
			int vkNum = (i - A_CAP_A) + 0x30;
			nk_input_char(&GUI_ctx, vkNum);
		}
		else if (keyStatus[i-32])
		{
			int vkNum = (i - A_LOW_A) + 0x30;
			nk_input_char(&GUI_ctx, vkNum);
		}
	}

	for (int i = A_0; i <= A_9; i++)
	{
		if (keyStatus[i])
		{
			int vkNum = (i - A_0) + 0x41;
			nk_input_char(&GUI_ctx, vkNum);
		}
	}
#endif

	//FBO_Bind(tr.renderGUIFbo);
	FBO_Bind(NULL);
	GL_SetDefaultState();
	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);;
	GL_Cull(CT_TWO_SIDED);
	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	//qglClear(GL_COLOR_BUFFER_BIT);

	static int first = 1;
	if (first) {
		imgui_init();
		first = 0;
	}

	imgui_new_frame();
	imgui_render();
	imgui_openjk_default_docks();
	imgui_end_frame();

	/* default OpenGL state */
	qglUseProgram(0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	qglBindVertexArray(0);
	FBO_Bind(glState.previousFBO);
	GL_SetDefaultState();
}


static void
device_draw(struct device *dev, struct nk_context *ctx, int width, int height,
	struct nk_vec2 scale, enum nk_anti_aliasing AA)
{
	FBO_Bind(tr.renderGUIFbo);

	GLfloat ortho[4][4] = {
		{ 2.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f,-2.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f,-1.0f, 0.0f },
		{ -1.0f,1.0f, 0.0f, 1.0f },
	};
	ortho[0][0] /= (GLfloat)width;
	ortho[1][1] /= (GLfloat)height;
	
	GL_SetDefaultState();
	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);;
	GL_Cull(CT_TWO_SIDED);

	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	qglClear(GL_COLOR_BUFFER_BIT);

	/* setup program */
	qglUseProgram(dev->prog);
	qglUniform1i(dev->uniform_tex, 0);
	qglUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
	{
		/* convert from command queue into draw list and draw to screen */
		const struct nk_draw_command *cmd;
		void *vertices, *elements;
		const nk_draw_index *offset = NULL;

		/* allocate vertex and element buffer */
		qglBindVertexArray(dev->vao);
		qglBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
		qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

		qglBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

		/* load draw vertices & elements directly into vertex + element buffer */
		vertices = qglMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		elements = qglMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
		{
			/* fill convert configuration */
			struct nk_convert_config config;
			static const struct nk_draw_vertex_layout_element vertex_layout[] = {
				{ NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position) },
				{ NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv) },
				{ NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col) },
				{ NK_VERTEX_LAYOUT_END }
			};
			NK_MEMSET(&config, 0, sizeof(config));
			config.vertex_layout = vertex_layout;
			config.vertex_size = sizeof(struct nk_glfw_vertex);
			config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
			config.null = dev->null;
			config.circle_segment_count = 22;
			config.curve_segment_count = 22;
			config.arc_segment_count = 22;
			config.global_alpha = 1.0f;
			config.shape_AA = AA;
			config.line_AA = AA;

			/* setup buffers to load vertices and elements */
			{
				struct nk_buffer vbuf, ebuf;
				nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
				nk_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
				nk_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);
			}
		}
		qglUnmapBuffer(GL_ARRAY_BUFFER);
		qglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

		/* iterate over and execute each draw command */
		nk_draw_foreach(cmd, ctx, &dev->cmds)
		{
			if (!cmd->elem_count) continue;
			qglBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
			qglScissor(
				(GLint)(cmd->clip_rect.x * scale.x),
				(GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
				(GLint)(cmd->clip_rect.w * scale.x),
				(GLint)(cmd->clip_rect.h * scale.y));
			qglDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
			offset += cmd->elem_count;
		}
		nk_clear(ctx);
	}

	/* default OpenGL state */
	qglUseProgram(0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	qglBindVertexArray(0);

	FBO_Bind(glState.previousFBO);

	GL_SetDefaultState();
}

/* glfw callbacks (I don't know if there is a easier way to access text and scroll )*/
static void error_callback(int e, const char *d) { ri->Printf(PRINT_ERROR, "Error %d: %s\n", e, d); }

/*static void text_input(GLFWwindow *win, unsigned int codepoint)
{
	nk_input_unicode((struct nk_context*)glfwGetWindowUserPointer(win), codepoint);
}
static void scroll_input(GLFWwindow *win, double _, double yoff)
{
	UNUSED(_); nk_input_scroll((struct nk_context*)glfwGetWindowUserPointer(win), nk_vec2(0, (float)yoff));
}*/


/* GUI */
struct device GUI_device;
struct nk_font_atlas GUI_atlas;
struct media GUI_media;
struct nk_context GUI_ctx;

#if defined(__GUI_SKINNED__)
void GUI_InitSkin()
{
	{   /* skin */
		qglEnable(GL_TEXTURE_2D);
		GUI_media.skin = image_load("Warzone/gui/skins/warzone.png");
		GUI_media.checkSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 32, 15, 15));
		GUI_media.check_cursorSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(450, 34, 11, 11));
		GUI_media.optionSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 64, 15, 15));
		GUI_media.option_cursorSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(451, 67, 9, 9));
		GUI_media.headerSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(128, 0, 127, 24));
		GUI_media.windowSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(128, 23, 127, 104));
		GUI_media.scrollbar_inc_buttonSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 256, 15, 15));
		GUI_media.scrollbar_inc_button_hoverSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 320, 15, 15));
		GUI_media.scrollbar_dec_buttonSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 224, 15, 15));
		GUI_media.scrollbar_dec_button_hoverSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(464, 288, 15, 15));
		GUI_media.buttonSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(384, 336, 127, 31));
		GUI_media.button_hoverSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(384, 368, 127, 31));
		GUI_media.button_activeSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(384, 400, 127, 31));
		GUI_media.tab_minimizeSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(451, 99, 9, 9));
		GUI_media.tab_maximizeSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(467, 99, 9, 9));
		GUI_media.sliderSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(418, 33, 11, 14));
		GUI_media.slider_hoverSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(418, 49, 11, 14));
		GUI_media.slider_activeSkin = nk_subimage_id(GUI_media.skin, 512, 512, nk_rect(418, 64, 11, 14));

		/* window */
		GUI_ctx.style.window.background = nk_rgb(204, 204, 204);
		GUI_ctx.style.window.fixed_background = nk_style_item_image(GUI_media.windowSkin);
		GUI_ctx.style.window.border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.combo_border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.contextual_border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.menu_border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.group_border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.tooltip_border_color = nk_rgb(67, 67, 67);
		GUI_ctx.style.window.scrollbar_size = nk_vec2(16, 16);
		GUI_ctx.style.window.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.window.padding = nk_vec2(8, 4);
		GUI_ctx.style.window.border = 3;

		/* window header */
		GUI_ctx.style.window.header.normal = nk_style_item_image(GUI_media.headerSkin);
		GUI_ctx.style.window.header.hover = nk_style_item_image(GUI_media.headerSkin);
		GUI_ctx.style.window.header.active = nk_style_item_image(GUI_media.headerSkin);
		GUI_ctx.style.window.header.label_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.window.header.label_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.window.header.label_active = nk_rgb(95, 95, 95);

		/* scrollbar */
		GUI_ctx.style.scrollv.normal = nk_style_item_color(nk_rgb(184, 184, 184));
		GUI_ctx.style.scrollv.hover = nk_style_item_color(nk_rgb(184, 184, 184));
		GUI_ctx.style.scrollv.active = nk_style_item_color(nk_rgb(184, 184, 184));
		GUI_ctx.style.scrollv.cursor_normal = nk_style_item_color(nk_rgb(220, 220, 220));
		GUI_ctx.style.scrollv.cursor_hover = nk_style_item_color(nk_rgb(235, 235, 235));
		GUI_ctx.style.scrollv.cursor_active = nk_style_item_color(nk_rgb(99, 202, 255));
		GUI_ctx.style.scrollv.dec_symbol = NK_SYMBOL_NONE;
		GUI_ctx.style.scrollv.inc_symbol = NK_SYMBOL_NONE;
		GUI_ctx.style.scrollv.show_buttons = nk_true;
		GUI_ctx.style.scrollv.border_color = nk_rgb(81, 81, 81);
		GUI_ctx.style.scrollv.cursor_border_color = nk_rgb(81, 81, 81);
		GUI_ctx.style.scrollv.border = 1;
		GUI_ctx.style.scrollv.rounding = 0;
		GUI_ctx.style.scrollv.border_cursor = 1;
		GUI_ctx.style.scrollv.rounding_cursor = 2;

		/* scrollbar buttons */
		GUI_ctx.style.scrollv.inc_button.normal = nk_style_item_image(GUI_media.scrollbar_inc_buttonSkin);
		GUI_ctx.style.scrollv.inc_button.hover = nk_style_item_image(GUI_media.scrollbar_inc_button_hoverSkin);
		GUI_ctx.style.scrollv.inc_button.active = nk_style_item_image(GUI_media.scrollbar_inc_button_hoverSkin);
		GUI_ctx.style.scrollv.inc_button.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.inc_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.inc_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.inc_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.inc_button.text_active = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.inc_button.border = 0.0f;

		GUI_ctx.style.scrollv.dec_button.normal = nk_style_item_image(GUI_media.scrollbar_dec_buttonSkin);
		GUI_ctx.style.scrollv.dec_button.hover = nk_style_item_image(GUI_media.scrollbar_dec_button_hoverSkin);
		GUI_ctx.style.scrollv.dec_button.active = nk_style_item_image(GUI_media.scrollbar_dec_button_hoverSkin);
		GUI_ctx.style.scrollv.dec_button.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.dec_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.dec_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.dec_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.dec_button.text_active = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.scrollv.dec_button.border = 0.0f;

		/* checkbox toggle */
		{struct nk_style_toggle *toggle;
		toggle = &GUI_ctx.style.checkbox;
		toggle->normal = nk_style_item_image(GUI_media.checkSkin);
		toggle->hover = nk_style_item_image(GUI_media.checkSkin);
		toggle->active = nk_style_item_image(GUI_media.checkSkin);
		toggle->cursor_normal = nk_style_item_image(GUI_media.check_cursorSkin);
		toggle->cursor_hover = nk_style_item_image(GUI_media.check_cursorSkin);
		toggle->text_normal = nk_rgb(95, 95, 95);
		toggle->text_hover = nk_rgb(95, 95, 95);
		toggle->text_active = nk_rgb(95, 95, 95); }

		/* option toggle */
		{struct nk_style_toggle *toggle;
		toggle = &GUI_ctx.style.option;
		toggle->normal = nk_style_item_image(GUI_media.optionSkin);
		toggle->hover = nk_style_item_image(GUI_media.optionSkin);
		toggle->active = nk_style_item_image(GUI_media.optionSkin);
		toggle->cursor_normal = nk_style_item_image(GUI_media.option_cursorSkin);
		toggle->cursor_hover = nk_style_item_image(GUI_media.option_cursorSkin);
		toggle->text_normal = nk_rgb(95, 95, 95);
		toggle->text_hover = nk_rgb(95, 95, 95);
		toggle->text_active = nk_rgb(95, 95, 95); }

		/* default button */
		GUI_ctx.style.button.normal = nk_style_item_image(GUI_media.buttonSkin);
		GUI_ctx.style.button.hover = nk_style_item_image(GUI_media.button_hoverSkin);
		GUI_ctx.style.button.active = nk_style_item_image(GUI_media.button_activeSkin);
		GUI_ctx.style.button.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.button.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.button.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.button.text_active = nk_rgb(95, 95, 95);

		/* default text */
		GUI_ctx.style.text.color = nk_rgb(95, 95, 95);

		/* contextual button */
		GUI_ctx.style.contextual_button.normal = nk_style_item_color(nk_rgb(206, 206, 206));
		GUI_ctx.style.contextual_button.hover = nk_style_item_color(nk_rgb(229, 229, 229));
		GUI_ctx.style.contextual_button.active = nk_style_item_color(nk_rgb(99, 202, 255));
		GUI_ctx.style.contextual_button.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.contextual_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.contextual_button.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.contextual_button.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.contextual_button.text_active = nk_rgb(95, 95, 95);

		/* menu button */
		GUI_ctx.style.menu_button.normal = nk_style_item_color(nk_rgb(206, 206, 206));
		GUI_ctx.style.menu_button.hover = nk_style_item_color(nk_rgb(229, 229, 229));
		GUI_ctx.style.menu_button.active = nk_style_item_color(nk_rgb(99, 202, 255));
		GUI_ctx.style.menu_button.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.menu_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.menu_button.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.menu_button.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.menu_button.text_active = nk_rgb(95, 95, 95);

		/* tree */
		GUI_ctx.style.tab.text = nk_rgb(95, 95, 95);
		GUI_ctx.style.tab.tab_minimize_button.normal = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.tab_minimize_button.hover = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.tab_minimize_button.active = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.tab_minimize_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_minimize_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_minimize_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_minimize_button.text_active = nk_rgba(0, 0, 0, 0);

		GUI_ctx.style.tab.tab_maximize_button.normal = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.tab_maximize_button.hover = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.tab_maximize_button.active = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.tab_maximize_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_maximize_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_maximize_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.tab_maximize_button.text_active = nk_rgba(0, 0, 0, 0);

		GUI_ctx.style.tab.node_minimize_button.normal = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.node_minimize_button.hover = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.node_minimize_button.active = nk_style_item_image(GUI_media.tab_minimizeSkin);
		GUI_ctx.style.tab.node_minimize_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_minimize_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_minimize_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_minimize_button.text_active = nk_rgba(0, 0, 0, 0);

		GUI_ctx.style.tab.node_maximize_button.normal = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.node_maximize_button.hover = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.node_maximize_button.active = nk_style_item_image(GUI_media.tab_maximizeSkin);
		GUI_ctx.style.tab.node_maximize_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_maximize_button.text_normal = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_maximize_button.text_hover = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.tab.node_maximize_button.text_active = nk_rgba(0, 0, 0, 0);

		/* selectable */
		GUI_ctx.style.selectable.normal = nk_style_item_color(nk_rgb(206, 206, 206));
		GUI_ctx.style.selectable.hover = nk_style_item_color(nk_rgb(206, 206, 206));
		GUI_ctx.style.selectable.pressed = nk_style_item_color(nk_rgb(206, 206, 206));
		GUI_ctx.style.selectable.normal_active = nk_style_item_color(nk_rgb(185, 205, 248));
		GUI_ctx.style.selectable.hover_active = nk_style_item_color(nk_rgb(185, 205, 248));
		GUI_ctx.style.selectable.pressed_active = nk_style_item_color(nk_rgb(185, 205, 248));
		GUI_ctx.style.selectable.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.selectable.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.selectable.text_pressed = nk_rgb(95, 95, 95);
		GUI_ctx.style.selectable.text_normal_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.selectable.text_hover_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.selectable.text_pressed_active = nk_rgb(95, 95, 95);

		/* slider */
		GUI_ctx.style.slider.normal = nk_style_item_hide();
		GUI_ctx.style.slider.hover = nk_style_item_hide();
		GUI_ctx.style.slider.active = nk_style_item_hide();
		GUI_ctx.style.slider.bar_normal = nk_rgb(156, 156, 156);
		GUI_ctx.style.slider.bar_hover = nk_rgb(156, 156, 156);
		GUI_ctx.style.slider.bar_active = nk_rgb(156, 156, 156);
		GUI_ctx.style.slider.bar_filled = nk_rgb(156, 156, 156);
		GUI_ctx.style.slider.cursor_normal = nk_style_item_image(GUI_media.sliderSkin);
		GUI_ctx.style.slider.cursor_hover = nk_style_item_image(GUI_media.slider_hoverSkin);
		GUI_ctx.style.slider.cursor_active = nk_style_item_image(GUI_media.slider_activeSkin);
		GUI_ctx.style.slider.cursor_size = nk_vec2(16.5f, 21);
		GUI_ctx.style.slider.bar_height = 1;

		/* progressbar */
		GUI_ctx.style.progress.normal = nk_style_item_color(nk_rgb(231, 231, 231));
		GUI_ctx.style.progress.hover = nk_style_item_color(nk_rgb(231, 231, 231));
		GUI_ctx.style.progress.active = nk_style_item_color(nk_rgb(231, 231, 231));
		GUI_ctx.style.progress.cursor_normal = nk_style_item_color(nk_rgb(63, 242, 93));
		GUI_ctx.style.progress.cursor_hover = nk_style_item_color(nk_rgb(63, 242, 93));
		GUI_ctx.style.progress.cursor_active = nk_style_item_color(nk_rgb(63, 242, 93));
		GUI_ctx.style.progress.border_color = nk_rgb(114, 116, 115);
		GUI_ctx.style.progress.padding = nk_vec2(0, 0);
		GUI_ctx.style.progress.border = 2;
		GUI_ctx.style.progress.rounding = 1;

		/* combo */
		GUI_ctx.style.combo.normal = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.hover = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.active = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.border_color = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.label_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.label_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.label_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.border = 1;
		GUI_ctx.style.combo.rounding = 1;

		/* combo button */
		GUI_ctx.style.combo.button.normal = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.button.hover = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.button.active = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.combo.button.text_background = nk_rgb(216, 216, 216);
		GUI_ctx.style.combo.button.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.button.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.combo.button.text_active = nk_rgb(95, 95, 95);

		/* property */
		GUI_ctx.style.property.normal = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.hover = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.active = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.border_color = nk_rgb(81, 81, 81);
		GUI_ctx.style.property.label_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.label_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.label_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.sym_left = NK_SYMBOL_TRIANGLE_LEFT;
		GUI_ctx.style.property.sym_right = NK_SYMBOL_TRIANGLE_RIGHT;
		GUI_ctx.style.property.rounding = 10;
		GUI_ctx.style.property.border = 1;

		/* edit */
		GUI_ctx.style.edit.normal = nk_style_item_color(nk_rgb(240, 240, 240));
		GUI_ctx.style.edit.hover = nk_style_item_color(nk_rgb(240, 240, 240));
		GUI_ctx.style.edit.active = nk_style_item_color(nk_rgb(240, 240, 240));
		GUI_ctx.style.edit.border_color = nk_rgb(62, 62, 62);
		GUI_ctx.style.edit.cursor_normal = nk_rgb(99, 202, 255);
		GUI_ctx.style.edit.cursor_hover = nk_rgb(99, 202, 255);
		GUI_ctx.style.edit.cursor_text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.cursor_text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.text_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.selected_normal = nk_rgb(99, 202, 255);
		GUI_ctx.style.edit.selected_hover = nk_rgb(99, 202, 255);
		GUI_ctx.style.edit.selected_text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.selected_text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.edit.border = 1;
		GUI_ctx.style.edit.rounding = 2;

		/* property buttons */
		GUI_ctx.style.property.dec_button.normal = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.dec_button.hover = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.dec_button.active = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.dec_button.text_background = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.property.dec_button.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.dec_button.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.dec_button.text_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.inc_button = GUI_ctx.style.property.dec_button;

		/* property edit */
		GUI_ctx.style.property.edit.normal = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.edit.hover = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.edit.active = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.property.edit.border_color = nk_rgba(0, 0, 0, 0);
		GUI_ctx.style.property.edit.cursor_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.cursor_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.cursor_text_normal = nk_rgb(216, 216, 216);
		GUI_ctx.style.property.edit.cursor_text_hover = nk_rgb(216, 216, 216);
		GUI_ctx.style.property.edit.text_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.text_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.text_active = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.selected_normal = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.selected_hover = nk_rgb(95, 95, 95);
		GUI_ctx.style.property.edit.selected_text_normal = nk_rgb(216, 216, 216);
		GUI_ctx.style.property.edit.selected_text_hover = nk_rgb(216, 216, 216);

		/* chart */
		GUI_ctx.style.chart.background = nk_style_item_color(nk_rgb(216, 216, 216));
		GUI_ctx.style.chart.border_color = nk_rgb(81, 81, 81);
		GUI_ctx.style.chart.color = nk_rgb(95, 95, 95);
		GUI_ctx.style.chart.selected_color = nk_rgb(255, 0, 0);
		GUI_ctx.style.chart.border = 1;
	}
}
#endif

void GUI_Init(void)
{
	ri->Printf(PRINT_WARNING, "GUI_Init: Begin.\n");

	/* OpenGL */
	qglViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);

	{/* GUI */
		device_init(&GUI_device);
		{
			const void *image; int w, h;
			struct nk_font_config cfg = nk_font_config(0);
			cfg.oversample_h = 3; cfg.oversample_v = 2;
			/* Loading one font with different heights is only required if you want higher
			* quality text otherwise you can just set the font height directly
			* e.g.: ctx->style.font.height = 20. */
			nk_font_atlas_init_default(&GUI_atlas);
			nk_font_atlas_begin(&GUI_atlas);
			GUI_media.font_14 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 14.0f, &cfg);
			GUI_media.font_15 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 15.0f, &cfg);
			GUI_media.font_16 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 16.0f, &cfg);
			GUI_media.font_17 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 17.0f, &cfg);
			GUI_media.font_18 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 18.0f, &cfg);
			GUI_media.font_19 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 19.0f, &cfg);
			GUI_media.font_20 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 20.0f, &cfg);
			GUI_media.font_21 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 21.0f, &cfg);
			GUI_media.font_22 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 22.0f, &cfg);
			GUI_media.font_24 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 24.0f, &cfg);
			GUI_media.font_28 = nk_font_atlas_add_from_file(&GUI_atlas, "Warzone/gui/fonts/Roboto-Regular.ttf", 28.0f, &cfg);
			image = nk_font_atlas_bake(&GUI_atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
			device_upload_atlas(&GUI_device, image, w, h);
			nk_font_atlas_end(&GUI_atlas, nk_handle_id((int)GUI_device.font_tex), &GUI_device.null);
		}

		nk_init_default(&GUI_ctx, &GUI_media.font_14->handle);
	}
	
	ri->Printf(PRINT_WARNING, "GUI_Init: Device Initialized.\n");

#if defined(__GUI_SKINNED__)
	GUI_InitSkin();
#elif defined(__GUI_THEMED__)
	#if defined(__GUI_THEME_WARZONE__)
		set_style(&GUI_ctx, THEME_WARZONE);
	#elif defined(__GUI_THEME_BLACK__)
		set_style(&GUI_ctx, THEME_BLACK);
	#elif defined(__GUI_THEME_WHITE__)
		set_style(&GUI_ctx, THEME_WHITE);
	#elif defined(__GUI_THEME_RED__)
		set_style(&GUI_ctx, THEME_RED);
	#elif defined(__GUI_THEME_BLUE__)
		set_style(&GUI_ctx, THEME_BLUE);
	#elif defined(__GUI_THEME_DARK__)
		set_style(&GUI_ctx, THEME_DARK);
	#else // default
		set_style(&GUI_ctx, THEME_WARZONE);
	#endif
#endif

	//GUI_ctx->style.window.fixed_background = nk_style_item_image(nk_image_xxx(your_image_texture)); // Load image background for static window size...

	/* icons */
	qglEnable(GL_TEXTURE_2D);
	GUI_media.unchecked = icon_load("Warzone/gui/icons/unchecked.png");
	GUI_media.checked = icon_load("Warzone/gui/icons/checked.png");
	GUI_media.rocket = icon_load("Warzone/gui/icons/rocket.png");
	GUI_media.cloud = icon_load("Warzone/gui/icons/cloud.png");
	GUI_media.pen = icon_load("Warzone/gui/icons/pen.png");
	GUI_media.play = icon_load("Warzone/gui/icons/play.png");
	GUI_media.pause = icon_load("Warzone/gui/icons/pause.png");
	GUI_media.stop = icon_load("Warzone/gui/icons/stop.png");
	GUI_media.next = icon_load("Warzone/gui/icons/next.png");
	GUI_media.prev = icon_load("Warzone/gui/icons/prev.png");
	GUI_media.tools = icon_load("Warzone/gui/icons/tools.png");
	GUI_media.dir = icon_load("Warzone/gui/icons/directory.png");
	GUI_media.copy = icon_load("Warzone/gui/icons/copy.png");
	GUI_media.convert = icon_load("Warzone/gui/icons/export.png");
	GUI_media.del = icon_load("Warzone/gui/icons/delete.png");
	GUI_media.edit = icon_load("Warzone/gui/icons/edit.png");
	GUI_media.volume = icon_load("Warzone/gui/icons/volume.png");
	GUI_media.menu[0] = icon_load("Warzone/gui/icons/home.png");
	GUI_media.menu[1] = icon_load("Warzone/gui/icons/phone.png");
	GUI_media.menu[2] = icon_load("Warzone/gui/icons/plane.png");
	GUI_media.menu[3] = icon_load("Warzone/gui/icons/wifi.png");
	GUI_media.menu[4] = icon_load("Warzone/gui/icons/settings.png");
	GUI_media.menu[5] = icon_load("Warzone/gui/icons/volume.png");
	

	// Warzone window Icons...
	GUI_media.iconMainMenu = icon_load("Warzone/gui/windowIcons/mainmenu.png");
	GUI_media.iconAbilities = icon_load("Warzone/gui/windowIcons/abilities.png");
	GUI_media.iconCharacter = icon_load("Warzone/gui/windowIcons/character.png");
	GUI_media.iconInventory = icon_load("Warzone/gui/windowIcons/inventory.png");
	GUI_media.iconPowers = icon_load("Warzone/gui/windowIcons/powers.png");
	GUI_media.iconRadio = icon_load("Warzone/gui/windowIcons/radio.png");
	GUI_media.iconSettings = icon_load("Warzone/gui/windowIcons/settings.png");

	GUI_media.inventoryBlank = icon_load("Warzone/gui/images/blank.png");

	{
		int num = 1;
		for (int i = 0; i < 64; ++i) 
		{
			if (num >= 45)
				GUI_media.inventory[i] = GUI_media.inventoryBlank;
			else
				GUI_media.inventory[i] = icon_load(va("Warzone/gui/images/icon%d.png", num));
			num++;
			//if (num > 45) num = 1;
		}
	}

	ri->Printf(PRINT_WARNING, "GUI_Init: Textures Initialized.\n");
}

void GUI_Shutdown(void)
{
	qglDeleteTextures(1, (const GLuint*)&GUI_media.unchecked.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.checked.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.rocket.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.cloud.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.pen.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.play.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.pause.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.stop.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.next.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.prev.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.tools.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.dir.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.del.handle.id);

	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconMainMenu.handle.id);
	
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconAbilities.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconCharacter.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconInventory.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconPowers.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconRadio.handle.id);
	qglDeleteTextures(1, (const GLuint*)&GUI_media.iconSettings.handle.id);

#ifdef __GUI_SKINNED__
	qglDeleteTextures(1, (const GLuint*)&GUI_media.skin);
#endif

	nk_font_atlas_clear(&GUI_atlas);
	nk_free(&GUI_ctx);

	device_shutdown(&GUI_device);
}

void NuklearUI_Main(void)
{
	if (!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
	{
		float width = FBO_WIDTH;
		float height = FBO_HEIGHT;

		/* High DPI displays */
		struct nk_vec2 scale;

		scale.x = 1.0;// (float)display_width / (float)width;
		scale.y = 1.0;// (float)display_height / (float)height;

#if 1
	/* Input */
		{
			double x, y;
			nk_input_begin(&GUI_ctx);

			nk_input_key(&GUI_ctx, NK_KEY_DEL, keyStatus[A_DELETE]);
			nk_input_key(&GUI_ctx, NK_KEY_ENTER, keyStatus[A_ENTER]);
			nk_input_key(&GUI_ctx, NK_KEY_TAB, keyStatus[A_TAB]);
			nk_input_key(&GUI_ctx, NK_KEY_BACKSPACE, keyStatus[A_BACKSPACE]);
			nk_input_key(&GUI_ctx, NK_KEY_LEFT, keyStatus[A_CURSOR_LEFT]);
			nk_input_key(&GUI_ctx, NK_KEY_RIGHT, keyStatus[A_CURSOR_RIGHT]);
			nk_input_key(&GUI_ctx, NK_KEY_UP, keyStatus[A_CURSOR_UP]);
			nk_input_key(&GUI_ctx, NK_KEY_DOWN, keyStatus[A_CURSOR_DOWN]);
			nk_input_key(&GUI_ctx, NK_KEY_SHIFT, keyStatus[A_SHIFT]);

			if (keyStatus[A_CTRL] || keyStatus[A_CTRL2])
			{
				nk_input_key(&GUI_ctx, NK_KEY_COPY, keyStatus[A_CAP_C] || keyStatus[A_LOW_C]);
				nk_input_key(&GUI_ctx, NK_KEY_PASTE, keyStatus[A_CAP_P] || keyStatus[A_LOW_P]);
				nk_input_key(&GUI_ctx, NK_KEY_CUT, keyStatus[A_CAP_X] || keyStatus[A_LOW_X]);
				nk_input_key(&GUI_ctx, NK_KEY_CUT, keyStatus[A_CAP_E] || keyStatus[A_LOW_E]);
				nk_input_key(&GUI_ctx, NK_KEY_SHIFT, 1);
			}
			else
			{
				nk_input_key(&GUI_ctx, NK_KEY_COPY, 0);
				nk_input_key(&GUI_ctx, NK_KEY_PASTE, 0);
				nk_input_key(&GUI_ctx, NK_KEY_CUT, 0);
				nk_input_key(&GUI_ctx, NK_KEY_SHIFT, 0);

				if (keyStatus[A_CTRL] || keyStatus[A_CTRL2])
				{
					nk_input_key(&GUI_ctx, NK_KEY_CTRL, 0);
				}
			}

#if 1//defined(__GNUC__) || defined(MACOS_X)
			vec2_t ratio;
			ratio[0] = (float)glConfig.vidWidth / (float)SCREEN_WIDTH;
			ratio[1] = (float)glConfig.vidHeight / (float)SCREEN_HEIGHT;
			x = mouseStatus[0] * ratio[0]; y = mouseStatus[1] * ratio[1];
#else
			POINT p;
			if (GetCursorPos(&p))
			{//cursor position now in p.x and p.y
				//HANDLE hwnd = GetCurrentProcess();
				HWND hwnd = GetActiveWindow();
				if (ScreenToClient(hwnd, &p))
				{
					//p.x and p.y are now relative to hwnd's client area
					x = p.x;
					y = p.y;
				}
			}
#endif
			//ri->Printf(PRINT_ALL, "RENDERER: Mouse is at %f %f.\n", x, y);
			//if (keyStatus[A_MOUSE1]) ri->Printf(PRINT_ALL, "RENDERER: Mouse left CLICKED!\n");
			//if (keyStatus[A_MOUSE2]) ri->Printf(PRINT_ALL, "RENDERER: Mouse right CLICKED!\n");
			//if (keyStatus[A_MOUSE3]) ri->Printf(PRINT_ALL, "RENDERER: Mouse middle CLICKED!\n");

			nk_input_motion(&GUI_ctx, (int)x, (int)y);
			nk_input_button(&GUI_ctx, NK_BUTTON_LEFT, (int)x, (int)y, keyStatus[A_MOUSE1]);
			nk_input_button(&GUI_ctx, NK_BUTTON_RIGHT, (int)x, (int)y, keyStatus[A_MOUSE2]);
			nk_input_button(&GUI_ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, keyStatus[A_MOUSE3]);
			nk_input_end(&GUI_ctx);

			float wheel = 0;

			if (keyStatus[A_MWHEELDOWN])
				wheel = -1.0;
			else if (keyStatus[A_MWHEELUP])
				wheel = 1.0;

			if (keyStatus[A_MWHEELDOWN]) ri->Printf(PRINT_ALL, "RENDERER: Mouse scroll down!\n");
			if (keyStatus[A_MWHEELUP]) ri->Printf(PRINT_ALL, "RENDERER: Mouse scroll up!\n");

			nk_input_scroll(&GUI_ctx, nk_vec2( wheel, 0.0 ));

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

			for (int i = A_CAP_A; i <= A_CAP_Z; i++)
			{
				if (keyStatus[i])
				{
					int vkNum = (i - A_CAP_A) + 0x30;
					nk_input_char(&GUI_ctx, vkNum);
				}
				else if (keyStatus[i-32])
				{
					int vkNum = (i - A_LOW_A) + 0x30;
					nk_input_char(&GUI_ctx, vkNum);
				}
			}

			for (int i = A_0; i <= A_9; i++)
			{
				if (keyStatus[i])
				{
					int vkNum = (i - A_0) + 0x41;
					nk_input_char(&GUI_ctx, vkNum);
				}
			}
		}
#else
		
		// just for testing tooltips...
		float size[2] = { 380.0, 406.0 };
		nk_input_begin(&GUI_ctx);
		nk_input_motion(&GUI_ctx, (((float)FBO_WIDTH - size[0]) - 64.0) + (size[0]/3.0), (((float)FBO_HEIGHT - size[1]) - 64.0) + (size[1] / 3.5));
		nk_input_end(&GUI_ctx);
		
#endif

		/* GUI */
		if (menuOpen)
		{// Others need the menu open...
			GUI_CheckOpenWindows(&GUI_ctx);

#ifdef __DEMOS__
			basic_demo(&GUI_ctx, &GUI_media);
			button_demo(&GUI_ctx, &GUI_media);
			grid_demo(&GUI_ctx, &GUI_media);
#endif

			//GUI_Settings(&GUI_ctx, &GUI_media);

			if (GUI_IsWindowOpen(&GUI_ctx, MENU_INVENTORY))
			{
				GUI_Inventory(&GUI_ctx, &GUI_media);
			}
			else if (selectedContextItem || backEnd.ui_MouseCursor > 0)
			{// Clear any seleted stuff when inventory window is closed... TODO: When I add force power inv screen, also check if it's open first...
				selectedContextItem = 0;
				backEnd.ui_MouseCursor = 0;
			}

			if (GUI_IsWindowOpen(&GUI_ctx, MENU_RADIO))
			{
				GUI_Radio(&GUI_ctx, &GUI_media);
			}

			// Quickbar is always visible...
			GUI_QuickBar(&GUI_ctx, &GUI_media);

			// Main Menu is always visible...
			GUI_MainMenu(&GUI_ctx, &GUI_media);

			if (keyStatus[A_MOUSE1] && menuNoChangeTime > backEnd.refdef.time)
			{
				mainMenuOpen = false;
			}

			if (selectedContextItem || backEnd.ui_MouseCursor > 0)
			{// If the user clicked somewhere not handled by the ui with either a drag object selected, or a right click menu open, then remove/close them... TODO: Add "trash item" here...
				if (keyStatus[A_MOUSE1])
				{
					selectedContextItem = 0;
					backEnd.ui_MouseCursor = 0;
				}
			}
		}
/*#ifndef __USE_INV__
		else
		{// Just clear the buffer...
			FBO_Bind(tr.renderGUIFbo);

			qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			qglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			qglClear(GL_COLOR_BUFFER_BIT);

			FBO_Bind(glState.previousFBO);

			GL_SetDefaultState();
		}
#endif //__USE_INV__*/
		else
		{
			// Quickbar is always visible...
			GUI_QuickBar(&GUI_ctx, &GUI_media);

			// Main Menu is always visible...
			GUI_MainMenu(&GUI_ctx, &GUI_media);

			GUI_CloseAllWindows(&GUI_ctx);

			if (mainMenuOpen || selectedContextItem || backEnd.ui_MouseCursor > 0)
			{// UI closed, disable any context menu or mouse cursor (drag/drop) settings...
				selectedContextItem = 0;
				backEnd.ui_MouseCursor = 0;
				mainMenuOpen = false;
			}
		}

		/* Draw */
		device_draw(&GUI_device, &GUI_ctx, width, height, scale, NK_ANTI_ALIASING_ON);
	}
}
