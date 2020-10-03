class OpenVRInterface
{
private:
	uint32_t		renderWidth = 0;
	uint32_t		renderHeight = 0;

	uint32_t		recommendedRenderWidth = 0;
	uint32_t		recommendedRenderHeight = 0;

	uint32_t		deviceWidth = 0;
	uint32_t		deviceHeight = 0;

	unsigned int	vrShaderProgId;

	void			SubmitFramesOpenGL(GLint leftEyeTex, GLint rightEyeTex, bool linear = false);
	void			handleVRError(vr::EVRInitError err);
	std::string		GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL);
public:
	OpenVRInterface(void); // constructor
	~OpenVRInterface(void); // destructor

	void			UpdateRenderer(void);
	void			UpdateRenderer2D(void);

	uint32_t		RenderWidth(void);
	uint32_t		RenderHeight(void);

	uint32_t		DeviceWidth(void);
	uint32_t		DeviceHeight(void);

	void			LoadWarpShader(void);
	unsigned int	GetWarpShader(void);

	inline bool		hmdIsPresent();
};

extern OpenVRInterface *VR;

void VR_Initialize(void);
void VR_Shutdown(void);
