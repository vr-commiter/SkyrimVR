#include "skse64_common/skse_version.h"
#include <shlobj.h>
#include <intrin.h>
#include <string>
#include <xbyak/xbyak.h>

#include "Hook.h"


#include "api/higgsinterface001.h"
#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"
#include "skse64_common/BranchTrampoline.h"
#include <skse64/PapyrusEvents.h>
#include "skse64/PluginAPI.h"	


static SKSEMessagingInterface* g_messaging = NULL;
static PluginHandle					g_pluginHandle = kPluginHandle_Invalid;
static SKSEPapyrusInterface* g_papyrus = NULL;
static SKSEObjectInterface* g_object = NULL;

#pragma comment(lib, "Ws2_32.lib")
PapyrusVRAPI* g_papyrusvr;


void SetupReceptors()
{
	_MESSAGE("Building Event Sinks...");

	//Retrieve the SKSEActionEvent dispatcher
	EventDispatcherList* edl = GetEventDispatcherList();
	if (edl)
	{
		SkyrimVR_TrueGear::g_HitEventDispatcher = (EventDispatcher<TESHitEvent>*)(&(edl->unk630));
		SkyrimVR_TrueGear::g_HitEventDispatcher->AddEventSink(&SkyrimVR_TrueGear::g_HitEventHandler);

		SkyrimVR_TrueGear::g_ContainerChangedEventDispatcher = (EventDispatcher<TESContainerChangedEvent>*)(&(edl->unk370));
		SkyrimVR_TrueGear::g_ContainerChangedEventDispatcher->AddEventSink(&SkyrimVR_TrueGear::g_ContainerChangedEventHandler);

		SkyrimVR_TrueGear::g_TESEquipEventDispatcher = (EventDispatcher<TESEquipEvent>*)(&(edl->unk4D0));
		SkyrimVR_TrueGear::g_TESEquipEventDispatcher->AddEventSink(&SkyrimVR_TrueGear::g_TESEquipEventHandler);


		SkyrimVR_TrueGear::g_TESQuestStageEventDispatcher = (EventDispatcher<TESQuestStageEvent>*)(&(edl->unkAA8));
		SkyrimVR_TrueGear::g_TESQuestStageEventDispatcher->AddEventSink(&SkyrimVR_TrueGear::g_TESQuestStageEventHandler);
	}
	void* dispatchPtr = g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ActionEvent);
	SkyrimVR_TrueGear::g_skseActionEventDispatcher = (EventDispatcher<SKSEActionEvent>*)dispatchPtr;

	SkyrimVR_TrueGear::g_skseActionEventDispatcher->AddEventSink(&SkyrimVR_TrueGear::mySKSEActionEvent);

	MenuManager* menuManager = MenuManager::GetSingleton();
	if (menuManager)
		menuManager->MenuOpenCloseEventDispatcher()->AddEventSink(&SkyrimVR_TrueGear::menuEvent);
}

struct DoAddHook_Code : Xbyak::CodeGenerator
{
	DoAddHook_Code(void* buf, uintptr_t hook_OnProjectileHit) : CodeGenerator(4096, buf)
	{
		Xbyak::Label retnLabel;
		Xbyak::Label funcLabel;

		//  .text:000000014074CFBA                 lea     r9, [r13 + 0Ch]; a4
		lea(r9, ptr[r13 + 0x0C]);
		// 	.text:000000014074CFBE                 mov[rsp + 180h + a6], 0; a6  (WARNING NOTE: Stack Offsets changed to 0x28, 0x20 fix Impact VFX bug in VR but this may be totally wrong in SE!!)
		mov(byte[rsp + 0x28], 0);
		// 	.text:000000014074CFC3                 mov[rsp + 180h + a5], ecx; a5
		mov(dword[rsp + 0x20], ecx);
		// 	.text:000000014074CFC7                 mov     r8, r13; a3
		mov(r8, r13);
		// 	.text:000000014074CFCA                 mov     rdx, rbx; a2
		mov(rdx, rbx);
		// 	.text:000000014074CFCD                 mov     rcx, r14; a1
		mov(rcx, r14);
		// 	.text:000000014074CFD0                 call    sub_140753670
		// int64_t OnProjectileHitFunctionHooked(Projectile * akProjectile, TESObjectREFR * akTarget, NiPoint3 * point, UInt32 unk1, UInt32 unk2, UInt8 unk3)
		call(ptr[rip + funcLabel]);
		// 0x1B
		//  .text:000000014074CFD5                 lea     r8, [r14+0F0h]			
		// exit 74CFD5
		jmp(ptr[Xbyak::util::rip + retnLabel]);

		L(funcLabel);
		dq(hook_OnProjectileHit);

		L(retnLabel);
		dq(SkyrimVR_TrueGear::OnProjectileHitHookLocation.GetUIntPtr() + 0x1B);
	}
};

