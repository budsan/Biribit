# Biribit

Generic client and server made in C++11 based on RakNet for real-time and turn-based games.

## Dependencies:
- RakNet
- Google protocol buffers

Both dependencies are present as submodules and built in CMake Biribit scripts. Unfortunately, protoc needs to be in your PATH to successfully build Biribit. For that you must build Google protocol buffers submodule and install first.
For further instructions about how build protobuf, please visit: https://github.com/google/protobuf

## Server features:
- Lightweight, able to run in low end devices like Raspberry Pi.
- Ready to run as a daemon in Linux.
- Several games can coexist in same server.
- Server controls client names to be unique. Otherwise, renames as Name1, Name2…
- Clients can communicate each other creating and joining rooms. Each room represents a game match.
- Rooms have 2 ways of communication:
- Broadcast binary data to other client in the room. Useful for real-time games.
- Append a binary data entry to journal's room. Useful for turn-based games.

## Client library features:
- Find servers in LAN.
- Supports many connections simultaneously to servers.
- Due to the nature of networking, this runs in a separate thread. Anyway, it’s designed for games and calls which poll data are designed to be called frame by frame without penalty.
- Multi-platform: Fully working in Windows, Linux and Android. Could work in more platforms easily.
- Includes C API wrapper and C# API wrapper, designed to work in Unity 3D.

### Test client
There’s a client example for testing purposes, made in SDL and imgui. I recommend to take a look at CommandsClient.cpp to get an idea of how client works.

Feel free to send me a message or an email for suggestions.
