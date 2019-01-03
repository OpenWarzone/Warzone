// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui.h"
#include "imgui_api.h"
#include "imgui_impl_idtech3_gles.h"

#include "include_quakegl.h"

//#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so. 
// If text or lines are blurry when integrating ImGui in your engine: in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplSdlGL3_RenderDrawLists(ImDrawData* draw_data)
{
	//return;
	
	//int lastProgram = glDsaState.program;
	int lastProgram = 0;

	//GLuint tmu = glDsaState.texunit - GL_TEXTURE0;
	//if (glDsaState.texunit < GL_TEXTURE0) // then quake isnt called yet, it will be 0
	//	tmu = GL_TEXTURE0;
	//GLuint lastTexture = glDsaState.textures[tmu];
	////int lastTexture = glDsaState.textures[0];
	
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

#if 0

	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLboolean last_enable_blend = qglIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = qglIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = qglIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = qglIsEnabled(GL_SCISSOR_TEST);

	// and ...

	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	if (last_enable_blend) qglEnable(GL_BLEND); else qglDisable(GL_BLEND);
	if (last_enable_cull_face) qglEnable(GL_CULL_FACE); else qglDisable(GL_CULL_FACE);
	if (last_enable_depth_test) qglEnable(GL_DEPTH_TEST); else qglDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) qglEnable(GL_SCISSOR_TEST); else qglDisable(GL_SCISSOR_TEST);
#endif
	// Backup GL state
	// commented stuff is not available in WebGL

// fuck it, huge performance drain in webgl

//#ifndef EMSCRIPTEN
//    GLint last_program; qglGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
//    GLint last_texture; qglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
//    GLint last_active_texture; qglGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
//    GLint last_array_buffer; qglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
//    GLint last_element_array_buffer; qglGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
//    //GLint last_vertex_array; qglGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
//    //GLint last_blend_src; qglGetIntegerv(GL_BLEND_SRC, &last_blend_src);
//    //GLint last_blend_dst; qglGetIntegerv(GL_BLEND_DST, &last_blend_dst);
//    GLint last_blend_equation_rgb; qglGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
//    GLint last_blend_equation_alpha; qglGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
//#endif
//    GLint last_viewport[4]; qglGetIntegerv(GL_VIEWPORT, last_viewport);
//    GLint last_scissor_box[4]; qglGetIntegerv(GL_SCISSOR_BOX, last_scissor_box); 
//    GLboolean last_enable_blend = qglIsEnabled(GL_BLEND);
//    GLboolean last_enable_cull_face = qglIsEnabled(GL_CULL_FACE);
//    GLboolean last_enable_depth_test = qglIsEnabled(GL_DEPTH_TEST);
//    GLboolean last_enable_scissor_test = qglIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    qglEnable(GL_BLEND);
   // qglBlendEquation(GL_FUNC_ADD);
    qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_DEPTH_TEST);
    qglEnable(GL_SCISSOR_TEST);
    //qglActiveTexture(GL_TEXTURE0);


    // Setup orthographic projection matrix
    qglViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };

	//GL_UseProgram(g_ShaderHandle);
    qglUseProgram(g_ShaderHandle);

    qglUniform1i(g_AttribLocationTex, 0);
    qglUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);


	//return;

//#ifndef EMSCRIPTEN
    qglBindVertexArray(g_VaoHandle);
//#endif

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        qglBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        qglBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        qglBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                qglBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                qglScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                qglDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

// triangle rendering works nicely here
#if 0
	//glEnable(GL_BLEND);
	qglDisable(GL_BLEND);
    qglBlendEquation(GL_FUNC_ADD);
    qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglDisable(GL_CULL_FACE);
    qglDisable(GL_DEPTH_TEST);
    //qglEnable(GL_SCISSOR_TEST);
	
	qglDisable(GL_SCISSOR_TEST);
    qglActiveTexture(GL_TEXTURE0);

	//simpletriangle();
	//RenderDebugLines();
#endif

//    // Restore modified GL state
//#ifndef EMSCRIPTEN
//    qglUseProgram(last_program);
//    //qglActiveTexture(last_active_texture);
//    qglBindTexture(GL_TEXTURE_2D, last_texture);
//    //qglBindVertexArray(last_vertex_array);
//    qglBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
//    qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
//    //glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
//    //glBlendFunc(last_blend_src, last_blend_dst);
//#endif
//    if (last_enable_blend) qglEnable(GL_BLEND); else qglDisable(GL_BLEND);
//    if (last_enable_cull_face) qglEnable(GL_CULL_FACE); else qglDisable(GL_CULL_FACE);
//    if (last_enable_depth_test) qglEnable(GL_DEPTH_TEST); else qglDisable(GL_DEPTH_TEST);
//    if (last_enable_scissor_test) qglEnable(GL_SCISSOR_TEST); else qglDisable(GL_SCISSOR_TEST);
//    qglViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
//    qglScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);

	// make sure nobody fucks us up
	//qglBindVertexArray(0);
	// srsly need some kind of sane abstraction, directly debuggable from imgui... this shit took way too long to figure out
	//GL_BindNullProgram();
	//GL_BindNullTextures();

