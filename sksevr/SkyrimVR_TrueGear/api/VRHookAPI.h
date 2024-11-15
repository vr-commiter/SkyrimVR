#pragma once

#include "skse64\openvr_1_0_12.h"

// VR input callbacks
typedef bool (*GetControllerState_CB)(vr_1_0_12::TrackedDeviceIndex_t unControllerDeviceIndex, vr_1_0_12::VRControllerState_t *pControllerState, uint32_t unControllerStateSize);
typedef vr_1_0_12::EVRCompositorError (*WaitGetPoses_CB)(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr_1_0_12::TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount,
	VR_ARRAY_COUNT(unGamePoseArrayCount) vr_1_0_12::TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount);

class OpenVRHookManagerAPI
{
public:
    virtual bool IsInitialized() = 0;

    virtual void RegisterControllerStateCB(GetControllerState_CB cbfunc) = 0;
	virtual void RegisterGetPosesCB(WaitGetPoses_CB cbfunc) = 0;
	virtual void UnregisterControllerStateCB(GetControllerState_CB cbfunc) = 0;
	virtual void UnregisterGetPosesCB(WaitGetPoses_CB cbfunc) = 0;
};