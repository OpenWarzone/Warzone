#include "qcommon\q_shared.h"

#ifdef __ENGINE_AWESOMIUM__
#include "client.h"
#include "ui/keycodes.h"
#include "cl_awesomium.h"

#include <unordered_map>

#pragma comment(lib, "awesomium.lib")

std::string WebStringToANSI(const Awesomium::WebString &str) {
	std::string ret = Awesomium::ToString(str);
	return ret;
}

static void BGRA8_To_RGBA8(int width, int height, const unsigned char *src, unsigned char *dest) {
	for (int offset = 0; offset < height * width * 4;) {
		dest[offset] = src[offset + 2];
		dest[offset + 1] = src[offset + 1];
		dest[offset + 2] = src[offset];
		offset += 4;
	}
}

namespace Awesomium {
	static std::unordered_map<int, Window*> views;
	static WebConfig config;
	size_t Window::windowsCount = 0;
	Window *Window::mainInterface;

	void KeyEvent(int key, qboolean down) {
		WebKeyboardEvent e;
		WebView *view_current;
		e.type = down == qtrue ? WebKeyboardEvent::kTypeKeyDown :
			WebKeyboardEvent::kTypeKeyUp;

		for (auto it = views.begin(); it != views.end(); ++it) { // EpicLoyd::FIX:: pizdets
			if (!it->second || (it->second && !it->second->getActive())) continue;
			view_current = const_cast<WebView*>(it->second->GetView());
			if (!view_current) continue;

			switch (key) {
			case A_BACKSPACE:
				e.virtual_key_code = KeyCodes::AK_BACK;
				break;
			case A_ENTER:
			case A_KP_ENTER:
				e.virtual_key_code = KeyCodes::AK_RETURN;
				break;
			case A_SPACE:
				e.virtual_key_code = KeyCodes::AK_SPACE;
				break;
			case A_MWHEELUP:
				view_current->InjectMouseWheel(10, 0);
				break;
			case A_MWHEELDOWN:
				view_current->InjectMouseWheel(-10, 0);
				break;
			case A_MOUSE1:
				(down == qtrue) ? view_current->InjectMouseDown(MouseButton::kMouseButton_Left) :
					view_current->InjectMouseUp(MouseButton::kMouseButton_Left);
				break;
			case A_MOUSE2:
				(down == qtrue) ? view_current->InjectMouseDown(MouseButton::kMouseButton_Right) :
					view_current->InjectMouseUp(MouseButton::kMouseButton_Right);
				break;
			default:
				e.text[0] = (char)key;
			}
			view_current->InjectKeyboardEvent(e);
		}
	}

	void MouseEvent(int x, int y) {
		static int actual_x = 0, actual_y = 0;

		actual_x += x;
		if (actual_x < 0) actual_x = 0;
		if (actual_x > cls.glconfig.vidWidth) actual_x = cls.glconfig.vidWidth;
		actual_y += y;
		if (actual_y < 0) actual_y = 0;
		if (actual_y > cls.glconfig.vidHeight) actual_y = cls.glconfig.vidHeight;

		for (auto it = views.begin(); it != views.end(); ++it) { // EpicLoyd::FIX::PIZDETS
			if (!it->second) continue;
			it->second->MouseEvent(actual_x, actual_y, x, y);
		}
	}

	Window::Window(int width, int height, const std::string & path) {
		config.additional_options.Push(WSLit("--use-gl"));

		this->name = path;
		this->width = width;
		this->height = height;
		this->core = WebCore::instance();
		if (this->core == nullptr) {
			Com_Error(ERR_FATAL, "Awesomium: Unable to init awesomium core\n");
		}
		this->session = core->CreateWebSession(WSLit(""), WebPreferences());
		this->loader = new PageLoader(path);
		this->session->AddDataSource(WSLit("data"), this->loader);
		this->view = this->core->CreateWebView(this->width, this->height, session);
		if (!this->view) {
			Com_Error(ERR_FATAL, "Awesomium: Unable to create WebView\n");
			return;
		}
		this->id = windowsCount++;
		views[this->id] = this;
	}

	Window::~Window() {
		this->view->Stop();
		this->view->Destroy();
		delete this->loader;
		this->session->Release();
		views[this->id] = nullptr;
	}

	void Awesomium::Window::MouseEvent(int x, int y, int rel_x, int rel_y){
		if (!this->active) return;
		this->view->InjectMouseMove(x, y);
	}

	void Window::Resize(int width, int height) {
		this->view->Resize(width, height);
	}

	void Window::Load(const std::string & url) {
		this->view->LoadURL(WebURL(WSLit(url.c_str())));
	}

	void Window::Render(void) {
		core->Update();
		if (this->view->IsLoading()) return;
		Awesomium::BitmapSurface *srf = (BitmapSurface*)this->view->surface();
		if (!srf) return;

		//Swap from bgra to rgba
		unsigned char *buffer_actual = (unsigned char *)malloc(this->width * this->height * 4);
		BGRA8_To_RGBA8(this->width, this->height, srf->buffer(), buffer_actual);

		re->DrawAwesomiumFrame(0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, const_cast<unsigned char *>(buffer_actual));
	}

	const WebView *Awesomium::Window::GetView(void){
		if (!view) return nullptr;
		return view;
	}

	const bool Window::getActive(void) const{
		return this->active;
	}

	void InitUserInterface(void) {
		if (!cl_useAwesomium->integer) return;

		if (!WebCore::instance()) {
			WebCore::Initialize(Awesomium::config);
		}
		Window *win = new Window(cls.glconfig.vidWidth, cls.glconfig.vidHeight, "html");
		win->Load("asset://data/index.html");
		Window::mainInterface = win;
	}

	void RenderUserInterface(void) {
		if (!cl_useAwesomium->integer || !Window::mainInterface) return;
		Window::mainInterface->Render();
	}

	void ShutdownUserInterface(void) {
		if (!cl_useAwesomium->integer || !Window::mainInterface) return;
		delete Window::mainInterface;
		Window::mainInterface = nullptr;
	}
}
#endif //__ENGINE_AWESOMIUM__
