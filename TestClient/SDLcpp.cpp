#include "SDLcpp.h"

namespace SDL
{
	Error::Error() : exception(), msg(SDL_GetError())
	{
	}

	Error::Error(const std::string& m) : exception(), msg(m)
	{
	}

	Error::~Error() throw()
	{
	}

	const char* Error::what() const throw()
	{
		return msg.c_str();
	}

	System::System(Uint32 flags) throw(Error)
	{
		SDL_SetMainReady();
		if (SDL_Init(flags) != 0)
			throw Error();
	}

	System::~System()
	{
		SDL_Quit();
	}

	Window::Window(const char *title,
		int x, int y, int w,
		int h, Uint32 flags) throw(Error)
	{
		if ((window = SDL_CreateWindow(title, x, y, w, h, flags)) == NULL)
			throw Error();

		if ((glcontext = SDL_GL_CreateContext(window)) == NULL)
			throw Error();
	}

	void Window::GetSize(int* w, int* h)
	{
		SDL_GetWindowSize(window, w, h);
	}

	void Window::Swap()
	{
		SDL_GL_SwapWindow(window);
	}

	bool Window::GetWMInfo(SDL_SysWMinfo* info)
	{
		return SDL_GetWindowWMInfo(window, info) == SDL_TRUE;
	}

	Uint32 Window::GetID()
	{
		return SDL_GetWindowID(window);
	}

	Window::~Window()
	{
		if (glcontext != NULL)
			SDL_GL_DeleteContext(glcontext);

		if (window != NULL)
			SDL_DestroyWindow(window);
	}
}