#include <iostream>
#include <exception>
#include <string>

#include "SDLcpp.h"
#include "OpenGL.h"

#include "Game.h"

int main()
{
	try
	{
		SDL::System sdl(SDL_INIT_VIDEO | SDL_INIT_TIMER);
		SDL::Window window("Biribit test client",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280,
			800,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_OPENGL);

		Game game(window);
		if (!game.Init())
			throw SDL::Error("Game wasn't able to init.");

		bool open = true;
		unsigned int lastTime = SDL_GetTicks(), currentTime;
		while (!game.Closed())
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
				game.HandleEvent(event);

			currentTime = SDL_GetTicks();
			float deltaTime = (currentTime - lastTime) * (1 / 1000.0f);
			lastTime = currentTime;

			game.Update(deltaTime);
			game.Draw();
			window.Swap();
		}
	}
	catch (const SDL::Error& err)
	{
		std::cerr
			<< "Error while initializing SDL:  "
			<< err.what() << std::endl;
	}

	return 0;
}
