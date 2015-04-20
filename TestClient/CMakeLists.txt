cmake_minimum_required(VERSION 2.8)

link_directories(
        ${PROJECT_BINARY_DIR}/lib
        /usr/lib
)

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

add_executable(TerminalClient.exe
        ${SYSTEM_LIB_CPPS}
	Commands.cpp
	Console.cpp
	Updater.cpp
	CursesClient.cpp
)

target_link_libraries(TestClient.exe
	rt
        pthread
        GL
        SDL2
	BiribitClient
)

target_link_libraries(TerminalClient.exe
        rt
        pthread
        BiribitClient
)
