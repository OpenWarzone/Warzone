#include "tr_local.h"

#ifdef __VR__
#pragma comment(lib, "../../../sdk/openVR/openvr_api.lib")

int OVRDetected = 0;
vr::IVRSystem* HMD = NULL;
OpenVRInterface *VR = NULL;


//
// Initialization and Shutdown...
//

OpenVRInterface::OpenVRInterface(void)
{
	if (!hmdIsPresent())
	{
		ri->Printf(PRINT_ERROR, "[VR] Error : HMD not detected on the system. VR disabled.\n");
		OVRDetected = 0;
		HMD = NULL;
		return;
	}
	else
	{
		ri->Printf(PRINT_WARNING, "[VR] HMD detected on the system.\n");
	}

	if (!vr::VR_IsRuntimeInstalled())
	{
		ri->Printf(PRINT_ERROR, "[VR] Error : OpenVR Runtime not detected on the system. VR disabled.\n");
		OVRDetected = 0;
		HMD = NULL;
		return;
	}
	else
	{
		ri->Printf(PRINT_WARNING, "[VR] OpenVR Runtime detected on the system.\n");
	}

	vr::EVRInitError err = vr::VRInitError_None;
	HMD = vr::VR_Init(&err, vr::VRApplication_Scene);

	if (err != vr::VRInitError_None)
	{
		handleVRError(err);
		HMD = NULL;
		return;
	}

	ri->Printf(PRINT_WARNING, "[VR] Found HMD device %s (%s) (%s)\n"
		, GetTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String).c_str()
		, GetTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String).c_str()
		, GetTrackedDeviceString(HMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ControllerType_String).c_str());

	if (!vr::VRCompositor())
	{
		ri->Printf(PRINT_ERROR, "[VR] Unable to initialize VR compositor! VR disabled.\n");
		OVRDetected = 0;
		HMD = NULL;
		return;
	}
	else
	{
		ri->Printf(PRINT_WARNING, "[VR] Initialized the VR compositor!\n");
	}

	HMD->GetRecommendedRenderTargetSize(&recommendedRenderWidth, &recommendedRenderHeight);

	ri->Printf(PRINT_WARNING, "[VR] Initialized HMD with suggested render target size : %u x %u.\n", recommendedRenderWidth, recommendedRenderHeight);

	if (vr::IVRExtendedDisplay *d = vr::VRExtendedDisplay())
	{
		int32_t x = 0, y = 0;
		d->GetWindowBounds(&x, &y, &deviceWidth, &deviceHeight);

		ri->Printf(PRINT_WARNING, "[VR] Actual HMD display size : %u x %u.\n", deviceWidth, deviceHeight);
	}
	else
	{
		deviceWidth = recommendedRenderWidth;
		deviceHeight = recommendedRenderHeight;
		ri->Printf(PRINT_WARNING, "[VR] Actual HMD display size is unknown.\n");
	}

	// Hmmm... Will want to change this at some point to use recommended values... probably will do this if I do separate eye renders...
	//renderWidth = glConfig.vidWidth;
	//renderHeight = glConfig.vidHeight;

	renderWidth = recommendedRenderWidth / 2;
	renderHeight = renderWidth;// recommendedRenderHeight;

	//renderWidth = deviceWidth / 2;
	//renderHeight = deviceHeight;

	//glConfig.vidWidth = renderWidth;
	//glConfig.vidHeight = renderHeight;

	ri->Printf(PRINT_WARNING, "[VR] Actual render size : %u x %u.\n", renderWidth, renderHeight);
}

OpenVRInterface::~OpenVRInterface(void)
{
	if (HMD)
	{
		vr::VR_Shutdown();
		HMD = NULL;
	}
}


//
// Private...
//

std::string OpenVRInterface::GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

void OpenVRInterface::handleVRError(vr::EVRInitError err)
{
	ri->Printf(PRINT_ERROR, "%s\n", vr::VR_GetVRInitErrorAsEnglishDescription(err));
}

void OpenVRInterface::SubmitFramesOpenGL(GLint leftEyeTex, GLint rightEyeTex, bool linear)
{
	if (!HMD)
	{
		ri->Printf(PRINT_ERROR, "Error : presenting frames when VR system handle is NULL.\n");
	}

	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

	vr::EColorSpace colorSpace = linear ? vr::ColorSpace_Linear : vr::ColorSpace_Gamma;

	vr::Texture_t leftEyeTexture = { (void*)leftEyeTex, vr::TextureType_OpenGL, colorSpace };
	vr::Texture_t rightEyeTexture = { (void*)rightEyeTex, vr::TextureType_OpenGL, colorSpace };

	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

	vr::VRCompositor()->PostPresentHandoff();
}


//
// Public...
//

// returns if the system believes there is an HMD present without initializing all of OpenVR
inline bool OpenVRInterface::hmdIsPresent(void)
{
	return vr::VR_IsHmdPresent();
}

uint32_t OpenVRInterface::RenderWidth(void)
{
	return VR->renderWidth;
}

uint32_t OpenVRInterface::RenderHeight(void)
{
	return VR->renderHeight;
}

uint32_t OpenVRInterface::DeviceWidth(void)
{
	return VR->deviceWidth;
}

uint32_t OpenVRInterface::DeviceHeight(void)
{
	return VR->deviceHeight;
}

