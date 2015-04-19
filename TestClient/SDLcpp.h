#pragma once

#include <exception>
#include <string>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

namespace SDL
{
	class Error : public std::exception
	{
	public:
		Error();
		Error(const std::string&);
		virtual ~Error() throw();
		virtual const char* what() const throw();
	private:
		std::string msg;
	};

	class System
	{
	public:
		System(Uint32 flags = 0) throw(Error);
		virtual ~System();
	};

	class Window
	{
	public:
		Window(const char *title,
			int x, int y, int w,
			int h, Uint32 flags) throw(Error);

		virtual ~Window();

		void GetSize(int* w, int* h);
		void Swap();

		bool GetWMInfo(SDL_SysWMinfo* info);
		Uint32 GetID();

	private:
		SDL_Window* window;
		SDL_GLContext glcontext;
	};
}
