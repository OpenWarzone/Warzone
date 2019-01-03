#include "qcommon\q_shared.h"

#ifdef __ENGINE_AWESOMIUM__
#include "Awesomium/WebCore.h"
#include "Awesomium/BitmapSurface.h"
#include "Awesomium/STLHelpers.h"
#include "Awesomium/WebViewListener.h"
#include "Awesomium/DataSource.h"

std::string WebStringToANSI(const Awesomium::WebString &str);

namespace Awesomium {
	class PageLoader : public DataSource {
		std::string homepath;
	public:
		PageLoader(const std::string &homepath) { this->homepath = homepath; };
		~PageLoader() {};

		void OnRequest(int request_id, const Awesomium::ResourceRequest& request, const Awesomium::WebString& path) {
			fileHandle_t f;
			std::string p = WebStringToANSI(path);

			const unsigned char error[] = "<h1>Failed to load file</h1>";
			const int size = sizeof(error) / sizeof(error[0]);
			const char *loadpath = va("%s/%s", homepath.c_str(), p.c_str());

			const int len = FS_FOpenFileRead(loadpath, &f, qfalse);
			if (!f) {
				SendResponse(request_id, size, error, WSLit("text/html"));
			}
			unsigned char *buffer = (unsigned char*)malloc(len);
			FS_Read(buffer, len, f);
			FS_FCloseFile(f);
			SendResponse(request_id, len, buffer, WSLit("text/html"));
			free(buffer);
		}

	};

	class Window {
	private:
		WebCore *core;
		WebView *view;
		WebSession *session;
		PageLoader *loader;
		bool active;
		int width;
		int height;
	public:
		static size_t windowsCount;
		static Window *mainInterface;
		uint32_t id;
		std::string name;
		// don't allow default instantiation
		Window() = delete;
		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window::Window(int width, int height, const std::string & name);
		~Window();

		void Resize(int width, int height);
		void Load(const std::string &url);

		virtual void MouseEvent(int x, int y, int rel_x, int rel_y);

		void Render(void);
		const WebView *GetView(void);
		const bool getActive(void) const ;
	};

	void KeyEvent(int key, qboolean down);
	void MouseEvent(int x, int y);
	void InitUserInterface(void);
	void RenderUserInterface(void);
	void ShutdownUserInterface(void);
}
#endif //__ENGINE_AWESOMIUM__