#if 0
	GL_UseProgram(lastProgram);
	if (glDsaState.texunit != 0)
		GL_BindMultiTexture(glDsaState.texunit, GL_TEXTURE_2D, lastTexture);
	R_BindNullVao();
#endif

	//qglBindBuffer(GL_ARRAY_BUFFER, 0);
	//qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	qglEnable(GL_DEPTH_TEST);
	//qglFrontFace(GL_CW);
}

char *clipboard_text = NULL;
CCALL void set_clipboard_text(char *text) {
	clipboard_text = text;
}

const char* ImGui_ImplSdlGL3_GetClipboardText(void*) {
//#if !_WINDOWS
//	return clipboard_text;
//#else
//    return SDL_GetClipboardText();
//#endif
	return "todo";
}

void ImGui_ImplSdlGL3_SetClipboardText(void*, const char* text) {
	//imgui_log("ImGui_ImplSdlGL3_SetClipboardText...\n");
#ifdef EMSCRIPTEN
			EM_ASM_({
			  copy_to_clipboard($0);
			}, text);	
#else
    //SDL_SetClipboardText(text);
#endif
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
#if 0
bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event->type)
    {
    case SDL_MOUSEWHEEL:
        {
            if (event->wheel.y > 0)
				imgui_mouse_wheel(1);
            if (event->wheel.y < 0)
                imgui_mouse_wheel(-1);
            return true;
        }
    case SDL_MOUSEBUTTONDOWN:
        {
            if (event->button.button == SDL_BUTTON_LEFT) imgui_mouse_set_button(0, true);
            if (event->button.button == SDL_BUTTON_RIGHT) imgui_mouse_set_button(1, true);
            if (event->button.button == SDL_BUTTON_MIDDLE) imgui_mouse_set_button(2, true);
            return true;
        }
    case SDL_MOUSEBUTTONUP:
        {
            if (event->button.button == SDL_BUTTON_LEFT  ) imgui_mouse_set_button(0, false);
            if (event->button.button == SDL_BUTTON_RIGHT ) imgui_mouse_set_button(1, false);
            if (event->button.button == SDL_BUTTON_MIDDLE) imgui_mouse_set_button(2, false);
            return true;
        }
    case SDL_TEXTINPUT:
        {
			imgui_on_text(event->text.text);
            return true;
        }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            int key = event->key.keysym.sym & ~SDLK_SCANCODE_MASK;
            //io.KeysDown[key] = (event->type == SDL_KEYDOWN);
			imgui_on_key(key, event->type == SDL_KEYDOWN);
			imgui_on_ctrl  ((SDL_GetModState() & KMOD_CTRL ) != 0);
			imgui_on_alt   ((SDL_GetModState() & KMOD_ALT  ) != 0);
			imgui_on_shift ((SDL_GetModState() & KMOD_SHIFT) != 0);
            //io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0); // win key
            return true;
        }
    }
    return false;
}
#endif

void ImGui_ImplSdlGL3_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    // Upload texture to graphics system
#ifndef EMSCRIPTEN
    GLint last_texture;
    qglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
#endif
    qglGenTextures(1, &g_FontTexture);
    qglBindTexture(GL_TEXTURE_2D, g_FontTexture);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifndef EMSCRIPTEN
    //qglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
#ifndef EMSCRIPTEN
    qglBindTexture(GL_TEXTURE_2D, last_texture);
#endif
}


CCALL void imgui_create_font() {
	ImGui_ImplSdlGL3_CreateFontsTexture();
}

