cmake_minimum_required(VERSION 2.8)

add_definitions(-DENABLE_IMGUI)

add_executable(TestClient
	Commands.cpp
	CommandsClient.cpp
	Console.cpp
	ConsoleInterface.cpp
	Game.cpp
	imgui.cpp
	imgui_impl_sdl2.cpp
	SDLClient.cpp
	SDLcpp.cpp
	Updater.cpp
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
	BiribitForUnity
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
