#include "libovrwrapper.h"
#include <OVR.h>
#include <sixense_utils/derivatives.hpp>
#include <sixense_utils/button_states.hpp>
#include <sixense_utils/event_triggers.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>

using namespace OVR;

// Ptr<> AddRef'ed, AutoCleaned
int                          bInited = 0;
HMDInfo                      pDevInfo;
Ptr<DeviceManager>           pManager;
Ptr<HMDDevice>               pHMD;
Ptr<SensorDevice>            pSensor;
SensorFusion                 pSensorFusion;
Util::Render::StereoConfig   pStereo;


void SIXcontroller_manager_setup_callback( sixenseUtils::ControllerManager::setup_step step ) 
{
	if (sixenseUtils::getTheControllerManager()->isMenuVisible()) 
	{
		// Turn on the flag that tells the graphics system to draw the instruction screen instead of the controller information. The game
		// should be paused at this time.
		// Ask the controller manager what the next instruction string should be.
//		controller_manager_text_string = sixenseUtils::getTheControllerManager()->getStepString();

		// We could also load the supplied controllermanager textures using the filename: sixenseUtils::getTheControllerManager()->getTextureFileName();
	} 
	else 
	{
		// We're done with the setup, so hide the instruction screen.
	}
}



LIBOVRWRAPPER_API int OVR_Init()
{
	bInited = 0;
	sixenseInit();
	sixenseUtils::getTheControllerManager()->setGameType(sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER);
	sixenseUtils::getTheControllerManager()->registerSetupCallback(SIXcontroller_manager_setup_callback);
	//sixenseSetFilterParams(0.0f,  1.0f, 0.5f, 0.5f);
	sixenseSetFilterEnabled(0);

	System::Init(Log::ConfigureDefaultLog(OVR::LogMask_All));

	if (System::IsInitialized())
	{
		int stage = -1;
		while (++stage > -1 && !bInited)
		{
			switch (stage)
			{
				case 0:
					pManager = *DeviceManager::Create();
					if (pManager == NULL)
						return bInited;
					else
						bInited = 1;
					break;
				case 1:
					pHMD     = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();					
					if (pHMD == NULL)
						return bInited;
					else
						bInited = 2;
					break;
				case 2:
					pSensor = *pHMD->GetSensor();
					if (pSensor == NULL)
						return bInited;
					else
						bInited = 3;
					break;
				case 3:
					pHMD->GetDeviceInfo(&pDevInfo);
					bInited = 4;
					break;
				default:
					//bInited = true;
					bInited = 5;
					break;
			};
		}
	}

	pSensorFusion.AttachToSensor(pSensor);

	return bInited;// (bInited ? 1 : 0);
}


LIBOVRWRAPPER_API void OVR_Exit()
{
	pSensor.Clear();
	pHMD.Clear();
	pManager->Release();
	System::Destroy();
	sixenseExit();
	bInited = false;
}


LIBOVRWRAPPER_API int OVR_QueryHMD(OVR_HMDInfo* refHmdInfo)
{
	if (!bInited)
	{
		return 0;
	}

	HMDInfo src;
	if (pHMD->GetDeviceInfo(&src))
	{
		refHmdInfo->HResolution             = src.HResolution;
        refHmdInfo->VResolution             = src.VResolution;
        refHmdInfo->HScreenSize             = src.HScreenSize;
        refHmdInfo->VScreenSize             = src.VScreenSize;
        refHmdInfo->VScreenCenter           = src.VScreenCenter;
        refHmdInfo->EyeToScreenDistance     = src.EyeToScreenDistance;
        refHmdInfo->LensSeparationDistance  = src.LensSeparationDistance;
        refHmdInfo->InterpupillaryDistance  = src.InterpupillaryDistance;
        refHmdInfo->DistortionK[0]          = src.DistortionK[0];
        refHmdInfo->DistortionK[1]          = src.DistortionK[1];
        refHmdInfo->DistortionK[2]          = src.DistortionK[2];
        refHmdInfo->DistortionK[3]          = src.DistortionK[3];
        refHmdInfo->DesktopX                = src.DesktopX;
        refHmdInfo->DesktopY                = src.DesktopY;
        memcpy(refHmdInfo->DisplayDeviceName, src.DisplayDeviceName, sizeof(refHmdInfo->DisplayDeviceName));        
	}

	return 1;
}