unsigned int OpenVRInterface::GetWarpShader(void)
{
	return VR->vrShaderProgId;
}

void OpenVRInterface::LoadWarpShader(void)
{
	char*VertexShader =
		"void main()"\
		"{	"\
		"gl_TexCoord[0] = gl_MultiTexCoord0;"\
		"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; "\
		"}";


	char *FragmentShader =
		"uniform sampler2D texid;\n"\
		"uniform vec2      LensCenter;\n"\
		"uniform vec2      ScreenCenter;\n"\
		"uniform vec2      Scale;\n"\
		"uniform vec2      ScaleIn;\n"\
		"uniform vec4      HmdWarpParam;\n"\
		"uniform vec2      Offset;\n"\
		"void main()\n"\
		"{\n"\
		"   vec2  uv = gl_TexCoord[0].xy;\n"\
		"	vec2  theta   = (uv - LensCenter) *ScaleIn;\n"\
		"	float rSq     = theta.x * theta.x + theta.y * theta.y;\n"\
		"	vec2  rvector = Offset + theta * (HmdWarpParam.x + HmdWarpParam.y * rSq +\n"\
		"		                     HmdWarpParam.z * rSq * rSq +\n"\
		"							 HmdWarpParam.w * rSq * rSq * rSq);\n"\
		"	vec2  tc      = (LensCenter + Scale * rvector);\n"\
		"	if (any(bvec2(clamp(tc, ScreenCenter-vec2(0.50,0.5), ScreenCenter+vec2(0.50,0.5))-tc)))\n"\
		"		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"\
		"	else\n"\
		"		gl_FragColor = texture2D(texid, tc);	\n"\
		"}\n";


	GLint Result = GL_FALSE;
	int InfoLogLength;
	GLuint ProgramID;
	char* infoLog;
	int charsWritten;



	// Create the shaders
	GLuint VertexShaderID = qglCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = qglCreateShader(GL_FRAGMENT_SHADER);

	qglShaderSource(VertexShaderID, 1, &VertexShader, NULL);
	qglCompileShader(VertexShaderID);

	// Check Vertex Shader
	qglGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	qglGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1)
	{
		infoLog = (char *)malloc(InfoLogLength);
		qglGetShaderInfoLog(VertexShaderID, InfoLogLength, &charsWritten, infoLog);
		Com_Printf("%s\n", infoLog);
		free(infoLog);
	}


	// Compile Fragment Shader
	qglShaderSource(FragmentShaderID, 1, &FragmentShader, NULL);
	qglCompileShader(FragmentShaderID);

	// Check Fragment Shader
	qglGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	qglGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1)
	{
		infoLog = (char *)malloc(InfoLogLength);
		qglGetShaderInfoLog(FragmentShaderID, InfoLogLength, &charsWritten, infoLog);
		Com_Printf("%s\n", infoLog);
		free(infoLog);
	}


	ProgramID = qglCreateProgram();
	qglAttachShader(ProgramID, VertexShaderID);
	qglAttachShader(ProgramID, FragmentShaderID);
	qglLinkProgram(ProgramID);

	// Check the program
	qglGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	qglGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1)
	{
		infoLog = (char *)malloc(InfoLogLength);
		qglGetProgramInfoLog(ProgramID, InfoLogLength, &charsWritten, infoLog);
		Com_Printf("%s\n", infoLog);
		free(infoLog);
	}

	qglDeleteShader(VertexShaderID);
	qglDeleteShader(FragmentShaderID);

	vrShaderProgId = ProgramID;
}

void OpenVRInterface::UpdateRenderer(void)
{
#ifdef __VR_SEPARATE_EYE_RENDER__
	VR->SubmitFramesOpenGL(tr.renderRightVRImage->texnum, tr.renderLeftVRImage->texnum, true);
#else //!__VR_SEPARATE_EYE_RENDER__
	VR->SubmitFramesOpenGL(tr.renderImage->texnum, tr.renderImage->texnum);
#endif //__VR_SEPARATE_EYE_RENDER__
}

void OpenVRInterface::UpdateRenderer2D(void)
{
#ifdef __VR_SEPARATE_EYE_RENDER__
	VR->SubmitFramesOpenGL(tr.renderLeftVRImage->texnum, tr.renderLeftVRImage->texnum);
#else //!__VR_SEPARATE_EYE_RENDER__
	VR->SubmitFramesOpenGL(tr.renderImage->texnum, tr.renderImage->texnum);
#endif //__VR_SEPARATE_EYE_RENDER__
}


void VR_Shutdown(void)
{
	if (VR)
	{
		VR->~OpenVRInterface();
	}

	if (HMD)
	{// Should be handled above, but just in case...
		vr::VR_Shutdown();
	}

	VR = NULL;
	HMD = NULL;
}

void VR_Initialize(void)
{
	VR_Shutdown();

	if (OVRDetected || vr::VR_IsHmdPresent())
	{
		VR = new OpenVRInterface;

		if (HMD)
		{
			OVRDetected = 1;
			ri->Cvar_Set("vr_ovrdetected", va("%i", OVRDetected));
		}
		else
		{
			OVRDetected = 0;
			ri->Cvar_Set("vr_ovrdetected", va("%i", OVRDetected));
		}
	}
	else
	{
		OVRDetected = 0;
		ri->Cvar_Set("vr_ovrdetected", va("%i", OVRDetected));
	}
}
#endif //__VR__