extern "C" {

	bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {	// Called by SKSE to learn about this plugin and check that it's safe to load it
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim VR\\SKSE\\SkyrimVR_TrueGearPlugin.log");
		gLog.SetPrintLevel(IDebugLog::kLevel_Error);
		gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

		std::string logMsg("TrueGear Mod: ");
		logMsg.append("1.0.0");
		_MESSAGE(logMsg.c_str());

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "SkyrimVR_TrueGear";
		info->version = 0x010000;

		// store plugin handle so we can identify ourselves later
		g_pluginHandle = skse->GetPluginHandle();

		std::string skseVers = "SKSE Version: ";
		skseVers += std::to_string(skse->runtimeVersion);
		_MESSAGE(skseVers.c_str());

		if (skse->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");

			return false;
		}
		else if (skse->runtimeVersion < CURRENT_RELEASE_RUNTIME)
		{
			_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);

			return false;
		}
		SkyrimVR_TrueGear::g_trampolineInterface = static_cast<SKSETrampolineInterface*>(skse->QueryInterface(kInterface_Trampoline));
		if (!SkyrimVR_TrueGear::g_trampolineInterface)
		{
			_MESSAGE("WARNING: Could not get new trampoline alloc interface, Using legacy SKSE VR");
		}

		// ### do not do anything else in this callback
		// ### only fill out PluginInfo and return true/false

		// supported runtime version
		return true;
	}

	//Listener for PapyrusVR Messages
	void OnPapyrusVRMessage(SKSEMessagingInterface::Message* msg)
	{
		if (msg)
		{
			if (msg->type == kPapyrusVR_Message_Init && msg->data)
			{
				_MESSAGE("SkyrimVRTools Init Message recived with valid data, registering for callback");
				g_papyrusvr = (PapyrusVRAPI*)msg->data;

				PapyrusVR::OnVRButtonEvent buttonListener = SkyrimVR_TrueGear::OnVRButtonEventListener;
				g_papyrusvr->GetVRManager()->RegisterVRButtonListener(buttonListener);

				PapyrusVR::OnVRHapticEvent hapticListener = SkyrimVR_TrueGear::OnVRHapticEventListener;
				g_papyrusvr->GetVRManager()->RegisterVRHapticListener(hapticListener);
			}
		}
	}

	inline bool file_exists(const std::string& name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	static const size_t TRAMPOLINE_SIZE = 256;

	//Listener for SKSE Messages
	void OnSKSEMessage(SKSEMessagingInterface::Message* msg)
	{
		if (msg)
		{
			if (msg->type == SKSEMessagingInterface::kMessage_PostLoad)
			{
				_MESSAGE("SKSE PostLoad recieved, registering for SkyrimVRTools messages");
				g_messaging->RegisterListener(g_pluginHandle, "SkyrimVRTools", OnPapyrusVRMessage);

			}
			else if (msg->type == SKSEMessagingInterface::kMessage_PostPostLoad)
			{
				HiggsPluginAPI::IHiggsInterface001* higgsInterface = HiggsPluginAPI::GetHiggsInterface001(g_pluginHandle, g_messaging);
				if (higgsInterface)
				{
					_MESSAGE("Got HIGGS interface");
					higgsInterface->AddPulledCallback(SkyrimVR_TrueGear::HiggsPull);
					higgsInterface->AddStashedCallback(SkyrimVR_TrueGear::HiggsStash);
					higgsInterface->AddConsumedCallback(SkyrimVR_TrueGear::HiggsConsume);
					higgsInterface->AddCollisionCallback(SkyrimVR_TrueGear::HiggsCollide);
				}
				else
				{
					_MESSAGE("Did not get HIGGS interface");
				}
			}
			else if (msg->type == SKSEMessagingInterface::kMessage_InputLoaded)
				SetupReceptors();
			else if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded)
			{
				SkyrimVR_TrueGear::GameLoad();

				std::string	runtimeDirectory = GetRuntimeDirectory();
				if (!runtimeDirectory.empty())
				{
					SkyrimVR_TrueGear::locationalDamageInstalled = (file_exists(runtimeDirectory + "Data\\SKSE\\Plugins\\LocationalDamageSKSEVR.dll"));

					_MESSAGE("Locational Damage VR is %s.", SkyrimVR_TrueGear::locationalDamageInstalled ? "installed" : "not installed");
				}

				if (SkyrimVR_TrueGear::locationalDamageInstalled)
				{
					//Only do this if locational damage is installed.
					UInt64* currentCodex = RelocAddr<UInt64*>(ONPROJECTILEHIT_HOOKLOCATION);
					UInt64 currentCode = *currentCodex;
					currentCode = currentCode & 0xffffffffffff;

					UInt64 curCode2 = (currentCode >> 16) & 0xffffffff;

					SInt32 relativeAddress = (SInt32)curCode2;

					SInt64 superRelativeAddress = (SInt64)relativeAddress;

					UInt64* currentCodexSecond = RelocAddr<UInt64*>(ONPROJECTILEHIT_HOOKLOCATION + superRelativeAddress + 0x6);
					UInt64 currentCodeSecond = *currentCodexSecond;
					_MESSAGE("directAddress is %llx", currentCodeSecond);

					SkyrimVR_TrueGear::directAddressX = currentCodeSecond;
				}

				// NEW SKSEVR feature: trampoline interface object from QueryInterface() - Use SKSE existing process code memory pool - allow Skyrim to run without ASLR
				if (SkyrimVR_TrueGear::g_trampolineInterface)
				{
					void* branch = SkyrimVR_TrueGear::g_trampolineInterface->AllocateFromBranchPool(g_pluginHandle, TRAMPOLINE_SIZE);
					if (!branch) {
						_ERROR("couldn't acquire branch trampoline from SKSE. this is fatal. skipping remainder of init process.");
						return;
					}

					g_branchTrampoline.SetBase(TRAMPOLINE_SIZE, branch);

					void* local = SkyrimVR_TrueGear::g_trampolineInterface->AllocateFromLocalPool(g_pluginHandle, TRAMPOLINE_SIZE);
					if (!local) {
						_ERROR("couldn't acquire codegen buffer from SKSE. this is fatal. skipping remainder of init process.");
						return;
					}

					g_localTrampoline.SetBase(TRAMPOLINE_SIZE, local);

					_MESSAGE("Using new SKSEVR trampoline interface memory pool alloc for codegen buffers.");
				}
				else  // otherwise if using an older SKSEVR version, fall back to old code
				{

					if (!g_branchTrampoline.Create(TRAMPOLINE_SIZE))  // don't need such large buffers
					{
						_FATALERROR("[ERROR] couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
						return;
					}

					if (!g_localTrampoline.Create(TRAMPOLINE_SIZE, nullptr))
					{
						_FATALERROR("[ERROR] couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
						return;
					}

					_MESSAGE("Using legacy SKSE trampoline creation.");
				}

				//SkyrimVR_TrueGear::SkyVRaanHooks();

				if (!SkyrimVR_TrueGear::locationalDamageInstalled)
				{
					//Only do this if locational damage is not installed.

					void* codeBuf = g_localTrampoline.StartAlloc();
					DoAddHook_Code code(codeBuf, (uintptr_t)SkyrimVR_TrueGear::OnProjectileHitFunctionHooked);
					g_localTrampoline.EndAlloc(code.getCurr());

					g_branchTrampoline.Write6Branch(SkyrimVR_TrueGear::OnProjectileHitHookLocation.GetUIntPtr(), uintptr_t(code.getCode()));

					_MESSAGE("Code hooked successfully!");
				}
			}
		}
	}

	bool SKSEPlugin_Load(const SKSEInterface* skse) {	// Called by SKSE to load this plugin
		_MESSAGE("SkyrimVR_TrueGearPluginScript loaded");

		g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);

		g_messaging = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
		g_messaging->RegisterListener(g_pluginHandle, "SKSE", OnSKSEMessage);

		//Check if the function registration was a success...
		bool bSuccess = g_papyrus->Register(SkyrimVR_TrueGear::RegisterFuncs);

		if (bSuccess) {
			_MESSAGE("Register Succeeded");
		}


		return true;
	}
};