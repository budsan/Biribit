#include "Updater.h"

Updater::~Updater()
{
	for(auto it = listeners.begin(); it != listeners.end(); it++) {
		(*it)->updaters.erase(this);
	}
}

bool Updater::AddUpdaterListener(UpdaterListener* listener)
{
	auto pair = listeners.insert(listener);
	if (!pair.second) {
		return false;
	}

	listener->updaters.insert(this);
	return false;
}

bool Updater::DelUpdaterListener(UpdaterListener* listener)
{
	std::size_t items = listeners.erase(listener);
	if (items == 0) {
		return false;
	}

	listener->updaters.erase(this);
	return true;
}

void Updater::UpdateListeners(float deltaTime)
{
	for (auto it = listeners.begin(); it != listeners.end(); ++it) {
		(*it)->OnUpdate(deltaTime);
	}
}

UpdaterListener::~UpdaterListener()
{
	while(!updaters.empty())
		(*updaters.begin())->DelUpdaterListener(this);
}


void Updater::GUIListeners()
{
	for (auto it = listeners.begin(); it != listeners.end(); ++it) {
		(*it)->OnGUI();
	}
}

void Updater::GUIMenuListeners()
{
	bool last = true;
	for (auto it = listeners.begin(); it != listeners.end(); ++it) {
		last = (*it)->OnGUIMenu();
	}
}
