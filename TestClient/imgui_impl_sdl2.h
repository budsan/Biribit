#pragma once

namespace SDL
{
	class Window;
}

struct SDL_TextInputEvent;

bool ImGui_ImplSdl2_Init(SDL::Window &window);
void ImGui_ImplSdl2_Shutdown();
void ImGui_ImplSdl2_NewFrame(float deltaTime);

void ImGui_ImplSdl2_InvalidateDeviceObjects();
bool ImGui_ImplSdl2_CreateDeviceObjects();

void ImGui_ImplSdl2_AddText(SDL_TextInputEvent& text);

