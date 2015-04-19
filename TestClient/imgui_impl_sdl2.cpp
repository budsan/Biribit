#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include "utf8.h"
#include "OpenGL.h"
#include "SDLcpp.h"

#include <vector>

static GLuint g_FontTexture = 0;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
static void ImGui_ImplSdl2_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
	if (cmd_lists_count == 0)
		return;

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnable(GL_TEXTURE_2D);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

	// Setup orthographic projection matrix
	const float width = ImGui::GetIO().DisplaySize.x;
	const float height = ImGui::GetIO().DisplaySize.y;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	for (int n = 0; n < cmd_lists_count; n++)
	{
		const ImDrawList* cmd_list = cmd_lists[n];
		const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->vtx_buffer.front();
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

		int vtx_offset = 0;
		for (size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];
			if (pcmd->user_callback)
			{
				pcmd->user_callback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
				glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
				glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
			}
			vtx_offset += pcmd->vtx_count;
		}
	}
#undef OFFSETOF

	// Restore modified state
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindTexture(GL_TEXTURE_2D, 0);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}

static const char* ImGui_ImplSdl2_GetClipboardText()
{
	return SDL_GetClipboardText();
}

static void ImGui_ImplSdl2_SetClipboardText(const char* text)
{
	SDL_SetClipboardText(text);
}

bool ImGui_ImplSdl2_CreateDeviceObjects()
{
	ImGuiIO& io = ImGui::GetIO();

	// Build texture
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

	// Create texture
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

	return true;
}

void ImGui_ImplSdl2_InvalidateDeviceObjects()
{
	if (g_FontTexture)
	{
		glDeleteTextures(1, &g_FontTexture);
		ImGui::GetIO().Fonts->TexID = 0;
		g_FontTexture = 0;
	}
}

bool ImGui_ImplSdl2_Init(SDL::Window &window)
{
	ImGuiIO& io = ImGui::GetIO();

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = SDL_GetScancodeFromKey(SDLK_TAB);
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_GetScancodeFromKey(SDLK_LEFT);
	io.KeyMap[ImGuiKey_RightArrow] = SDL_GetScancodeFromKey(SDLK_RIGHT);
	io.KeyMap[ImGuiKey_UpArrow] = SDL_GetScancodeFromKey(SDLK_UP);
	io.KeyMap[ImGuiKey_DownArrow] = SDL_GetScancodeFromKey(SDLK_DOWN);
	io.KeyMap[ImGuiKey_Home] = SDL_GetScancodeFromKey(SDLK_HOME);
	io.KeyMap[ImGuiKey_End] = SDL_GetScancodeFromKey(SDLK_END);
	io.KeyMap[ImGuiKey_Delete] = SDL_GetScancodeFromKey(SDLK_DELETE);
	io.KeyMap[ImGuiKey_Backspace] = SDL_GetScancodeFromKey(SDLK_BACKSPACE);
	io.KeyMap[ImGuiKey_Enter] = SDL_GetScancodeFromKey(SDLK_RETURN);
	io.KeyMap[ImGuiKey_Escape] = SDL_GetScancodeFromKey(SDLK_ESCAPE);
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;

	io.RenderDrawListsFn = ImGui_ImplSdl2_RenderDrawLists;
	io.SetClipboardTextFn = ImGui_ImplSdl2_SetClipboardText;
	io.GetClipboardTextFn = ImGui_ImplSdl2_GetClipboardText;

#ifdef _MSC_VER
	SDL_SysWMinfo info;
	if (window.GetWMInfo(&info) && info.subsystem == SDL_SYSWM_WINDOWS)
		io.ImeWindowHandle = info.info.win.window;
#endif

	return true;
}

void ImGui_ImplSdl2_Shutdown()
{
	ImGui_ImplSdl2_InvalidateDeviceObjects();
	ImGui::Shutdown();
}

void ImGui_ImplSdl2_NewFrame(float deltaTime)
{
	if (!g_FontTexture)
		ImGui_ImplSdl2_CreateDeviceObjects();

	ImGuiIO& io = ImGui::GetIO();

	// Setup time step
	io.DeltaTime = deltaTime > 0 ? deltaTime : 0.001f;

	// Hide/show hardware mouse cursor
	int toggle = io.MouseDrawCursor ? 0 : 1;
	if (SDL_ShowCursor(-1) != toggle)
		SDL_ShowCursor(toggle);

	// Start the frame
	ImGui::NewFrame();
}

void ImGui_ImplSdl2_AddText(SDL_TextInputEvent& text)
{
	ImGuiIO& io = ImGui::GetIO();
	std::string str(text.text);
	std::vector<unsigned short> wide;
	utf8::utf8to16(str.begin(), str.end(), std::back_inserter(wide));
	if (!wide.empty() && wide[0] > 0 && wide[0] < 0x10000)
		io.AddInputCharacter(wide[0]);
}
