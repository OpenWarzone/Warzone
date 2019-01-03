#include "include_imgui.h"
#include "imgui_api.h"

//#ifdef ES2
//	#include <GLES3/gl3.h>
//#else
//	#include <kung/include_gl.h>
//#endif

#include "include_quakegl.h"
#include "imgui_impl_idtech3_gles.h"

// externs
struct imgui_globals_s imguidata;
int imgui_ready;


//#endif


//#include <kung/imgui/dock/dock.h>
//#include <kung/imgui/dock/dock_repl.h>
//#include <kung/imgui/dock/dock_explorer.h>
//#include <kung/imgui/dock/dock_shadereditor.h>
//#include <kung/imgui/dock/dock_opsys.h>
//#include <kung/imgui/dock/dock_models.h>
//#include <kung/imgui/dock/dock_anims.h>
//#include <kung/imgui/dock/dock_sound.h>
//#include <kung/imgui/dock/dock_vars.h>
//#include <kung/imgui/dock/dock_quakeshaders.h>
//#include <kung/imgui/dock/dock_huds.h>
//#include <kung/imgui/dock/dock_node.h>
//#include <kung/imgui/dock/dock_duktape.h>
//#include <kung/imgui/dock/dock_julia.h>
//#include <kung/imgui/dock/dock_images.h>

//#include <list>
//std::list<Dock *> docks;


//extern GLuint g_FontTexture;
EXTERNC double g_Time = 0.0;
EXTERNC float g_MouseWheel = 0.0;

//void render_op_system();

//#include <kung/imgui/dock/dock_webtech.h>


CCALL void imgui_set_mousepos(int left, int top)          { imguidata.mouse_left   = left;  imguidata.mouse_top     = top;    }
CCALL void imgui_set_widthheight(int width, int height)   { imguidata.screen_width = width; imguidata.screen_height = height; }



CCALL void imgui_mouse_set_button(int button, kungbool state) { ImGuiIO& io = ImGui::GetIO(); io.MouseDown[button] = state;       }
CCALL void imgui_mouse_wheel(float wheelDelta)                { ImGuiIO& io = ImGui::GetIO(); io.MouseWheel = wheelDelta; g_MouseWheel = wheelDelta;        }
CCALL void imgui_on_key(int key, kungbool state)              { ImGuiIO& io = ImGui::GetIO(); io.KeysDown[key % 512] = state;     }
CCALL void imgui_on_shift(kungbool state)                     { ImGuiIO& io = ImGui::GetIO(); io.KeyShift = state;                }
CCALL void imgui_on_ctrl (kungbool state)                     { ImGuiIO& io = ImGui::GetIO(); io.KeyCtrl  = state;                }
CCALL void imgui_on_alt  (kungbool state)                     { ImGuiIO& io = ImGui::GetIO(); io.KeyAlt   = state;                }
CCALL void imgui_on_text (char *text)                         { ImGuiIO& io = ImGui::GetIO(); io.AddInputCharactersUTF8(text);    }
CCALL void imgui_set_ticks(unsigned int ticks)                { imguidata.global_ticks = ticks;                                   }

//CCALL void imgui_debug_axis(vec3_t *axis) {
//	ImGui::Begin("imgui_debug_axis");
//	ImGui::Text("%f, %f, %f", axis[0][0], axis[0][1], axis[0][2]);
//	ImGui::Text("%f, %f, %f", axis[1][0], axis[1][1], axis[1][2]);
//	ImGui::Text("%f, %f, %f", axis[2][0], axis[2][1], axis[2][2]);
//
//	float orientation[6];
//		// forward
//	orientation[0] = axis[0][0];
//	orientation[1] = axis[0][1];
//	orientation[2] = axis[0][2];
//	// up
//	orientation[3] = axis[2][0];
//	orientation[4] = axis[2][1];
//	orientation[5] = axis[2][2];
//
//	
//	ImGui::Text("forward: %f, %f, %f", axis[0][0], axis[0][1], axis[0][2]);
//	ImGui::Text("up: %f, %f, %f", axis[2][0], axis[2][1], axis[2][2]);
//
//	ImGui::End();
//}

