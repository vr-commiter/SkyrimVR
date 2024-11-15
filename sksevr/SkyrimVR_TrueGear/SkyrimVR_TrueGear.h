#pragma once
#include "skse64/PapyrusNativeFunctions.h"
#include "skse64\GameReferences.h"
#include "skse64\GameRTTI.h"
#include "skse64\NiNodes.h"
#include "skse64\NiTypes.h"
#include "skse64/GameForms.h"
#include "skse64\PapyrusEvents.h"
#include "skse64\PapyrusVM.h"
#include "skse64\PapyrusForm.h"
#include "skse64\PapyrusActorValueInfo.h"
#include "skse64/GameData.h"
#include "skse64_common/Utilities.h"
#include "skse64\GameExtraData.h"

#include "EffectObject.h"
#include <list>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>

namespace SkyrimVR_TrueGear
{
	const std::string MOD_VERSION = "1.0.0";

	//float TOLERANCE = 0.00001f;

	void SendRegist();
	void SendPlay(std::string uuid);
	void SendPlayEffectByContent(std::string content);

	bool connectionCheck();
	void send(std::string request);
	
	void WsLoop();
	void CreateSystem();
	void Play(std::string event);
	void PlayAngle(std::string event,float angle,float vertial);
	void PlayRandom(const std::string& event, const std::vector<std::vector<int>>& eletricals, const std::vector<int>& randomCounts);
	void PlayLate(std::string event,int time,int count);
	void SendSeekEffectByUUid(std::string uuid);
}
