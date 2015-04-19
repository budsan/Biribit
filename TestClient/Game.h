#pragma once

#include <string>

#include "Updater.h"
#include "SDLcpp.h"

class ConsoleInterface;
class Game : public Updater
{
public:
	Game(SDL::Window &window);
	virtual ~Game();

	bool Init();

	void Update(float deltaTime);
	void Draw();
	void Close();
	bool Closed();

	void HandleEvent(SDL_Event &event);

protected:
	SDL::Window &window;
	bool closed;
	bool showMainMenu;
	bool showGUIs;

	friend class ConsoleInterface;
};
