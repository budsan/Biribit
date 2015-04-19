#include "Game.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "ConsoleInterface.h"
#include "Commands.h"

#include "OpenGL.h"
#include "Generic.h"

#include "imgui_impl_sdl2.h"
#include "imgui.h"

//---------------------------------------------------------------------------//

Game::Game(SDL::Window &window) :
	window(window),
	closed(false),
	showMainMenu(true),
	showGUIs(true)
{
	Console& console = ConsoleInstance();
	CommandHandler::AddAllCommands(console, *this);
	CommandHandler::AddAllLogCallback(console);
}

Game::~Game()
{
	CommandHandler::DelAllLogCallback();
	ImGui_ImplSdl2_Shutdown();
}

bool Game::Init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

	ImGui_ImplSdl2_Init(window);

	return true;
}

void Game::Update(float deltaTime) 
{
	UpdateListeners(deltaTime);

	//--ON-GUI-------------------------------------------------------------//
	ImGui_ImplSdl2_NewFrame(deltaTime);
	{
		if (showMainMenu)
		{
			ImGui::Begin("Main menu", &showMainMenu, ImVec2(600, 400));
			ImGui::Text("Quick guide:");
			ImGui::Text("Press F1 to hide/show all GUIs.");
			ImGui::Text("Press F2 to toggle this window.");
			ImGui::Text("Press F3 to toggle console.");
			GUIMenuListeners();
			ImGui::End();
		}

		//static bool show_test_window = true;
		//ImGui::ShowTestWindow(&show_test_window);
		
		GUIListeners();
	}
}

void Game::Draw()
{
	ImGuiIO& io = ImGui::GetIO();
	
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if(showGUIs) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glClear(GL_DEPTH_BUFFER_BIT);
		ImGui::Render();
	}
}

void Game::Close()
{
	closed = true;
}

bool Game::Closed()
{
	return closed;
}

void Game::HandleEvent(SDL_Event &event)
{
	Uint32 windowID = window.GetID();
	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_WINDOWEVENT:
		if (event.window.windowID != windowID)
			break;

		switch (event.window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			io.DisplaySize = ImVec2((float)event.window.data1, (float)event.window.data2);
			glViewport(0, 0, event.window.data1, event.window.data2);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			Close();
			break;
		case SDL_WINDOWEVENT_LEAVE:
			io.MousePos = ImVec2(-1, -1);
		case SDL_WINDOWEVENT_ENTER:
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				io.MousePos = ImVec2((float)x, (float)y);
			}
		}
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (event.button.windowID != windowID)
			break;

		if (io.MousePos.x != -1)
			io.MousePos = ImVec2((float)event.button.x, (float)event.button.y);

		io.MouseDown[event.button.button - 1] = event.button.state == SDL_PRESSED;
		break;
	case SDL_MOUSEMOTION:
		if (event.motion.windowID != windowID || io.MousePos.x < 0)
			break;

		io.MousePos = ImVec2((float)event.motion.x, (float)event.motion.y);
		break;
	case SDL_MOUSEWHEEL:
		if (event.wheel.windowID != windowID)
			break;

		io.MouseWheel = (float) event.wheel.y;
		break;
	case SDL_KEYDOWN:
		if (event.key.windowID != windowID)
			break;

		switch (event.key.keysym.sym)
		{
		case SDLK_F1:
			showGUIs = !showGUIs;
			break;
		case SDLK_F2:
			if (!showGUIs) {
				showGUIs = true;
				showMainMenu = true;
			}
			else
				showMainMenu = !showMainMenu;
			break;
		case SDLK_F3:
			if (!showGUIs) {
				showGUIs = true;
				ConsoleInstance().SetDisplay(true);
			}
			else
				ConsoleInstance().ToggleDisplay();
			break;
		}

		goto sdl_keymod;
	case SDL_KEYUP:
		if (event.key.windowID != windowID)
			break;

	sdl_keymod:
		{
			SDL_Scancode key = SDL_GetScancodeFromKey(event.key.keysym.sym);
			if (key >= 0 && key < 512)
				io.KeysDown[key] = event.key.type == SDL_KEYDOWN;
		}
	
		io.KeyCtrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
		io.KeyShift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
		io.KeyAlt = (event.key.keysym.mod & KMOD_ALT) != 0;
		break;
	case SDL_TEXTINPUT:
		if (event.text.windowID != windowID)
			break;

		ImGui_ImplSdl2_AddText(event.text);
		break;
	}
}
