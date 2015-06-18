cmake_minimum_required(VERSION 2.8)

add_definitions(-DENABLE_IMGUI)

add_executable(TestClient
	Commands.cpp
	Commands.h
	CommandsClient.cpp
	Console.cpp
	Console.h
	ConsoleInterface.cpp
	ConsoleInterface.h
	Game.cpp
	Game.h
	imgui.cpp
	imgui.h
	imgui_impl_sdl2.cpp
	imgui_impl_sdl2.h
	OpenGL.h
	SDLClient.cpp
	SDLcpp.cpp
	SDLcpp.h
	Updater.cpp
	Updater.h
	utf8.h
)

if(SYS_OS_WINDOWS)
	set(TEST_CLIENT_LIBRARIES OpenGL32)
elseif(SYS_OS_LINUX)
	set(TEST_CLIENT_LIBRARIES rt pthread GL)
endif()

target_link_libraries(TestClient
	${TEST_CLIENT_LIBRARIES}
	SDL2
	BiribitClient
)

if(SYS_OS_UNIX)
	add_executable(TerminalClient
		Commands.cpp
		Console.cpp
		Updater.cpp
		CursesClient.cpp
	)

	target_link_libraries(TerminalClient
		rt
		pthread
		ncurses
		BiribitClient
	)
endif()