bool ImGui_ImplSdlGL3_CreateDeviceObjects()
{
    // Backup GL state
	GLint last_texture, last_array_buffer;// , last_vertex_array;
    qglGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    qglGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    //glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar *vertex_shader =
#ifdef __EMSCRIPTEN__
		"precision highp float;\n"
#else
		"#version 330\n"
#endif
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "	Frag_UV = UV;\n"
        "	Frag_Color = Color;\n"
        "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
#ifdef __EMSCRIPTEN__
		"precision mediump float;\n"
#else
		"#version 330\n"
#endif
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";


#if EMSCRIPTEN
		vertex_shader =
		"precision highp float;\n"
		"uniform mat4 ProjMtx;\n"
		"attribute vec2 Position;\n"
		"attribute vec2 UV;\n"
		"attribute vec4 Color;\n"
		"varying vec2 Frag_UV;\n"
		"varying vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

		fragment_shader =
		"precision mediump float;\n"
		"uniform sampler2D Texture;\n"
		"varying vec2 Frag_UV;\n"
		"varying vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = Frag_Color * texture2D( Texture, Frag_UV.st);\n"
		"}\n";
#endif

    g_ShaderHandle = qglCreateProgram();
    g_VertHandle = qglCreateShader(GL_VERTEX_SHADER);
    g_FragHandle = qglCreateShader(GL_FRAGMENT_SHADER);
    qglShaderSource(g_VertHandle, 1, &vertex_shader, 0);
    qglShaderSource(g_FragHandle, 1, &fragment_shader, 0);
    qglCompileShader(g_VertHandle);
    qglCompileShader(g_FragHandle);
    qglAttachShader(g_ShaderHandle, g_VertHandle);
    qglAttachShader(g_ShaderHandle, g_FragHandle);
    qglLinkProgram(g_ShaderHandle);

    g_AttribLocationTex = qglGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = qglGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = qglGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = qglGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = qglGetAttribLocation(g_ShaderHandle, "Color");

    qglGenBuffers(1, &g_VboHandle);
    qglGenBuffers(1, &g_ElementsHandle);

//#ifndef EMSCRIPTEN
    qglGenVertexArrays(1, &g_VaoHandle);
    qglBindVertexArray(g_VaoHandle);
//#endif
    qglBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    qglEnableVertexAttribArray(g_AttribLocationPosition);
    qglEnableVertexAttribArray(g_AttribLocationUV);
    qglEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    qglVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    qglVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    qglVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

    ImGui_ImplSdlGL3_CreateFontsTexture();

	

    // Restore modified GL state
    qglBindTexture(GL_TEXTURE_2D, last_texture);
    qglBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    //qglBindVertexArray(0);
    //qglDisableVertexAttribArray(g_AttribLocationPosition);
    //qglDisableVertexAttribArray(g_AttribLocationUV);
    //qglDisableVertexAttribArray(g_AttribLocationColor);

    return true;
}

void    ImGui_ImplSdlGL3_InvalidateDeviceObjects()
{
    if (g_VaoHandle) qglDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) qglDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) qglDeleteBuffers(1, &g_ElementsHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

    if (g_ShaderHandle && g_VertHandle) qglDetachShader(g_ShaderHandle, g_VertHandle);
    if (g_VertHandle) qglDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    if (g_ShaderHandle && g_FragHandle) qglDetachShader(g_ShaderHandle, g_FragHandle);
    if (g_FragHandle) qglDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    if (g_ShaderHandle) qglDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        qglDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }
}

#include "ui/keycodes.h"
bool    ImGui_ImplSdlGL3_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    //io.KeyMap[ImGuiKey_Tab] = A_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = A_CURSOR_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = A_CURSOR_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = A_CURSOR_UP;
    io.KeyMap[ImGuiKey_DownArrow] = A_CURSOR_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = A_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = A_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = A_HOME;
    io.KeyMap[ImGuiKey_End] = A_END;
    io.KeyMap[ImGuiKey_Delete] = A_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = A_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = A_ENTER;
    io.KeyMap[ImGuiKey_Escape] = A_ESCAPE;
    io.KeyMap[ImGuiKey_A] = A_CAP_A;
    io.KeyMap[ImGuiKey_C] = A_CAP_C;
    io.KeyMap[ImGuiKey_V] = A_CAP_V;
    io.KeyMap[ImGuiKey_X] = A_CAP_X;
    io.KeyMap[ImGuiKey_Y] = A_CAP_Y;
    io.KeyMap[ImGuiKey_Z] = A_CAP_Z;

    io.RenderDrawListsFn = ImGui_ImplSdlGL3_RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = ImGui_ImplSdlGL3_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSdlGL3_GetClipboardText;
    io.ClipboardUserData = NULL;

//#ifdef _WIN32
//    SDL_SysWMinfo wmInfo;
//    SDL_VERSION(&wmInfo.version);
//    SDL_GetWindowWMInfo(window, &wmInfo);
//    io.ImeWindowHandle = wmInfo.info.win.window;
//#else
//    (void)window;
//#endif

    return true;
}

void ImGui_ImplSdlGL3_Shutdown()
{
    ImGui_ImplSdlGL3_InvalidateDeviceObjects();
    ImGui::Shutdown();
}
#if 0
void ImGui_ImplSdlGL3_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplSdlGL3_CreateDeviceObjects();

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    Uint32	time = SDL_GetTicks();
    double current_time = time / 1000.0;
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
    int mx, my;
    Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MOUSE_FOCUS)
        io.MousePos = ImVec2((float)mx, (float)my);
    else
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;		// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

    // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
    ImGui::NewFrame();
}
#endif