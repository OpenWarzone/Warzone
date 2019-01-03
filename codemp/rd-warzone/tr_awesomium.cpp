#include "tr_local.h"

#ifdef ___WARZONE_AWESOMIUM___
// Various included headers
#include "Awesomium/WebCore.h"
#include "Awesomium/BitmapSurface.h"
#include "Awesomium/STLHelpers.h"

#include "glext.h"

#include <algorithm>

extern void Upload32( byte *data, int width, int height, imgType_t type, int flags, qboolean lightMap, GLenum internalFormat, int *pUploadWidth, int *pUploadHeight);
extern void RawImage_UploadTexture( byte *data, int x, int y, int width, int height, GLenum internalFormat, imgType_t type, int flags, qboolean subtexture );

// Various macro definitions
#define WIDTH   800
#define HEIGHT  600

using namespace Awesomium;

char OPEN_URL[256] = {0};
WebCore* web_core = NULL;
WebView* view = NULL;
SurfaceFactory *sf = NULL;
Surface* surface = NULL;

image_t *awesomiumImage = NULL;

#include "Awesomium/Surface.h"
#include "Awesomium/BitmapSurface.h"

namespace Awesomium
{
	class BitmapSurface;
	class WebString;
}

namespace AwesomiumWarzone
{

	class WarzoneSurface	: public Awesomium::Surface
	{
	public:
		WarzoneSurface( int _Width, int _Height );
		~WarzoneSurface();

		void Paint(	unsigned char* _SrcBuffer,
			int _SrcRowSpan,
			const Awesomium::Rect& _SrcRect,
			const Awesomium::Rect& _DestRect );

		void Scroll( int _DX, int _DY, const Awesomium::Rect& _ClipRect );



		const unsigned char* buffer() const;

		int width() const;

		int height() const;

		int row_span() const;

		void set_is_dirty(bool is_dirty);

		bool is_dirty() const;

		void CopyTo(unsigned char* dest_buffer,
			int dest_row_span,
			int dest_depth,
			bool convert_to_rgba,
			bool flip_y);

		bool SaveToPNG(const Awesomium::WebString& file_path,
			bool preserve_transparency = false) const;

		bool SaveToJPEG(const Awesomium::WebString& file_path,
			int quality = 90) const;

		unsigned char GetAlphaAtPoint(int x, int y) const;

		const Awesomium::Rect& GetLastChanges() const;

		Awesomium::BitmapSurface *WarzoneSurface::GetSurface() const;
	private:
		WarzoneSurface();

		Awesomium::BitmapSurface* m_BitmapSurface;

		// A rect encompassing all changed areas since the last time CopyTo was called.
		Awesomium::Rect m_LastChangedRect;
	};

}

namespace AwesomiumWarzone
{

	WarzoneSurface::WarzoneSurface( int _Width, int _Height )
	{
		m_BitmapSurface = new Awesomium::BitmapSurface(_Width, _Height);
	}

	WarzoneSurface::WarzoneSurface()
	{

	}

	WarzoneSurface::~WarzoneSurface()
	{
		delete m_BitmapSurface;
		m_BitmapSurface = 0;
	}

	void WarzoneSurface::Paint( unsigned char* _SrcBuffer, int _SrcRowSpan, const Awesomium::Rect& _SrcRect, const Awesomium::Rect& _DestRect )
	{
		m_BitmapSurface->Paint(_SrcBuffer, _SrcRowSpan, _SrcRect, _DestRect);

		if (m_LastChangedRect.IsEmpty())
		{
			m_LastChangedRect = _DestRect;
		}
		else
		{
			m_LastChangedRect.x = (std::min)(m_LastChangedRect.x, _DestRect.x);
			m_LastChangedRect.y = (std::min)(m_LastChangedRect.y, _DestRect.y);
			m_LastChangedRect.width = (std::max)(m_LastChangedRect.x + m_LastChangedRect.width, _DestRect.x + _DestRect.width) - m_LastChangedRect.x;
			m_LastChangedRect.height = (std::max)(m_LastChangedRect.y + m_LastChangedRect.height, _DestRect.y + _DestRect.height) - m_LastChangedRect.y;
		}
	}

	void WarzoneSurface::Scroll( int _DX, int _DY, const Awesomium::Rect& _ClipRect )
	{
		m_BitmapSurface->Scroll(_DX, _DY, _ClipRect);
	}



	const unsigned char* WarzoneSurface::buffer() const 
	{ 
		return m_BitmapSurface->buffer(); 
	}

	int WarzoneSurface::width() const 
	{ 
		return m_BitmapSurface->width(); 
	}

	int WarzoneSurface::height() const 
	{ 
		return m_BitmapSurface->height(); 
	}

	int WarzoneSurface::row_span() const 
	{ 
		return m_BitmapSurface->row_span(); 
	}

	void WarzoneSurface::set_is_dirty(bool is_dirty)
	{
		m_BitmapSurface->set_is_dirty(is_dirty);
	}

	bool WarzoneSurface::is_dirty() const
	{
		return m_BitmapSurface->is_dirty();
	}

	Awesomium::BitmapSurface *WarzoneSurface::GetSurface() const
	{
		return m_BitmapSurface;
	}

	void WarzoneSurface::CopyTo(	unsigned char* dest_buffer,
		int dest_row_span,
		int dest_depth,
		bool convert_to_rgba,
		bool flip_y)
	{
		m_BitmapSurface->CopyTo(dest_buffer, dest_row_span, dest_depth, convert_to_rgba, flip_y);

		m_LastChangedRect.width = 0;
		m_LastChangedRect.height = 0;
	}

	bool WarzoneSurface::SaveToPNG(const Awesomium::WebString& file_path,
		bool preserve_transparency ) const
	{
		return m_BitmapSurface->SaveToPNG(file_path, preserve_transparency);
	}