CCALL void imgui_on_key_text(int key) {
	ImGuiIO& io = ImGui::GetIO();
	char blubb[2];
	blubb[0] = key;
	blubb[1] = 0;
	io.AddInputCharactersUTF8(blubb);
}

//#include "imgui_impl_sdl_gl3.h"
#include "imgui_dock.h"



//#include <imgui/imgui_console.h>
//#include <kung/include_duktape.h>


//CCALL void q3_main();


//void CG_AdjustFrom640( float *x, float *y, float *w, float *h );



CCALL void imgui_draw_char( int x, int y, int width, int height, char c) {
	
	if (imgui_ready == 0)
		return;
	//ImGui::Text("%c", c);
	ImDrawList *drawlist = ImGui::GetWindowDrawList();


	char str[2];
	str[0] = c;
	str[1] = 0;
	drawlist->AddText( ImVec2(x,y), ImColor(1.0f,1.0f,1.0f,1.0f), (const char *)str );
}

// deprecated
CCALL void imgui_init_3d() {}

#if 1
CCALL void imgui_init() {
#ifndef EMSCRIPTEN
//#ifndef ES2
//	glewInit();
//#endif
#endif
	//PHYSFS_init(NULL);
	//PHYSFS_setWriteDir("assets");
	//PHYSFS_addToSearchPath("assets", 1);

	ImGui_ImplSdlGL3_Init();
	ImGui_ImplSdlGL3_CreateDeviceObjects();
	/////////////////////////////////////////
	//// Load docks from imgui_dock.layout //
	/////////////////////////////////////////
	//LoadDock();
	//
	//
	//ilInit();
	//q3_main();



	#ifndef EMSCRIPTEN
	//js_init();
	#endif

#if 0
	js_init_full();
	root = new Node("root", "opsystem/root.opsys");
	// now root is set and we can set the javascript root node
	js_call(ctx, "node_init_root", "");
	////InitPermanentOps();

	// crashes...
	js_call(ctx, "add_hierarchy_to_root", "");
#endif
}
#endif

#if 0
CCALL void imgui_render_2() {
	    if (!g_FontTexture)
        ImGui_ImplSdlGL3_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    double current_time = ((float)imguidata.global_ticks) / 1000.0;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;
	
    io.DisplaySize = ImVec2((float)imguidata.screen_width, (float)imguidata.screen_height);
    io.MousePos = ImVec2(imguidata.mouse_left, imguidata.mouse_top);
    io.MouseWheel = g_MouseWheel;
	g_MouseWheel = 0;

	ImGui::NewFrame();

	ImGui::SetNextWindowPos( ImVec2(0,0) );
	ImGui::Begin("BCKGND", NULL, ImGui::GetIO().DisplaySize, 0.0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus );
	ImDrawList *drawlist = ImGui::GetWindowDrawList();
	drawlist->AddText( ImVec2(400,400), ImColor(1.0f,1.0f,1.0f,1.0f), "Text in Background Layer" );
	float center_w = ImGui::GetWindowWidth() / 2;
	float center_h = ImGui::GetWindowHeight() / 2;
	drawlist->AddLine(ImVec2(center_w - 20, center_h), ImVec2(center_w - 10, center_h), 0xffffffff, 2.0);
	drawlist->AddLine(ImVec2(center_w + 20, center_h), ImVec2(center_w + 10, center_h), 0xffffffff, 2.0);
	ImGui::End();
	

	qglViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	//glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	if (0) { // red bg
	//glClearColor(1.0f, 0.0f, 0.0f, 0.5f);
	//glClear(GL_COLOR_BUFFER_BIT);
	}
	
	ImGui::Render();
}
#endif

