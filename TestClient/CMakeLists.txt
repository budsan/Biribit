cmake_minimum_required(VERSION 2.8)

add_definitions(-DENABLE_IMGUI)

add_executable(TestClient.exe
	${SYSTEM_LIB_CPPS}
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

target_link_libraries(TestClient.exe
	${TEST_CLIENT_LIBRARIES}
	SDL2
	BiribitClient
)

if(SYS_OS_LINUX)
	add_executable(TerminalClient.exe
		${SYSTEM_LIB_CPPS}
		Commands.cpp
		Console.cpp
		Updater.cpp
		CursesClient.cpp
	)

	target_link_libraries(TerminalClient.exe
		rt
		pthread
		ncurses
		BiribitClient
	)
endif()