LIBOVRWRAPPER_API int OVR_Peek(float* yaw, float* pitch, float* roll)
{
	if (!bInited)
	{
		return 0;
	}

	Quatf hmdOrient = pSensorFusion.GetOrientation();
	hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(yaw, pitch, roll);

	return 1;
}

LIBOVRWRAPPER_API int OVR_StereoConfig(int eye, struct OVR_StereoCfg* stereoCfg)
{
	/*if (!bInited)
	{
		return 0;
	}*/

	using namespace OVR::Util::Render;
	StereoEyeParams seye = pStereo.GetEyeRenderParams((eye==0?StereoEye_Left:StereoEye_Right));

	pStereo.SetFullViewport(Viewport(stereoCfg->x, stereoCfg->y, stereoCfg->w, stereoCfg->h));
	pStereo.SetStereoMode(Stereo_LeftRight_Multipass);
	pStereo.SetHMDInfo(pDevInfo);

    if (pDevInfo.HScreenSize > 0.0f)
    {
        if (pDevInfo.HScreenSize > 0.140f) // 7"
            pStereo.SetDistortionFitPointVP(-1.0f, 0.0f);
        else
            pStereo.SetDistortionFitPointVP(0.0f, 1.0f);
    }
	
	stereoCfg->renderScale = pStereo.GetDistortionScale();

    DistortionConfig distCfg = pStereo.GetDistortionConfig();
	
	// Right Eye ?
	if (eye == 1)
	{
		distCfg.XCenterOffset = -distCfg.XCenterOffset;
	}

	stereoCfg->x = seye.VP.x;
	stereoCfg->y = seye.VP.y;
	stereoCfg->w = seye.VP.w;
	stereoCfg->h = seye.VP.h;
	stereoCfg->XCenterOffset = distCfg.XCenterOffset;
	stereoCfg->distscale     = distCfg.Scale;
	stereoCfg->K[0] = distCfg.K[0];
	stereoCfg->K[1] = distCfg.K[1];
	stereoCfg->K[2] = distCfg.K[2];
	stereoCfg->K[3] = distCfg.K[3];
	
	return 1;
}


LIBOVRWRAPPER_API int SIX_Peek(int hand, float* joyx, float* joyy, float* pos, float* trigger, float* yaw, float* pitch, float* roll, unsigned int* buttons)
{
	const  sixenseUtils::IControllerManager::controller_desc HandIndex[] = { sixenseUtils::ControllerManager::P1L, sixenseUtils::ControllerManager::P1R };
	static sixenseAllControllerData   acd;
	static sixenseUtils::ButtonStates btnState;

	sixenseSetActiveBase(0);
	sixenseGetAllNewestData(&acd);
	sixenseUtils::getTheControllerManager()->update(&acd);	
	
	int idx  = sixenseUtils::getTheControllerManager()->getIndex(HandIndex[hand]);
	
	sixenseControllerData& ref = acd.controllers[idx];
	btnState.update(&ref);

	*buttons = ref.buttons;
	

	if (!ref.enabled)
	{
		pos[0] = 0.0f;
		pos[1] = 0.0f;
		pos[2] = 0.0f;
		*joyx  = 0.0f;
		*joyy  = 0.0f;
		*trigger = 0.0f;
		*yaw     = 0.0f;
		*pitch   = 0.0f;
		*roll    = 0.0f;
		return 0;
	}
	
	pos[0] = ref.pos[0];
	pos[1] = ref.pos[1];
	pos[2] = ref.pos[2];
	
	*joyx  = ref.joystick_x;
	*joyy  = ref.joystick_y;

	*trigger = ref.trigger;
	
	Quatf orient(ref.rot_quat[0], ref.rot_quat[1], ref.rot_quat[2], ref.rot_quat[3]);
	orient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(yaw, pitch, roll);
	return 1;
}