#if 1
CCALL void imgui_new_frame() {
	//if (!g_FontTexture)
	//	ImGui_ImplSdlGL3_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    double current_time = ((float)imguidata.global_ticks) / 1000.0;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;
	
    io.DisplaySize = ImVec2((float)imguidata.screen_width, (float)imguidata.screen_height);
    io.MousePos = ImVec2(imguidata.mouse_left, imguidata.mouse_top);
    io.MouseWheel = g_MouseWheel;
	g_MouseWheel = 0;

	ImGui::NewFrame();


		ImGui::SetNextWindowPos( ImVec2(0,0) );
		ImGui::SetNextWindowSize(io.DisplaySize); // this used to work, but not in latest imgui version... need to call SetWindowSize() now
		ImGui::Begin("BCKGND", NULL, ImGui::GetIO().DisplaySize, 0.0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);		
		//ImGuiWindow *bgwin = ImGui::GetCurrentWindow();
		ImGui::SetWindowSize(io.DisplaySize);
		ImDrawList *drawlist = ImGui::GetWindowDrawList();
		//drawlist->AddText( ImVec2(400,400), ImColor(1.0f,1.0f,1.0f,1.0f), "Text in Background Layer" );
		//float center_w = ImGui::GetWindowWidth() / 2;
		//float center_h = ImGui::GetWindowHeight() / 2;
		//drawlist->AddLine(ImVec2(center_w - 20, center_h), ImVec2(center_w - 10, center_h), 0xffffffff, 2.0);
		//drawlist->AddLine(ImVec2(center_w + 20, center_h), ImVec2(center_w + 10, center_h), 0xffffffff, 2.0);
		//if (leetmenu->callback_ingame)
		//	leetmenu->callback_ingame();
		//ImGui::End();

	imgui_ready = 1;
}
#endif

#if 1
CCALL void imgui_render() {
	imgui_default_docks();



}
#endif

#if 1
CCALL void imgui_end_frame() {

	// end the global background window started in imgui_new_frame()
	ImGui::End();

	
	ImGui::EndFrame();
	//return;

	#ifdef EMSCRIPTEN
		EM_ASM({
		  callback_imgui_render();
		});
	#endif	
	
#if 0
	render_op_system();
#endif

	//render_op_system();

	// only clear the screen for emscripten, otherwise we cant see what q3 renders
	// also NOT having glClearColor here causes this fucked up emscripten bug, that nothing works anymore..
#ifdef EMSCRIPTEN
	//glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	//glClearColor(0.3, 0.4, 0.5, 0.5);
	//glClear(GL_COLOR_BUFFER_BIT);
#endif
	
	// just to make it work till proper way is implemented
	//imguidata.screen_width = 800;
	//imguidata.screen_height = 600;
	imguidata.global_ticks++;
	qglViewport(0, 0, imguidata.screen_width, imguidata.screen_height);
	//glClear(GL_COLOR_BUFFER_BIT);
	qglClear(GL_DEPTH_BUFFER_BIT);
	
	
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);
	//glEnable(GL_CULL_FACE);
	//glEnable(GL_DEPTH_TEST);
	
	qglEnable(GL_BLEND);
	//glDisable(GL_BLEND);
    //qglBlendEquation(GL_FUNC_ADD);
    qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_DEPTH_TEST);
    //glEnable(GL_SCISSOR_TEST);
	//glEnable(GL_DEPTH_TEST);
	qglDisable(GL_SCISSOR_TEST);
    //qglActiveTexture(GL_TEXTURE0);


    //GLint last_program; qglGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    //GLint last_texture; qglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    //GLint last_active_texture; qglGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    //GLint last_array_buffer; qglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    //GLint last_element_array_buffer; qglGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    ////GLint last_vertex_array; qglGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    ////GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
    ////GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
    //GLint last_blend_equation_rgb; qglGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    //GLint last_blend_equation_alpha; qglGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    //GLint last_viewport[4]; qglGetIntegerv(GL_VIEWPORT, last_viewport);
    //GLint last_scissor_box[4]; qglGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
    //GLboolean last_enable_blend = qglIsEnabled(GL_BLEND);
    //GLboolean last_enable_cull_face = qglIsEnabled(GL_CULL_FACE);
    //GLboolean last_enable_depth_test = qglIsEnabled(GL_DEPTH_TEST);
    //GLboolean last_enable_scissor_test = qglIsEnabled(GL_SCISSOR_TEST);


	ImGui::Render();


	
    qglEnable(GL_CULL_FACE);
    qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);


    //// Restore modified GL state
    //qglUseProgram(last_program);
    ////qglActiveTexture(last_active_texture);
    //qglBindTexture(GL_TEXTURE_2D, last_texture);
    ////qglBindVertexArray(last_vertex_array);
    //qglBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    //qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    ////qglBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    ////glBlendFunc(last_blend_src, last_blend_dst);
    //if (last_enable_blend) qglEnable(GL_BLEND); else qglDisable(GL_BLEND);
    //if (last_enable_cull_face) qglEnable(GL_CULL_FACE); else qglDisable(GL_CULL_FACE);
    //if (last_enable_depth_test) qglEnable(GL_DEPTH_TEST); else qglDisable(GL_DEPTH_TEST);
    //if (last_enable_scissor_test) qglEnable(GL_SCISSOR_TEST); else qglDisable(GL_SCISSOR_TEST);
    //qglViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    //qglScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);

	imgui_ready = 0;
}


