#pragma once

#include <set>

class Updater;
class UpdaterListener
{
public:
	virtual ~UpdaterListener();

private:

	friend class Updater;
	virtual void OnUpdate(float deltaTime) = 0;
	virtual void OnGUI() {}
	virtual bool OnGUIMenu() {return false;}
	virtual int Priority() const {return 0;}

	friend struct UpdaterListenerCompare;

	std::set<Updater*> updaters;
};

struct UpdaterListenerCompare
{
	bool operator() (UpdaterListener* const a, UpdaterListener* const b) const
	{
		if (a->Priority() == b->Priority())
			return a < b;
		else
			return a->Priority() > b->Priority();
	}
};

class Updater
{
public:
	virtual ~Updater();

	bool AddUpdaterListener(UpdaterListener* listener);
	bool DelUpdaterListener(UpdaterListener* listener);

protected:
	
	void UpdateListeners(float deltaTime);
	void GUIListeners();
	void GUIMenuListeners();

private:

	std::set<UpdaterListener*, UpdaterListenerCompare> listeners;
};