	bool WarzoneSurface::SaveToJPEG(const Awesomium::WebString& file_path,
		int quality ) const
	{
		return m_BitmapSurface->SaveToJPEG(file_path, quality);
	}

	unsigned char WarzoneSurface::GetAlphaAtPoint(int x, int y) const
	{
		return m_BitmapSurface->GetAlphaAtPoint(x, y);
	}

	const Awesomium::Rect& WarzoneSurface::GetLastChanges() const
	{
		return m_LastChangedRect;
	}
}

namespace AwesomiumWarzone
{

	class WarzoneSurfaceFactory	: public Awesomium::SurfaceFactory
	{
	public:
		WarzoneSurfaceFactory();
		~WarzoneSurfaceFactory();

		Awesomium::Surface* CreateSurface( Awesomium::WebView* _View, int _Width, int _Height );

		void DestroySurface( Awesomium::Surface* _Surface );
	};

}

namespace AwesomiumWarzone
{

	WarzoneSurfaceFactory::WarzoneSurfaceFactory()
	{

	}

	WarzoneSurfaceFactory::~WarzoneSurfaceFactory()
	{

	}

	Awesomium::Surface* WarzoneSurfaceFactory::CreateSurface( Awesomium::WebView* _View, int _Width, int _Height )
	{
		return new WarzoneSurface(_Width, _Height);
	}

	void WarzoneSurfaceFactory::DestroySurface( Awesomium::Surface* _Surface )
	{
		delete _Surface;
	}

}

void webview_copybuffertotexture( WebView* _Instance, image_t *image, int _TextureWidth, int _TextureHeight )
{
	if (!_Instance || !image) return;

	AwesomiumWarzone::WarzoneSurface* wsurface = (AwesomiumWarzone::WarzoneSurface*)_Instance->surface();

	// Make sure our surface is not null -- it may be null if the WebView 
	// process has crashed.
	if (wsurface != 0) 
	{
		// Save our BitmapSurface to a JPEG image in the current
		// working directory.

		//surface->SaveToPNG(WSLit("./result.jpg"), true);

		// update native texture from code
		if (image)
		{
			GLuint gltex = (GLuint)(size_t)(image->texnum);
			GL_Bind(image);

			int texWidth = _TextureWidth, texHeight = _TextureHeight;
			
			if (qglGetTexLevelParameteriv)
			{// Is NULL in rend2????
				qglGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
				qglGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
			}

			unsigned char* data = new unsigned char[texWidth*texHeight*4];

			wsurface->CopyTo(data, texWidth * 4, 4, true, true);

			qglTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);

			delete[] data;
		}
	}
}

AwesomiumWarzone::WarzoneSurface *bmsurface = NULL;

void AwesomiumInit( void )
{
	if (!web_core)
	{
		// Your code goes here.
		// Create the WebCore singleton with default configuration
		web_core = WebCore::Initialize(WebConfig());
		
		// Create a new WebView instance with a certain width and height
		view = web_core->CreateWebView(WIDTH, HEIGHT, 0, kWebViewType_Offscreen);
		
		sf = new AwesomiumWarzone::WarzoneSurfaceFactory();
		surface = new AwesomiumWarzone::WarzoneSurface(WIDTH, HEIGHT);
		web_core->set_surface_factory(sf);

		bmsurface = static_cast<AwesomiumWarzone::WarzoneSurface*>(surface);
		
		awesomiumImage = R_CreateImage( "_awesomium", NULL, WIDTH, HEIGHT, IMGTYPE_COLORALPHA, IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION, GL_RGBA8 );
	}
}

void AwesomiumShutdown( void )
{
	if (web_core)
	{
		if (view) view->Destroy();
		WebCore::Shutdown();
	}
}

// Our main program
int DrawAwesomium( char *URL, FBO_t *srcFbo ) {
	AwesomiumInit();

	if (!strcmp(URL, OPEN_URL))
	{
		web_core->Update();

		if (!view) {
			//ri->Printf(PRINT_WARNING, "AWESOMIUM: No view for %s\n", URL);
			return 0;
		}

		if (view->IsLoading()) {
			//ri->Printf(PRINT_WARNING, "AWESOMIUM: Loading %s\n", URL);
			return 0; // wait
		}
		
		if (!surface || !bmsurface) {
			//ri->Printf(PRINT_WARNING, "AWESOMIUM: No surface for %s\n", URL);
			return 0; // wait
		}
		
		if (bmsurface->buffer())
		{
			webview_copybuffertotexture( view, awesomiumImage, bmsurface->width(), bmsurface->height() );

			float xoffset = (srcFbo->width - bmsurface->width()) / 2.0;
			float yoffset = (srcFbo->height - bmsurface->height()) / 2.0;
			
			vec4i_t		srcBox, dstBox;
			VectorSet4(srcBox, 0, 0, bmsurface->width(), bmsurface->height());
			VectorSet4(dstBox, xoffset, yoffset, bmsurface->width(), bmsurface->height());
			FBO_BlitFromTexture(awesomiumImage, srcBox, NULL, NULL, dstBox, NULL, NULL, 0);
		}
		else
		{
			//ri->Printf(PRINT_WARNING, "AWESOMIUM: No surface buffer for %s\n", URL);
		}
	}
	else
	{
		ri->Printf(PRINT_WARNING, "AWESOMIUM: Opening %s\n", URL);
		
		// Load a certain URL into our WebView instance
		WebURL url(WSLit(URL));
		view->LoadURL(url);
		memset(OPEN_URL, 0, sizeof(OPEN_URL));
		strcpy(OPEN_URL, URL);
	}
	return 0;
}
#endif //___WARZONE_AWESOMIUM___
