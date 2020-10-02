#ifdef LIBOVRWRAPPER_EXPORTS
	#if defined __cplusplus
		#define LIBOVRWRAPPER_API extern "C" __declspec(dllexport)
	#else
		#define LIBOVRWRAPPER_API __declspec(dllexport)
	#endif
#else
	#if defined __cplusplus
		#define LIBOVRWRAPPER_API extern "C" __declspec(dllimport)
	#else
		#define LIBOVRWRAPPER_API __declspec(dllimport)
	#endif
#endif

struct OVR_HMDInfo
{	
	unsigned      HResolution;
	unsigned      VResolution; 
    float         HScreenSize;
	float         VScreenSize;
    float         VScreenCenter;
    float         EyeToScreenDistance;
    float         LensSeparationDistance;
    float         InterpupillaryDistance;
    float         DistortionK[4];
    int           DesktopX;
	int           DesktopY;
    char          DisplayDeviceName[32];
};


struct OVR_StereoCfg
{
	int   x;
	int   y;
	int   w; 
	int   h;
	float renderScale;
	float XCenterOffset;
	float distscale; 
	float K[4];
};

#define SIXENSE_BUTTON_BUMPER   (0x01<<7)
#define SIXENSE_BUTTON_JOYSTICK (0x01<<8)
#define SIXENSE_BUTTON_1        (0x01<<5)
#define SIXENSE_BUTTON_2        (0x01<<6)
#define SIXENSE_BUTTON_3        (0x01<<3)
#define SIXENSE_BUTTON_4        (0x01<<4)
#define SIXENSE_BUTTON_START    (0x01<<0)

LIBOVRWRAPPER_API int   OVR_Init();
LIBOVRWRAPPER_API void  OVR_Exit();
LIBOVRWRAPPER_API int   OVR_QueryHMD(struct OVR_HMDInfo* refHmdInfo);
LIBOVRWRAPPER_API int   OVR_Peek(float* yaw, float* pitch, float* roll);
LIBOVRWRAPPER_API int   OVR_StereoConfig(int eye, struct OVR_StereoCfg* stereoCfg);
LIBOVRWRAPPER_API int   SIX_Peek(int hand, float* joyx, float* joyy, float* pos, float* trigger, float* yaw, float* pitch, float* roll, unsigned int* buttons);