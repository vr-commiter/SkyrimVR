#pragma once
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se\GameReferences.h"
#include "f4se\GameRTTI.h"
#include "f4se\NiNodes.h"
#include "f4se\NiTypes.h"
#include "f4se/GameForms.h"
#include "f4se\PapyrusEvents.h"
#include "f4se\PapyrusVM.h"
#include "f4se\PapyrusForm.h"
#include "f4se/GameData.h"
#include "f4se_common/Utilities.h"
#include "f4se\GameExtraData.h"
#include "f4se/GameAPI.h"

#include "EffectObject.h"
#include <list>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>



namespace F4VR_TrueGear
{
	const std::string MOD_VERSION = "1.0.0";

	//float TOLERANCE = 0.00001f;

	void SendRegist();

	void SendPlay(std::string uuid);

	void SendPlayEffectByContent(std::string content);

	void SendSeekEffectByUUid(std::string content);

	void PreSeekEffect(std::string uuid);

	EffectObject FindEffectByUuid(std::string uuid);
	bool connectionCheck();
	void send(char* request);
	void CreateSystem();

	void WsLoop();
	void Play(std::string event);
	void PlayAngle(std::string event,float angle,float vertial);
	void PlayRandom(const std::string& event, const std::vector<std::vector<int>>& eletricals, const std::vector<int>& randomCounts);
	void PlayLate(std::string event,int time,int count);
	void rece_message();
}