CCALL bool imgui_button(char *label, ImVec2 *pos) {
	// gnah, there is no "dock" in case of background imgui window... so either special case everything to check for NULL pointer or make the normal window a "dock" too
	// otherwise automatic ids work in Dock mode only and that code fucks up then in background imgui window
	#if 0
	if (g_dock.m_current) {
		// give buttons automatically a new id, so buttons with same names cause no "click not detected" conflict
		// the next_new_id is cleaned from julia, via imgui_reset_ids() ccall at end of dock rendering loop
		unsigned int newid = g_dock.m_current->next_new_id++;
		ImGui::PushID(newid);
	}
	#endif
	auto ret = ImGui::Button(label, *pos);
	#if 0
	if (g_dock.m_current)
		ImGui::PopID();
	#endif
	return ret;
}
CCALL void imgui_text_colored(ImVec4 *color, char *text)                                       {        ImGui::TextColored(*color, text);                       }
CCALL void imgui_text(char *text)                                                              {        ImGui::Text(text);                                      }
CCALL bool imgui_begin_dock(char *label, bool *pressedclose, CDock *useThisDock)         { return BeginDock(label, pressedclose, 0, useThisDock);  }
CCALL void imgui_end_dock(char *text)                                                          {        EndDock();                                       }
CCALL bool imgui_drag_float(char *label, float *v, float v_speed, float v_min, float v_max)    { return ImGui::DragFloat(label, v, v_speed, v_min, v_max);      }
CCALL void imgui_push_id(int id)                                                               {        ImGui::PushID(id);                                      }
CCALL void imgui_pop_id(int id)                                                                {        ImGui::PopID();                                         }

CCALL bool imgui_dock_closebuttonpressed(CDock *dock)                                    { return dock->closeButtonPressed;                               }
CCALL void imgui_dock_closebuttonreset(CDock *dock)                                      { dock->closeButtonPressed = false;                              }

CCALL void imgui_line(ImVec2 *a, ImVec2 *b, unsigned int color, float thickness) {
	ImGuiWindow *window = ImGui::GetCurrentWindow();

	//ImVec2 winpos = ImGui::GetWindowPos();
	*a += window->PosFloat; // make the line relative to window and scroll position
	*b += window->PosFloat;
	*a -= window->Scroll;
	*b -= window->Scroll;

	window->DrawList->AddLine(*a, *b, color, thickness);
}
CCALL void imgui_point(ImVec2 *a, float radius, unsigned int color, int num_segments) {
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	*a += window->PosFloat; // make the line relative to window and scroll position
	*a -= window->Scroll;
	window->DrawList->AddCircleFilled(*a, radius, color, num_segments);
}

