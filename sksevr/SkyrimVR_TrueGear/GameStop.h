#pragma once
#include "skse64/GameReferences.h"

#include "skse64/PapyrusVM.h"
#include <unordered_map>

#include <atomic>

namespace SkyrimVR_TrueGear
{
	extern std::vector<std::string> gameStoppingMenus;

	extern std::vector<std::string> gameStoppingMenusNoDialogue;

	extern std::unordered_map<std::string, bool> menuTypes;

	bool isGameStopped();

	bool isGameStoppedNoDialogue();

	class AllMenuEventHandler : public BSTEventSink <MenuOpenCloseEvent> {
	public:
		virtual EventResult	ReceiveEvent(MenuOpenCloseEvent* evn, EventDispatcher<MenuOpenCloseEvent>* dispatcher);
	};

	extern AllMenuEventHandler menuEvent;

	extern std::atomic<bool> loadingMenuJustClosed;
}