CCALL void imgui_begin_group()                                                                    { ImGui::BeginGroup();               }
CCALL void imgui_end_group()                                                                      { ImGui::EndGroup();                 }
CCALL void imgui_push_item_width(float width)                                                     { ImGui::PushItemWidth(width);       }
CCALL void imgui_pop_item_width()                                                                 { ImGui::PopItemWidth();             }
CCALL void imgui_same_line(float pos_x, float spacing_w)                                          { ImGui::SameLine(pos_x, spacing_w); }


CCALL bool imgui_is_item_clicked()                                                                { return ImGui::IsItemClicked();              }
CCALL bool imgui_is_item_active()                                                                 { return ImGui::IsItemActive();               }
CCALL bool imgui_is_item_hovered()                                                                { return ImGui::IsItemHovered();              }
CCALL bool imgui_is_item_hovered_rect()                                                           { return ImGui::IsItemHoveredRect();          }
CCALL bool imgui_tree_node(char *label)                                                           { return ImGui::TreeNode(label);              }
CCALL void imgui_tree_pop()                                                                       {        ImGui::TreePop();                    }
CCALL bool imgui_checkbox(char *label, bool *v)                                                   { return ImGui::Checkbox(label, v);           }

CCALL void imgui_winsize(float *vec2)                                                             {

	*(ImVec2 *)vec2 = ImGui::GetCurrentWindowRead()->Size;
	// the window size is fucked up for floating windows, so fix it hear
	if (g_dock.m_current && g_dock.m_current->status == Status_::Status_Float)
		((ImVec2 *)vec2)->y -= 15.0;
}
CCALL void imgui_cursorpos(float *vec2)                                                           { *(ImVec2 *)vec2 = ImGui::GetCursorPos();                         }
CCALL void imgui_cursorpos_set(float *vec2)                                                       { ImGui::SetCursorPos(*(ImVec2 *)vec2);                            }
CCALL void imgui_winsizecontents(float *vec2)                                                     { *(ImVec2 *)vec2 = ImGui::GetCurrentWindowRead()->SizeContents;   }

// currdock = GetCurrentDock()
// id = GetNewID(currdock)
// PushID(id)
// PopID()
CCALL void *imgui_get_current_dock()                                                              { return g_dock.m_current;                    }
CCALL uint32_t imgui_get_new_id(CDock *dock)                                                { return dock->next_new_id++;                 }
CCALL void imgui_reset_ids(CDock *dock)                                                     { dock->next_new_id = 0;                      }


CCALL bool imgui_input_text          (char *label, char *buf, size_t buf_size,              int flags, void *callback, void *userdata)    { return ImGui::InputText(         label, buf, buf_size,                  flags, (ImGuiTextEditCallback)callback, userdata); }
CCALL bool imgui_input_text_multiline(char *label, char *buf, size_t buf_size, float *size, int flags, void *callback, void *userdata)    { return ImGui::InputTextMultiline(label, buf, buf_size, *(ImVec2 *)size, flags, (ImGuiTextEditCallback)callback, userdata); }


CCALL bool imgui_combo(char *label, int *out_clickedid, const char **strarray, int itemcount)    { return ImGui::Combo(label, out_clickedid,strarray, itemcount); }

bool             IsKeyPressedMap(ImGuiKey key, bool repeat = true);
CCALL bool imgui_pressed_ctrl_enter() { return ImGui::GetIO().KeyCtrl && IsKeyPressedMap(ImGuiKey_Enter, 0); }
CCALL bool imgui_pressed_alt_enter() { return ImGui::GetIO().KeyAlt && IsKeyPressedMap(ImGuiKey_Enter, 0); }
CCALL void imgui_get_selection(int *select_start, int *select_end) {
	ImGuiContext *g = ImGui::GetCurrentContext();
	*select_start = g->InputTextState.StbState.select_start;
	*select_end = g->InputTextState.StbState.select_end;
	if (*select_start == *select_end) {
		*select_start = g->InputTextState.StbState.cursor;
		*select_end = g->InputTextState.StbState.cursor;
	}
}

CCALL void imgui_new_line() {
	ImGui::NewLine();
}

#endif