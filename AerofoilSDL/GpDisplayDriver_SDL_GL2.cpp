#include "IGpDisplayDriver.h"

#include "CoreDefs.h"
#include "GpApplicationName.h"
#include "GpComPtr.h"
#include "GpDisplayDriverProperties.h"
#include "GpVOSEvent.h"
#include "GpRingBuffer.h"
#include "GpInputDriver_SDL_Gamepad.h"
#include "GpSDL.h"
#include "GpUnicode.h"
#include "IGpCursor.h"
#include "IGpDisplayDriverSurface.h"
#include "IGpLogDriver.h"
#include "IGpPrefsHandler.h"
#include "IGpSystemServices.h"
#include "IGpVOSEventQueue.h"

#include "SDL_events.h"
#include "SDL_mouse.h"
#include "SDL_opengl.h"
#include "SDL_video.h"

#include <stdlib.h>
#include <new>
#include <assert.h>

#include <chrono>
#include <numeric>
#include <vector>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#pragma push_macro("LoadCursor")
#ifdef LoadCursor
#undef LoadCursor
#endif

#define GP_GL_IS_OPENGL_4_CONTEXT	0

class GpDisplayDriver_SDL_GL2;

static GpDisplayDriverSurfaceEffects gs_defaultEffects;

static const char *kPrefsIdentifier = "GpDisplayDriverSDL_GL2";
static uint32_t kPrefsVersion = 1;

struct GpDisplayDriver_SDL_GL2_Prefs
{
	bool m_isFullScreen;
};

namespace GpBinarizedShaders
{
	extern const char *g_drawQuadV_GL2;

	extern const char *g_drawQuadPalettePF_GL2;
	extern const char *g_drawQuadPalettePNF_GL2;
	extern const char *g_drawQuad32PF_GL2;
	extern const char *g_drawQuad32PNF_GL2;

	extern const char *g_drawQuadPaletteICCPF_GL2;
	extern const char *g_drawQuadPaletteICCPNF_GL2;
	extern const char *g_drawQuad32ICCPF_GL2;
	extern const char *g_drawQuad32ICCPNF_GL2;

	extern const char *g_copyQuadP_GL2;
	extern const char *g_scaleQuadP_GL2;
}

struct GpGLFunctions
{
	typedef void (GLAPIENTRYP PFNGLCLEARPROC)(GLbitfield mask);
	typedef void (GLAPIENTRYP PFNGLCLEARCOLORPROC)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	typedef void (GLAPIENTRYP PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (GLAPIENTRYP PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	typedef void (GLAPIENTRYP PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
	typedef void (GLAPIENTRYP PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
	typedef void (GLAPIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei n, GLuint *textures);
	typedef void (GLAPIENTRYP PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	typedef void (GLAPIENTRYP PFNGLTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	typedef void (GLAPIENTRYP PFNGLPIXELSTOREIPROC)(GLenum pname, GLint param);
	typedef void (GLAPIENTRYP PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
	typedef GLenum (GLAPIENTRYP PFNGLGETERRORPROC)();
	typedef void (GLAPIENTRYP PFNGLENABLEPROC)(GLenum cap);
	typedef void (GLAPIENTRYP PFNGLDISABLEPROC)(GLenum cap);

	PFNGLENABLEPROC Enable;
	PFNGLDISABLEPROC Disable;

	PFNGLCLEARPROC Clear;
	PFNGLCLEARCOLORPROC ClearColor;

	PFNGLVIEWPORTPROC Viewport;

	PFNGLGENFRAMEBUFFERSPROC GenFramebuffers;
	PFNGLBINDFRAMEBUFFERPROC BindFramebuffer;
	PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
	PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers;

	PFNGLGENBUFFERSPROC GenBuffers;
	PFNGLBUFFERDATAPROC BufferData;
	PFNGLBINDBUFFERPROC BindBuffer;
	PFNGLDELETEBUFFERSPROC DeleteBuffers;

	PFNGLCREATEPROGRAMPROC CreateProgram;
	PFNGLDELETEPROGRAMPROC DeleteProgram;
	PFNGLLINKPROGRAMPROC LinkProgram;
	PFNGLUSEPROGRAMPROC UseProgram;
	PFNGLGETPROGRAMIVPROC GetProgramiv;
	PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog;
	PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
	PFNGLGETATTRIBLOCATIONPROC GetAttribLocation;
	PFNGLUNIFORM4FVPROC Uniform4fv;
	PFNGLUNIFORM2FVPROC Uniform2fv;
	PFNGLUNIFORM1FVPROC Uniform1fv;
	PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;

	PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC DisableVertexAttribArray;

#if GP_GL_IS_OPENGL_4_CONTEXT
	PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
	PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
	PFNGLBINDVERTEXARRAYPROC BindVertexArray;
#endif

	PFNGLCREATESHADERPROC CreateShader;
	PFNGLCOMPILESHADERPROC CompileShader;
	PFNGLGETSHADERIVPROC GetShaderiv;
	PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
	PFNGLATTACHSHADERPROC AttachShader;
	PFNGLSHADERSOURCEPROC ShaderSource;
	PFNGLDELETESHADERPROC DeleteShader;

	PFNGLDRAWELEMENTSPROC DrawElements;

	PFNGLACTIVETEXTUREPROC ActiveTexture;
	PFNGLBINDTEXTUREPROC BindTexture;
	PFNGLTEXPARAMETERIPROC TexParameteri;
	PFNGLTEXIMAGE2DPROC TexImage2D;
	PFNGLTEXSUBIMAGE2DPROC TexSubImage2D;
	PFNGLPIXELSTOREIPROC PixelStorei;
	PFNGLUNIFORM1IPROC Uniform1i;

	PFNGLGENTEXTURESPROC GenTextures;
	PFNGLDELETETEXTURESPROC DeleteTextures;

	PFNGLGETERRORPROC GetError;

	bool LookUpFunctions();
};

static void CheckGLError(const GpGLFunctions &gl, IGpLogDriver *logger)
{
#if GP_DEBUG_CONFIG
	GLenum errorCode = gl.GetError();
	if (errorCode != 0)
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "GL error reported: %x", static_cast<int>(errorCode));
	}

	assert(errorCode == 0);
#endif
}

class GpGLObject
{
public:
	explicit GpGLObject();

	void AddRef();
	void Release();

protected:
	void InitDriver(GpDisplayDriver_SDL_GL2 *driver, const GpGLFunctions *gl);
	virtual void Destroy() = 0;

	GpDisplayDriver_SDL_GL2 *m_driver;
	const GpGLFunctions *m_gl;
	unsigned int m_count;
};


GpGLObject::GpGLObject()
	: m_count(0)
	, m_driver(nullptr)
{
}

void GpGLObject::AddRef()
{
	++m_count;
}

void GpGLObject::Release()
{
	unsigned int count = m_count;
	if (count == 1)
	{
		this->Destroy();
		return;
	}
	else
		m_count = count - 1;
}

void GpGLObject::InitDriver(GpDisplayDriver_SDL_GL2 *driver, const GpGLFunctions *gl)
{
	m_driver = driver;
	m_gl = gl;
}


template<class T>
class GpGLObjectImpl : public GpGLObject
{
public:
	static T *Create(GpDisplayDriver_SDL_GL2 *driver);

protected:
	virtual bool Init() = 0;
	void Destroy() final override;
};

template<class T>
void GpGLObjectImpl<T>::Destroy()
{
	T *self = static_cast<T*>(this);
	self->~T();
	free(self);
}

class GpGLRenderTargetView final : public GpGLObjectImpl<GpGLRenderTargetView>
{
public:
	GpGLRenderTargetView();
	~GpGLRenderTargetView();

	bool Init() override;
	GLuint GetID() const;

private:
	GLuint m_id;
};


GpGLRenderTargetView::GpGLRenderTargetView()
	: m_id(0)
{
}

GpGLRenderTargetView::~GpGLRenderTargetView()
{
	if (m_id)
		m_gl->DeleteFramebuffers(1, &m_id);
}

bool GpGLRenderTargetView::Init()
{
	m_gl->GenFramebuffers(1, &m_id);

	return m_id != 0;
}

GLuint GpGLRenderTargetView::GetID() const
{
	return m_id;
}

class GpGLTexture final : public GpGLObjectImpl<GpGLTexture>
{
public:
	GpGLTexture();
	~GpGLTexture();

	bool Init() override;
	GLuint GetID() const;

private:
	GLuint m_id;
};


GpGLTexture::GpGLTexture()
	: m_id(0)
{
}

GpGLTexture::~GpGLTexture()
{
	if (m_gl)
		m_gl->DeleteTextures(1, &m_id);
}

bool GpGLTexture::Init()
{
	m_gl->GenTextures(1, &m_id);

	return m_id != 0;
}

GLuint GpGLTexture::GetID() const
{
	return m_id;
}


class GpGLBuffer final : public GpGLObjectImpl<GpGLBuffer>
{
public:
	GpGLBuffer();
	~GpGLBuffer();

	bool Init() override;
	GLuint GetID() const;

private:
	GLuint m_id;
};

GpGLBuffer::GpGLBuffer()
	: m_id(0)
{
}

GpGLBuffer::~GpGLBuffer()
{
	if (m_gl)
		m_gl->DeleteBuffers(1, &m_id);
}

bool GpGLBuffer::Init()
{
	m_gl->GenBuffers(1, &m_id);

	return m_id != 0;
}

GLuint GpGLBuffer::GetID() const
{
	return m_id;
}


struct GpGLVertexArraySpec
{
	const GpGLBuffer *m_buffer;
	GLuint m_index;
	GLint m_size;
	GLenum m_type;
	GLboolean m_normalized;
	GLsizei m_stride;
	GLsizei m_offset;
};

#if GP_GL_IS_OPENGL_4_CONTEXT

class GpGLVertexArray final : public GpGLObjectImpl<GpGLVertexArray>
{
public:
	GpGLVertexArray();
	~GpGLVertexArray();

	bool Init() override;
	GLuint GetID() const;

	bool InitWithSpecs(const GpGLVertexArraySpec *specs, size_t numSpecs);
	void Activate(const GLint *locations);
	void Deactivate(const GLint *locations);

private:
	GLuint m_id;
};


GpGLVertexArray::GpGLVertexArray()
	: m_id(0)
{
}

GpGLVertexArray::~GpGLVertexArray()
{
	if (m_id)
		m_gl->DeleteVertexArrays(1, &m_id);
}

bool GpGLVertexArray::Init()
{
	m_gl->GenVertexArrays(1, &m_id);
	return m_id != 0;
}

GLuint GpGLVertexArray::GetID() const
{
	return m_id;
}


bool GpGLVertexArray::InitWithSpecs(const GpGLVertexArraySpec *specs, size_t numSpecs)
{
	m_gl->BindVertexArray(m_id);
	for (size_t i = 0; i < numSpecs; i++)
	{
		const GpGLVertexArraySpec &spec = specs[i];
		m_gl->BindBuffer(GL_ARRAY_BUFFER, spec.m_buffer->GetID());
		m_gl->VertexAttribPointer(spec.m_index, spec.m_size, spec.m_type, spec.m_normalized, spec.m_stride, static_cast<const char*>(nullptr) + spec.m_offset);
		m_gl->EnableVertexAttribArray(spec.m_index);
		m_gl->BindBuffer(GL_ARRAY_BUFFER, 0);
	}
	m_gl->BindVertexArray(0);

	return true;
}

void GpGLVertexArray::Activate(const GLint *locations)
{
	m_gl->BindVertexArray(m_id);
}

void GpGLVertexArray::Deactivate(const GLint *locations)
{
	m_gl->BindVertexArray(0);
}
#else
class GpGLVertexArray final : public GpGLObjectImpl<GpGLVertexArray>
{
public:
	GpGLVertexArray();
	~GpGLVertexArray();

	bool Init() override;

	bool InitWithSpecs(const GpGLVertexArraySpec *specs, size_t numSpecs);
	void Activate(const GLint *locations);
	void Deactivate(const GLint *locations);

private:
	GpGLVertexArraySpec *m_specs;
	size_t m_numSpecs;
};


GpGLVertexArray::GpGLVertexArray()
	: m_specs(nullptr)
	, m_numSpecs(0)
{
}

GpGLVertexArray::~GpGLVertexArray()
{
	if (m_specs)
		free(m_specs);
}

bool GpGLVertexArray::Init()
{
	return true;
}

bool GpGLVertexArray::InitWithSpecs(const GpGLVertexArraySpec *specs, size_t numSpecs)
{
	m_specs = static_cast<GpGLVertexArraySpec*>(malloc(sizeof(GpGLVertexArraySpec) * numSpecs));
	if (!m_specs)
		return false;

	for (size_t i = 0; i < numSpecs; i++)
		m_specs[i] = specs[i];

	m_numSpecs = numSpecs;

	return true;
}

void GpGLVertexArray::Activate(const GLint *locations)
{
	size_t numSpecs = m_numSpecs;
	for (size_t i = 0; i < numSpecs; i++)
	{
		if (locations[i] < 0)
			continue;

		const GpGLVertexArraySpec &spec = m_specs[i];
		m_gl->BindBuffer(GL_ARRAY_BUFFER, spec.m_buffer->GetID());
		m_gl->VertexAttribPointer(locations[i], spec.m_size, spec.m_type, spec.m_normalized, spec.m_stride, static_cast<const char*>(nullptr) + spec.m_offset);
		m_gl->EnableVertexAttribArray(locations[i]);
		m_gl->BindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

void GpGLVertexArray::Deactivate(const GLint *locations)
{
	size_t numSpecs = m_numSpecs;
	for (size_t i = 0; i < numSpecs; i++)
	{
		if (locations[i] < 0)
			continue;

		m_gl->DisableVertexAttribArray(locations[i]);
	}
}
#endif

class GpGLProgram final : public GpGLObjectImpl<GpGLProgram>
{
public:
	GpGLProgram();
	~GpGLProgram();

	bool Init() override;
	GLuint GetID() const;

private:
	GLuint m_id;
};

GpGLProgram::GpGLProgram()
	: m_id(0)
{
}

GpGLProgram::~GpGLProgram()
{
	if (m_id)
		m_gl->DeleteProgram(m_id);
}

bool GpGLProgram::Init()
{
	m_id = m_gl->CreateProgram();

	return m_id != 0;
}

GLuint GpGLProgram::GetID() const
{
	return m_id;
}

template<GLuint TShaderType>
class GpGLShader final : public GpGLObjectImpl<GpGLShader<TShaderType> >
{
public:
	GpGLShader();
	~GpGLShader();

	bool Init() override;
	GLuint GetID() const;

private:
	GLuint m_id;
};

template<GLuint TShaderType>
GpGLShader<TShaderType>::GpGLShader()
	: m_id(0)
{
}

template<GLuint TShaderType>
GpGLShader<TShaderType>::~GpGLShader()
{
	if (m_id != 0)
		this->m_gl->DeleteShader(m_id);
}

template<GLuint TShaderType>
bool GpGLShader<TShaderType>::Init()
{
	m_id = this->m_gl->CreateShader(TShaderType);
	return m_id != 0;
}

template<GLuint TShaderType>
GLuint GpGLShader<TShaderType>::GetID() const
{
	return m_id;
}

class GpDisplayDriverSurface_GL2 : public IGpDisplayDriverSurface
{
public:
	GpDisplayDriverSurface_GL2(GpDisplayDriver_SDL_GL2 *driver, size_t width, size_t height, size_t pitch, GpGLTexture *texture, GpPixelFormat_t pixelFormat, IGpDisplayDriver::SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext);
	~GpDisplayDriverSurface_GL2();

	static GpDisplayDriverSurface_GL2 *Create(GpDisplayDriver_SDL_GL2 *driver, size_t width, size_t height, size_t pitch, GpPixelFormat_t pixelFormat, GpDisplayDriverSurface_GL2 *prevSurface, IGpDisplayDriver::SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext);

	void Upload(const void *data, size_t x, size_t y, size_t width, size_t height, size_t pitch);
	void UploadEntire(const void *data, size_t pitch);
	void Destroy();

	void DestroyAll();
	bool RecreateAll();

	size_t GetImageWidth() const;
	size_t GetPaddedTextureWidth() const;
	size_t GetHeight() const;
	GpPixelFormat_t GetPixelFormat() const;
	GpGLTexture *GetTexture() const;

private:
	bool Init(GpDisplayDriverSurface_GL2 *prevSurface);
	bool RecreateSingle();
	GLenum ResolveGLFormat() const;
	GLenum ResolveGLInternalFormat() const;
	GLenum ResolveGLType() const;

	GpComPtr<GpGLTexture> m_texture;
	const GpGLFunctions *m_gl;
	GpPixelFormat_t m_pixelFormat;
	size_t m_imageWidth;
	size_t m_paddedTextureWidth;
	size_t m_pitch;
	size_t m_height;

	GpDisplayDriver_SDL_GL2 *m_driver;
	GpDisplayDriverSurface_GL2 *m_next;
	GpDisplayDriverSurface_GL2 *m_prev;

	IGpDisplayDriver::SurfaceInvalidateCallback_t m_invalidateCallback;
	void *m_invalidateContext;
};

class GpCursor_SDL2 final : public IGpCursor
{
public:
	explicit GpCursor_SDL2(SDL_Cursor *cursor);

	SDL_Cursor* GetCursor() const;

	void IncRef();
	void DecRef();

	void Destroy() override { this->DecRef(); }

private:
	SDL_Cursor *m_cursor;
	unsigned int m_count;
};

GpCursor_SDL2::GpCursor_SDL2(SDL_Cursor *cursor)
	: m_cursor(cursor)
	, m_count(1)
{
}

SDL_Cursor* GpCursor_SDL2::GetCursor() const
{
	return m_cursor;
}

void GpCursor_SDL2::IncRef()
{
	++m_count;
}

void GpCursor_SDL2::DecRef()
{
	if (m_count == 1)
		delete this;
	else
		--m_count;
}


class GpDisplayDriver_SDL_GL2 final : public IGpDisplayDriver, public IGpPrefsHandler
{
public:
	explicit GpDisplayDriver_SDL_GL2(const GpDisplayDriverProperties &properties);
	~GpDisplayDriver_SDL_GL2();

	bool Init() GP_ASYNCIFY_PARANOID_OVERRIDE;
	void ServeTicks(int tickCount) GP_ASYNCIFY_PARANOID_OVERRIDE;
	void ForceSync() override;
	void Shutdown() GP_ASYNCIFY_PARANOID_OVERRIDE;

	void TranslateSDLMessage(const SDL_Event *msg, IGpVOSEventQueue *eventQueue, float pixelScaleX, float pixelScaleY, bool obstructiveTextInput);

	void GetInitialDisplayResolution(unsigned int *width, unsigned int *height) override;
	IGpDisplayDriverSurface *CreateSurface(size_t width, size_t height, size_t pitch, GpPixelFormat_t pixelFormat, SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext) override;
	void DrawSurface(IGpDisplayDriverSurface *surface, int32_t x, int32_t y, size_t width, size_t height, const GpDisplayDriverSurfaceEffects *effects) override;
	IGpCursor *CreateBWCursor(size_t width, size_t height, const void *pixelData, const void *maskData, size_t hotSpotX, size_t hotSpotY) GP_ASYNCIFY_PARANOID_OVERRIDE;
	IGpCursor *CreateColorCursor(size_t width, size_t height, const void *pixelDataRGBA, size_t hotSpotX, size_t hotSpotY) GP_ASYNCIFY_PARANOID_OVERRIDE;
	void SetCursor(IGpCursor *cursor) override;
	void SetStandardCursor(EGpStandardCursor_t standardCursor) override;
	void UpdatePalette(const void *paletteData) override;
	void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
	void SetBackgroundDarkenEffect(bool isDark) override;
	void SetUseICCProfile(bool useICCProfile) override;
	void RequestToggleFullScreen(uint32_t timestamp) override;
	void RequestResetVirtualResolution() override;
	bool IsFullScreen() const override;
	const GpDisplayDriverProperties &GetProperties() const override;
	IGpPrefsHandler *GetPrefsHandler() const override;

	bool SupportsSizedFormats() const;

	void ApplyPrefs(const void *identifier, size_t identifierSize, const void *contents, size_t contentsSize, uint32_t version) override;
	bool SavePrefs(void *context, WritePrefsFunc_t writeFunc) override;

	void UnlinkSurface(GpDisplayDriverSurface_GL2 *surface, GpDisplayDriverSurface_GL2 *prev, GpDisplayDriverSurface_GL2 *next);

	const GpGLFunctions *GetGLFunctions() const;

	template<GLuint TShaderType> GpComPtr<GpGLShader<TShaderType> > CreateShader(const char *shaderSrc);

private:
	struct DrawQuadPixelFloatConstants
	{
		float m_modulation[4];

		float m_flickerAxis[2];
		float m_flickerStart;
		float m_flickerEnd;

		float m_desaturation;
		float m_unused[3];
	};

	struct CompactedPresentHistoryItem
	{
		std::chrono::time_point<std::chrono::high_resolution_clock>::duration m_timestamp;
		unsigned int m_numFrames;
	};

	void StartOpenGLForWindow(IGpLogDriver *logger);
	bool InitResources(uint32_t physicalWidth, uint32_t physicalHeight, uint32_t virtualWidth, uint32_t virtualHeight);

	void BecomeFullScreen();
	void BecomeWindowed();

	void SynchronizeCursors();
	void ChangeToCursor(SDL_Cursor *cursor);
	void ChangeToStandardCursor(EGpStandardCursor_t cursor);

	bool ResizeOpenGLWindow(uint32_t &windowWidth, uint32_t &windowHeight, uint32_t desiredWidth, uint32_t desiredHeight, IGpLogDriver *logger);
	bool InitBackBuffer(uint32_t width, uint32_t height);

	void ScaleVirtualScreen();

	bool SyncRender();

	GpGLFunctions m_gl;
	GpDisplayDriverProperties m_properties;

	struct DrawQuadProgram
	{
		GpComPtr<GpGLProgram> m_program;
		GLint m_vertexNDCOriginAndDimensionsLocation;
		GLint m_vertexSurfaceDimensionsLocation;
		GLint m_vertexPosUVLocation;
		GLint m_pixelModulationLocation;
		GLint m_pixelFlickerAxisLocation;
		GLint m_pixelFlickerStartThresholdLocation;
		GLint m_pixelFlickerEndThresholdLocation;
		GLint m_pixelDesaturationLocation;
		GLint m_pixelSurfaceTextureLocation;
		GLint m_pixelPaletteTextureLocation;

		bool Link(GpDisplayDriver_SDL_GL2 *driver, const GpGLShader<GL_VERTEX_SHADER> *vertexShader, const GpGLShader<GL_FRAGMENT_SHADER> *pixelShader);
	};

	struct BlitQuadProgram
	{
		GpComPtr<GpGLProgram> m_program;
		GLint m_vertexNDCOriginAndDimensionsLocation;
		GLint m_vertexSurfaceDimensionsLocation;
		GLint m_pixelDXDYDimensionsLocation;
		GLint m_vertexPosUVLocation;
		GLint m_pixelSurfaceTextureLocation;

		bool Link(GpDisplayDriver_SDL_GL2 *driver, const GpGLShader<GL_VERTEX_SHADER> *vertexShader, const GpGLShader<GL_FRAGMENT_SHADER> *pixelShader);
	};

	struct InstancedResources
	{
		GpComPtr<GpGLRenderTargetView> m_virtualScreenTextureRTV;
		GpComPtr<GpGLTexture> m_virtualScreenTexture;

		GpComPtr<GpGLRenderTargetView> m_upscaleTextureRTV;
		GpComPtr<GpGLTexture> m_upscaleTexture;

		GpComPtr<GpGLVertexArray> m_quadVertexArray;
		GpComPtr<GpGLBuffer> m_quadVertexBufferKeepalive;
		GpComPtr<GpGLBuffer> m_quadIndexBuffer;

		GpComPtr<GpGLTexture> m_paletteTexture;

		BlitQuadProgram m_scaleQuadProgram;
		BlitQuadProgram m_copyQuadProgram;

		DrawQuadProgram m_drawQuadPaletteNoFlickerProgram;
		DrawQuadProgram m_drawQuadPaletteFlickerProgram;
		DrawQuadProgram m_drawQuad15NoFlickerProgram;
		DrawQuadProgram m_drawQuad15FlickerProgram;
		DrawQuadProgram m_drawQuad32NoFlickerProgram;
		DrawQuadProgram m_drawQuad32FlickerProgram;
		DrawQuadProgram m_drawQuadPaletteICCNoFlickerProgram;
		DrawQuadProgram m_drawQuadPaletteICCFlickerProgram;
		DrawQuadProgram m_drawQuad15ICCNoFlickerProgram;
		DrawQuadProgram m_drawQuad15ICCFlickerProgram;
		DrawQuadProgram m_drawQuad32ICCNoFlickerProgram;
		DrawQuadProgram m_drawQuad32ICCFlickerProgram;
	};

	InstancedResources m_res;

	SDL_Window *m_window;
	SDL_GLContext m_glContext;

	SDL_Cursor *m_waitCursor;
	SDL_Cursor *m_iBeamCursor;
	SDL_Cursor *m_arrowCursor;
	bool m_cursorIsHidden;
	bool m_contextLost;

	bool m_isResettingSwapChain;

	bool m_isFullScreen;
	bool m_isFullScreenDesired;
	bool m_isResolutionResetDesired;
	int m_windowModeRevertX;
	int m_windowModeRevertY;
	int m_windowModeRevertWidth;
	int m_windowModeRevertHeight;
	uint32_t m_lastFullScreenToggleTimeStamp;

	std::chrono::high_resolution_clock::duration m_frameTimeAccumulated;
	std::chrono::high_resolution_clock::duration m_frameTimeSliceSize;

	uint32_t m_windowWidthPhysical;	// Physical resolution is the resolution of the actual window
	uint32_t m_windowHeightPhysical;
	uint32_t m_windowWidthVirtual;		// Virtual resolution is the resolution reported to the game
	uint32_t m_windowHeightVirtual;
	uint32_t m_initialWidthVirtual;		// Virtual resolution is the resolution reported to the game
	uint32_t m_initialHeightVirtual;
	float m_pixelScaleX;
	float m_pixelScaleY;

	bool m_useUpscaleFilter;
	uint32_t m_upscaleTextureWidth;
	uint32_t m_upscaleTextureHeight;

	GpCursor_SDL2 *m_activeCursor;
	GpCursor_SDL2 *m_pendingCursor;
	EGpStandardCursor_t m_currentStandardCursor;
	EGpStandardCursor_t m_pendingStandardCursor;
	bool m_mouseIsInClientArea;

	float m_bgColor[4];
	bool m_bgIsDark;

	bool m_useICCProfile;

	std::chrono::time_point<std::chrono::high_resolution_clock>::duration m_syncTimeBase;
	GpRingBuffer<CompactedPresentHistoryItem, 60> m_presentHistory;

	GpDisplayDriverSurface_GL2 *m_firstSurface;
	GpDisplayDriverSurface_GL2 *m_lastSurface;

	uint8_t m_paletteStorage[256 * 4 + GP_SYSTEM_MEMORY_ALIGNMENT];
	uint8_t *m_paletteData;

	bool m_textInputEnabled;
};


GpDisplayDriverSurface_GL2::GpDisplayDriverSurface_GL2(GpDisplayDriver_SDL_GL2 *driver, size_t width, size_t height, size_t pitch, GpGLTexture *texture, GpPixelFormat_t pixelFormat, IGpDisplayDriver::SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext)
	: m_gl(driver->GetGLFunctions())
	, m_texture(texture)
	, m_pixelFormat(pixelFormat)
	, m_imageWidth(width)
	, m_paddedTextureWidth(0)
	, m_height(height)
	, m_pitch(pitch)
	, m_driver(driver)
	, m_prev(nullptr)
	, m_next(nullptr)
	, m_invalidateCallback(invalidateCallback)
	, m_invalidateContext(invalidateContext)
{
	size_t paddingPixels = 0;

	switch (pixelFormat)
	{
	case GpPixelFormats::kBW1:
	case GpPixelFormats::k8BitStandard:
	case GpPixelFormats::k8BitCustom:
		paddingPixels = pitch - width;
		break;
	case GpPixelFormats::kRGB555:
		assert(pitch % 2 == 0);
		paddingPixels = pitch / 2 - width;
		break;
	case GpPixelFormats::kRGB24:
		assert(pitch % 3 == 0);
		paddingPixels = pitch / 3 - width;
		break;
	case GpPixelFormats::kRGB32:
		assert(pitch % 4 == 0);
		paddingPixels = pitch / 4 - width;
		break;
    default:
        assert(false);
        paddingPixels = 0;
	}

	m_paddedTextureWidth = width + paddingPixels;
}

GpDisplayDriverSurface_GL2::~GpDisplayDriverSurface_GL2()
{
	if (m_prev)
		m_prev->m_next = m_next;
	if (m_next)
		m_next->m_prev = m_prev;

	m_driver->UnlinkSurface(this, m_prev, m_next);
}

GpDisplayDriverSurface_GL2 *GpDisplayDriverSurface_GL2::Create(GpDisplayDriver_SDL_GL2 *driver, size_t width, size_t height, size_t pitch, GpPixelFormat_t pixelFormat, GpDisplayDriverSurface_GL2 *prevSurface, IGpDisplayDriver::SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext)
{
	GpComPtr<GpGLTexture> texture = GpComPtr<GpGLTexture>(GpGLTexture::Create(driver));
	if (!texture)
		return nullptr;

	GpDisplayDriverSurface_GL2 *surface = static_cast<GpDisplayDriverSurface_GL2*>(malloc(sizeof(GpDisplayDriverSurface_GL2)));
	if (!surface)
		return nullptr;

	new (surface) GpDisplayDriverSurface_GL2(driver, width, height, pitch, texture, pixelFormat, invalidateCallback, invalidateContext);
	if (!surface->Init(prevSurface))
	{
		surface->Destroy();
		return nullptr;
	}

	return surface;
}

void GpDisplayDriverSurface_GL2::Upload(const void *data, size_t x, size_t y, size_t width, size_t height, size_t pitch)
{
	m_gl->BindTexture(GL_TEXTURE_2D, m_texture->GetID());
	m_gl->TexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, ResolveGLFormat(), ResolveGLType(), data);
	m_gl->BindTexture(GL_TEXTURE_2D, 0);
}

void GpDisplayDriverSurface_GL2::UploadEntire(const void *data, size_t pitch)
{
	assert(pitch == m_pitch);

	CheckGLError(*m_gl, m_driver->GetProperties().m_logger);

	const GLint internalFormat = ResolveGLInternalFormat();
	const GLenum glFormat = ResolveGLFormat();
	const GLenum glType = ResolveGLType();

	m_gl->BindTexture(GL_TEXTURE_2D, m_texture->GetID());
	m_gl->TexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_paddedTextureWidth, m_height, 0, glFormat, glType, data);
	m_gl->BindTexture(GL_TEXTURE_2D, 0);

	CheckGLError(*m_gl, m_driver->GetProperties().m_logger);
}

void GpDisplayDriverSurface_GL2::Destroy()
{
	this->~GpDisplayDriverSurface_GL2();
	free(this);
}

void GpDisplayDriverSurface_GL2::DestroyAll()
{
	for (GpDisplayDriverSurface_GL2 *scan = this; scan; scan = scan->m_next)
	{
		scan->m_invalidateCallback(scan->m_invalidateContext);
		scan->m_texture = nullptr;
	}
}

bool GpDisplayDriverSurface_GL2::RecreateAll()
{
	for (GpDisplayDriverSurface_GL2 *scan = this; scan; scan = scan->m_next)
	{
		if (!scan->RecreateSingle())
			return false;
	}

	return true;
}

size_t GpDisplayDriverSurface_GL2::GetImageWidth() const
{
	return m_imageWidth;
}

size_t GpDisplayDriverSurface_GL2::GetPaddedTextureWidth() const
{
	return m_paddedTextureWidth;
}

size_t GpDisplayDriverSurface_GL2::GetHeight() const
{
	return m_height;
}

GpPixelFormat_t GpDisplayDriverSurface_GL2::GetPixelFormat() const
{
	return m_pixelFormat;
}

GpGLTexture *GpDisplayDriverSurface_GL2::GetTexture() const
{
	return m_texture;
}


bool GpDisplayDriverSurface_GL2::Init(GpDisplayDriverSurface_GL2 *prevSurface)
{
	CheckGLError(*m_gl, m_driver->GetProperties().m_logger);

	m_gl->BindTexture(GL_TEXTURE_2D, m_texture->GetID());
	m_gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	m_gl->TexImage2D(GL_TEXTURE_2D, 0, ResolveGLInternalFormat(), m_paddedTextureWidth, m_height, 0, ResolveGLFormat(), ResolveGLType(), nullptr);
	m_gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	m_gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	m_gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_gl->BindTexture(GL_TEXTURE_2D, 0);

	CheckGLError(*m_gl, m_driver->GetProperties().m_logger);

	m_prev = prevSurface;

	return true;
}

bool GpDisplayDriverSurface_GL2::RecreateSingle()
{
	m_texture = GpGLTexture::Create(m_driver);
	return m_texture != nullptr && this->Init(m_prev);
}

GLenum GpDisplayDriverSurface_GL2::ResolveGLFormat() const
{
	switch (m_pixelFormat)
	{
	case GpPixelFormats::k8BitCustom:
	case GpPixelFormats::k8BitStandard:
	case GpPixelFormats::kBW1:
		return m_driver->SupportsSizedFormats() ? GL_RED : GL_LUMINANCE;
	case GpPixelFormats::kRGB24:
	case GpPixelFormats::kRGB555:
		return GL_RGB;
	case GpPixelFormats::kRGB32:
		return GL_RGBA;
	default:
		return GL_RGBA;
	}
}

GLenum GpDisplayDriverSurface_GL2::ResolveGLInternalFormat() const
{
	if (m_driver->SupportsSizedFormats())
	{
		switch (m_pixelFormat)
		{
		case GpPixelFormats::k8BitCustom:
		case GpPixelFormats::k8BitStandard:
		case GpPixelFormats::kBW1:
			return GL_R8;
		case GpPixelFormats::kRGB24:
			return GL_RGB8;
		case GpPixelFormats::kRGB555:
			return GL_RGB5;
		case GpPixelFormats::kRGB32:
			return GL_RGBA8;
		default:
			return GL_RGBA8;
		}
	}
	else
	{
		switch (m_pixelFormat)
		{
		case GpPixelFormats::k8BitCustom:
		case GpPixelFormats::k8BitStandard:
		case GpPixelFormats::kBW1:
			return GL_LUMINANCE;
		case GpPixelFormats::kRGB24:
			return GL_RGB;
		case GpPixelFormats::kRGB555:
			return GL_RGBA;
		case GpPixelFormats::kRGB32:
			return GL_RGBA;
		default:
			return GL_RGBA;
		}
	}
}

GLenum GpDisplayDriverSurface_GL2::ResolveGLType() const
{
	switch (m_pixelFormat)
	{
	case GpPixelFormats::k8BitCustom:
	case GpPixelFormats::k8BitStandard:
	case GpPixelFormats::kBW1:
		return GL_UNSIGNED_BYTE;
	case GpPixelFormats::kRGB24:
		return GL_UNSIGNED_BYTE;
	case GpPixelFormats::kRGB555:
		return GL_UNSIGNED_SHORT_5_5_5_1;
	case GpPixelFormats::kRGB32:
		return GL_UNSIGNED_BYTE;
	default:
		return GL_UNSIGNED_BYTE;
	}
}


GpDisplayDriver_SDL_GL2::GpDisplayDriver_SDL_GL2(const GpDisplayDriverProperties &properties)
	: m_window(nullptr)
	, m_frameTimeAccumulated(std::chrono::high_resolution_clock::duration::zero())
	, m_frameTimeSliceSize(std::chrono::high_resolution_clock::duration::zero())
	, m_windowWidthPhysical(640)
	, m_windowHeightPhysical(480)
	, m_windowWidthVirtual(640)
	, m_windowHeightVirtual(480)
	, m_initialWidthVirtual(640)
	, m_initialHeightVirtual(480)
	, m_pixelScaleX(1.0f)
	, m_pixelScaleY(1.0f)
	, m_useUpscaleFilter(false)
	, m_upscaleTextureWidth(0)
	, m_upscaleTextureHeight(0)
	, m_pendingCursor(nullptr)
	, m_activeCursor(nullptr)
	, m_currentStandardCursor(EGpStandardCursors::kArrow)
	, m_pendingStandardCursor(EGpStandardCursors::kArrow)
	, m_mouseIsInClientArea(false)
	, m_isFullScreen(false)
	, m_isFullScreenDesired(false)
	, m_isResolutionResetDesired(false)
	, m_windowModeRevertX(200)
	, m_windowModeRevertY(200)
	, m_windowModeRevertWidth(640)
	, m_windowModeRevertHeight(480)
	, m_lastFullScreenToggleTimeStamp(0)
	, m_bgIsDark(false)
	, m_useICCProfile(false)
	, m_properties(properties)
	, m_syncTimeBase(std::chrono::time_point<std::chrono::high_resolution_clock>::duration::zero())
	, m_waitCursor(nullptr)
	, m_iBeamCursor(nullptr)
	, m_arrowCursor(nullptr)
	, m_cursorIsHidden(false)
	, m_contextLost(true)
	, m_lastSurface(nullptr)
	, m_firstSurface(nullptr)
	, m_textInputEnabled(false)
{
	m_bgColor[0] = 0.f;
	m_bgColor[1] = 0.f;
	m_bgColor[2] = 0.f;
	m_bgColor[3] = 1.f;

	// Stupid hack to detect mobile...
	m_isFullScreenDesired = m_properties.m_systemServices->IsFullscreenOnStartup();

	const intmax_t periodNum = std::chrono::high_resolution_clock::period::num;
	const intmax_t periodDen = std::chrono::high_resolution_clock::period::den;

	m_frameTimeSliceSize = std::chrono::high_resolution_clock::duration(periodDen * static_cast<intmax_t>(properties.m_frameTimeLockNumerator) / static_cast<intmax_t>(properties.m_frameTimeLockDenominator) / periodNum);

	m_waitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	m_iBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	m_arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

	m_paletteData = m_paletteStorage;
	while (reinterpret_cast<uintptr_t>(m_paletteData) % GP_SYSTEM_MEMORY_ALIGNMENT != 0)
		m_paletteData++;

	memset(m_paletteData, 255, 256 * 4);
}

template<class T>
static bool LookupOpenGLFunction(T &target, const char *name)
{
	target = static_cast<T>(nullptr);
	void *proc = SDL_GL_GetProcAddress(name);
	if (proc)
	{
		target = reinterpret_cast<T>(proc);
		return true;
	}
	else
		return false;
}

#define LOOKUP_FUNC(func) do { if (!LookupOpenGLFunction(this->func, "gl" #func)) return false; } while(false)


bool GpGLFunctions::LookUpFunctions()
{
	LOOKUP_FUNC(Enable);
	LOOKUP_FUNC(Disable);

	LOOKUP_FUNC(Clear);
	LOOKUP_FUNC(ClearColor);

	LOOKUP_FUNC(Viewport);

	LOOKUP_FUNC(GenFramebuffers);
	LOOKUP_FUNC(BindFramebuffer);
	LOOKUP_FUNC(FramebufferTexture2D);
	LOOKUP_FUNC(CheckFramebufferStatus);
	LOOKUP_FUNC(DeleteFramebuffers);

	LOOKUP_FUNC(CreateProgram);
	LOOKUP_FUNC(DeleteProgram);
	LOOKUP_FUNC(LinkProgram);
	LOOKUP_FUNC(UseProgram);
	LOOKUP_FUNC(GetProgramiv);
	LOOKUP_FUNC(GetProgramInfoLog);

	LOOKUP_FUNC(GenBuffers);
	LOOKUP_FUNC(BufferData);
	LOOKUP_FUNC(BindBuffer);
	LOOKUP_FUNC(DeleteBuffers);

	LOOKUP_FUNC(GetUniformLocation);
	LOOKUP_FUNC(GetAttribLocation);
	LOOKUP_FUNC(Uniform4fv);
	LOOKUP_FUNC(Uniform2fv);
	LOOKUP_FUNC(Uniform1fv);
	LOOKUP_FUNC(VertexAttribPointer);

	LOOKUP_FUNC(EnableVertexAttribArray);
	LOOKUP_FUNC(DisableVertexAttribArray);

#if GP_GL_IS_OPENGL_4_CONTEXT
	LOOKUP_FUNC(GenVertexArrays);
	LOOKUP_FUNC(DeleteVertexArrays);
	LOOKUP_FUNC(BindVertexArray);
#endif

	LOOKUP_FUNC(CreateShader);
	LOOKUP_FUNC(CompileShader);
	LOOKUP_FUNC(GetShaderiv);
	LOOKUP_FUNC(GetShaderInfoLog);
	LOOKUP_FUNC(AttachShader);
	LOOKUP_FUNC(ShaderSource);
	LOOKUP_FUNC(DeleteShader);

	LOOKUP_FUNC(DrawElements);

	LOOKUP_FUNC(ActiveTexture);
	LOOKUP_FUNC(BindTexture);
	LOOKUP_FUNC(TexParameteri);
	LOOKUP_FUNC(TexImage2D);
	LOOKUP_FUNC(TexSubImage2D);
	LOOKUP_FUNC(PixelStorei);
	LOOKUP_FUNC(Uniform1i);

	LOOKUP_FUNC(GenTextures);
	LOOKUP_FUNC(DeleteTextures);

	LOOKUP_FUNC(GetError);

	return true;
}

GpDisplayDriver_SDL_GL2::~GpDisplayDriver_SDL_GL2()
{
	SDL_DestroyWindow(m_window);
}

bool GpDisplayDriver_SDL_GL2::Init()
{
#if GP_GL_IS_OPENGL_4_CONTEXT
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	IGpLogDriver *logger = m_properties.m_logger;

	uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	if (m_properties.m_systemServices->IsFullscreenOnStartup())
	{
		windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		m_isFullScreen = true;
	}
	else
		windowFlags |= SDL_WINDOW_RESIZABLE;

	m_window = SDL_CreateWindow(GP_APPLICATION_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_windowWidthPhysical, m_windowHeightPhysical, windowFlags);
	if (!m_window)
	{
		if (logger)
			logger->Printf(IGpLogDriver::Category_Error, "Failed to create window: %s", SDL_GetError());

		return false;
	}

	if (m_isFullScreen)
	{
		m_windowModeRevertWidth = m_windowWidthPhysical;
		m_windowModeRevertHeight = m_windowHeightPhysical;

		int windowWidth = 0;
		int windowHeight = 0;
		SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

		if (logger)
			logger->Printf(IGpLogDriver::Category_Information, "Initialized fullscreen SDL window %i x %i", windowWidth, windowHeight);

		m_windowWidthPhysical = windowWidth;
		m_windowHeightPhysical = windowHeight;

		uint32_t desiredWidth = windowWidth;
		uint32_t desiredHeight = windowHeight;
		uint32_t virtualWidth = m_windowWidthVirtual;
		uint32_t virtualHeight = m_windowHeightVirtual;
		float pixelScaleX = m_pixelScaleX;
		float pixelScaleY = m_pixelScaleY;

		if (m_properties.m_adjustRequestedResolutionFunc(m_properties.m_adjustRequestedResolutionFuncContext, desiredWidth, desiredHeight, virtualWidth, virtualHeight, pixelScaleX, pixelScaleY))
		{
			m_windowWidthVirtual = virtualWidth;
			m_windowHeightVirtual = virtualHeight;
			m_pixelScaleX = pixelScaleX;
			m_pixelScaleY = pixelScaleY;

			if (logger)
				logger->Printf(IGpLogDriver::Category_Information, "AdjustedRequestedResolution succeeded.  Virtual dimensions %i x %i  Pixel scale %f x %f", static_cast<int>(virtualWidth), static_cast<int>(virtualHeight), static_cast<float>(pixelScaleX), static_cast<float>(pixelScaleY));
		}
		else
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "AdjustedRequestedResolution failed!");
		}
	}

	const bool obstructiveTextInput = m_properties.m_systemServices->IsTextInputObstructive();

	if (!obstructiveTextInput)
		SDL_StartTextInput();

	StartOpenGLForWindow(logger);

	if (!m_gl.LookUpFunctions())
		return false;

	m_initialWidthVirtual = m_windowWidthVirtual;
	m_initialHeightVirtual = m_windowHeightVirtual;

	return true;
}

void GpDisplayDriver_SDL_GL2::ServeTicks(int ticks)
{
	IGpLogDriver *logger = m_properties.m_logger;
	const bool obstructiveTextInput = m_properties.m_systemServices->IsTextInputObstructive();

	for (;;)
	{
		SDL_Event msg;
		if (SDL_PollEvent(&msg) != 0)
		{
			switch (msg.type)
			{
			case SDL_MOUSEMOTION:
			{
				if (!m_mouseIsInClientArea)
					m_mouseIsInClientArea = true;
			}
			break;
			//case SDL_MOUSELEAVE:	// Does SDL support this??
			//	m_mouseIsInClientArea = false;
			//	break;
			case SDL_RENDER_DEVICE_RESET:
			case SDL_RENDER_TARGETS_RESET:
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "Triggering GL context reset due to device loss (Type: %i)", static_cast<int>(msg.type));

				m_contextLost = true;
			}
			break;
			case SDL_CONTROLLERAXISMOTION:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERDEVICEREMAPPED:
				if (IGpInputDriverSDLGamepad *gamepadDriver = IGpInputDriverSDLGamepad::GetInstance())
					gamepadDriver->ProcessSDLEvent(msg);
				break;
			}

			TranslateSDLMessage(&msg, m_properties.m_eventQueue, m_pixelScaleX, m_pixelScaleY, obstructiveTextInput);
		}
		else
		{
			if (m_isFullScreen != m_isFullScreenDesired)
			{
				if (m_isFullScreenDesired)
					BecomeFullScreen();
				else
					BecomeWindowed();

				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "Triggering GL context reset due to fullscreen state change");

				m_contextLost = true;
				continue;
			}

			int clientWidth = 0;
			int clientHeight = 0;
			SDL_GetWindowSize(m_window, &clientWidth, &clientHeight);

			unsigned int desiredWidth = clientWidth;
			unsigned int desiredHeight = clientHeight;
			if (desiredWidth != m_windowWidthPhysical || desiredHeight != m_windowHeightPhysical || m_isResolutionResetDesired)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "Detected window size change");

				uint32_t prevWidthPhysical = m_windowWidthPhysical;
				uint32_t prevHeightPhysical = m_windowHeightPhysical;
				uint32_t prevWidthVirtual = m_windowWidthVirtual;
				uint32_t prevHeightVirtual = m_windowHeightVirtual;
				uint32_t virtualWidth = m_windowWidthVirtual;
				uint32_t virtualHeight = m_windowHeightVirtual;
				float pixelScaleX = 1.0f;
				float pixelScaleY = 1.0f;

				if (m_properties.m_adjustRequestedResolutionFunc(m_properties.m_adjustRequestedResolutionFuncContext, desiredWidth, desiredHeight, virtualWidth, virtualHeight, pixelScaleX, pixelScaleY))
				{
					bool resizedOK = ResizeOpenGLWindow(m_windowWidthPhysical, m_windowHeightPhysical, desiredWidth, desiredHeight, logger);

					if (!resizedOK)
						break;	// Critical video driver error, exit

					m_windowWidthVirtual = virtualWidth;
					m_windowHeightVirtual = virtualHeight;
					m_pixelScaleX = pixelScaleX;
					m_pixelScaleY = pixelScaleY;
					m_isResolutionResetDesired = false;

					if (GpVOSEvent *resizeEvent = m_properties.m_eventQueue->QueueEvent())
					{
						resizeEvent->m_eventType = GpVOSEventTypes::kVideoResolutionChanged;
						resizeEvent->m_event.m_resolutionChangedEvent.m_prevWidth = prevWidthVirtual;
						resizeEvent->m_event.m_resolutionChangedEvent.m_prevHeight = prevHeightVirtual;
						resizeEvent->m_event.m_resolutionChangedEvent.m_newWidth = m_windowWidthVirtual;
						resizeEvent->m_event.m_resolutionChangedEvent.m_newHeight = m_windowHeightVirtual;
					}

					if (logger)
						logger->Printf(IGpLogDriver::Category_Information, "Triggering GL context reset due to window size change");

					m_contextLost = true;
					continue;
				}
			}

			if (m_contextLost)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Information, "Resetting OpenGL context.  Physical: %i x %i   Virtual %i x %i", static_cast<int>(m_windowWidthPhysical), static_cast<int>(m_windowHeightPhysical), static_cast<int>(m_windowWidthVirtual), static_cast<int>(m_windowHeightVirtual));

				// Drop everything and reset
				m_res = InstancedResources();

				if (m_firstSurface)
					m_firstSurface->DestroyAll();

				if (!InitResources(m_windowWidthPhysical, m_windowHeightPhysical, m_windowWidthVirtual, m_windowHeightVirtual))
				{
					if (logger)
						logger->Printf(IGpLogDriver::Category_Information, "Terminating display driver due to InitResources failing");

					break;
				}

				if (m_firstSurface)
					m_firstSurface->RecreateAll();

				m_contextLost = false;
				continue;
			}

			bool wantTextInput = m_properties.m_systemServices->IsTextInputEnabled();
			if (wantTextInput != m_textInputEnabled)
			{
				m_textInputEnabled = wantTextInput;
				if (m_textInputEnabled)
					SDL_StartTextInput();
				else
					SDL_StopTextInput();
			}

			// Handle dismissal of on-screen keyboard
			const bool isTextInputActuallyActive = SDL_IsTextInputActive();
			m_textInputEnabled = isTextInputActuallyActive;
			m_properties.m_systemServices->SetTextInputEnabled(isTextInputActuallyActive);

			if (SyncRender())
			{
				ticks--;
				if (ticks <= 0)
					break;
			}
		}
	}
}

void GpDisplayDriver_SDL_GL2::ForceSync()
{
	m_frameTimeAccumulated = std::chrono::nanoseconds::zero();
}

static void PostMouseEvent(IGpVOSEventQueue *eventQueue, GpMouseEventType_t eventType, GpMouseButton_t button, int32_t x, int32_t y, float pixelScaleX, float pixelScaleY)
{
	if (GpVOSEvent *evt = eventQueue->QueueEvent())
	{
		evt->m_eventType = GpVOSEventTypes::kMouseInput;

		GpMouseInputEvent &mEvent = evt->m_event.m_mouseInputEvent;
		mEvent.m_button = button;
		mEvent.m_x = x;
		mEvent.m_y = y;
		mEvent.m_eventType = eventType;

		if (pixelScaleX != 1.0f)
			mEvent.m_x = static_cast<int32_t>(static_cast<float>(x) / pixelScaleX);

		if (pixelScaleY != 1.0f)
			mEvent.m_y = static_cast<int32_t>(static_cast<float>(y) / pixelScaleX);
	}
}

static void PostTouchEvent(IGpVOSEventQueue *eventQueue, GpTouchEventType_t eventType, int32_t x, int32_t y, int64_t deviceID, int64_t fingerID)
{
	if (GpVOSEvent *evt = eventQueue->QueueEvent())
	{
		evt->m_eventType = GpVOSEventTypes::kTouchInput;

		GpTouchInputEvent &tEvent = evt->m_event.m_touchInputEvent;
		tEvent.m_deviceID = deviceID;
		tEvent.m_fingerID = fingerID;
		tEvent.m_x = x;
		tEvent.m_y = y;
		tEvent.m_eventType = eventType;
	}
}

static bool IdentifyVKey(const SDL_KeyboardEvent *keyEvt, GpKeyIDSubset_t &outSubset, GpKeyboardInputEvent::KeyUnion &outKey)
{
	SDL_Keycode keyCode = static_cast<SDL_Keycode>(keyEvt->keysym.sym);

	switch (keyCode)
	{
	case SDLK_ESCAPE:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kEscape;
		break;
	case SDLK_PRINTSCREEN:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kPrintScreen;
		break;
	case SDLK_SCROLLLOCK:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kScrollLock;
		break;
	case SDLK_PAUSE:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kPause;
		break;
	case SDLK_INSERT:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kInsert;
		break;
	case SDLK_HOME:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kHome;
		break;
	case SDLK_PAGEUP:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kPageUp;
		break;
	case SDLK_PAGEDOWN:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kPageDown;
		break;
	case SDLK_DELETE:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kDelete;
		break;
	case SDLK_TAB:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kTab;
		break;
	case SDLK_END:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kEnd;
		break;
	case SDLK_BACKSPACE:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kBackspace;
		break;
	case SDLK_CAPSLOCK:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kCapsLock;
		break;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kEnter;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		{
			if (keyCode == SDLK_LSHIFT)
			{
				outSubset = GpKeyIDSubsets::kSpecial;
				outKey.m_specialKey = GpKeySpecials::kLeftShift;
			}
			else if (keyCode == SDLK_RSHIFT)
			{
				outSubset = GpKeyIDSubsets::kSpecial;
				outKey.m_specialKey = GpKeySpecials::kRightShift;
			}
			else
				return false;
		}
		break;
	case SDLK_LGUI:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kLeftCommand;
		break;
	case SDLK_RGUI:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kRightCommand;
		break;
	case SDLK_LCTRL:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kLeftCtrl;
		break;
	case SDLK_RCTRL:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kRightCtrl;
		break;
	case SDLK_LALT:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kLeftAlt;
		break;
	case SDLK_RALT:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kRightAlt;
		break;
	case SDLK_NUMLOCKCLEAR:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kNumLock;
		break;
	case SDLK_KP_0:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 0;
		break;
	case SDLK_KP_1:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 1;
		break;
	case SDLK_KP_2:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 2;
		break;
	case SDLK_KP_3:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 3;
		break;
	case SDLK_KP_4:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 4;
		break;
	case SDLK_KP_5:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 5;
		break;
	case SDLK_KP_6:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 6;
		break;
	case SDLK_KP_7:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 7;
		break;
	case SDLK_KP_8:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 8;
		break;
	case SDLK_KP_9:
		outSubset = GpKeyIDSubsets::kNumPadNumber;
		outKey.m_numPadNumber = 9;
		break;

	case SDLK_F1:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 1;
		break;
	case SDLK_F2:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 2;
		break;
	case SDLK_F3:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 3;
		break;
	case SDLK_F4:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 4;
		break;
	case SDLK_F5:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 5;
		break;
	case SDLK_F6:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 6;
		break;
	case SDLK_F7:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 7;
		break;
	case SDLK_F8:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 8;
		break;
	case SDLK_F9:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 9;
		break;
	case SDLK_F10:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 10;
		break;
	case SDLK_F11:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 11;
		break;
	case SDLK_F12:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 12;
		break;
	case SDLK_F13:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 13;
		break;
	case SDLK_F14:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 14;
		break;
	case SDLK_F15:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 15;
		break;
	case SDLK_F16:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 16;
		break;
	case SDLK_F17:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 17;
		break;
	case SDLK_F18:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 18;
		break;
	case SDLK_F19:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 19;
		break;
	case SDLK_F20:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 20;
		break;
	case SDLK_F21:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 21;
		break;
	case SDLK_F22:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 22;
		break;
	case SDLK_F23:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 23;
		break;
	case SDLK_F24:
		outSubset = GpKeyIDSubsets::kFKey;
		outKey.m_fKey = 24;
		break;

	case SDLK_COMMA:
		outSubset = GpKeyIDSubsets::kASCII;
		outKey.m_asciiChar = ',';
		break;

	case SDLK_MINUS:
		outSubset = GpKeyIDSubsets::kASCII;
		outKey.m_asciiChar = '-';
		break;

	case SDLK_UP:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kUpArrow;
		break;
	case SDLK_DOWN:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kDownArrow;
		break;
	case SDLK_LEFT:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kLeftArrow;
		break;
	case SDLK_RIGHT:
		outSubset = GpKeyIDSubsets::kSpecial;
		outKey.m_specialKey = GpKeySpecials::kRightArrow;
		break;

	case SDLK_KP_COMMA:
		outSubset = GpKeyIDSubsets::kNumPadSpecial;
		outKey.m_numPadSpecialKey = GpNumPadSpecials::kComma;
		break;

	case SDLK_KP_MULTIPLY:
		outSubset = GpKeyIDSubsets::kNumPadSpecial;
		outKey.m_numPadSpecialKey = GpNumPadSpecials::kAsterisk;
		break;

	case SDLK_KP_PERIOD:
		outSubset = GpKeyIDSubsets::kNumPadSpecial;
		outKey.m_numPadSpecialKey = GpNumPadSpecials::kPeriod;
		break;

	case SDLK_KP_DIVIDE:
		outSubset = GpKeyIDSubsets::kNumPadSpecial;
		outKey.m_numPadSpecialKey = GpNumPadSpecials::kSlash;
		break;

	default:
		{
			if (keyCode < 128)
			{
				outSubset = GpKeyIDSubsets::kASCII;
				if (keyCode >= 'a' && keyCode <= 'z')
					outKey.m_asciiChar = static_cast<char>(keyCode + 'A' - 'a');
				else
					outKey.m_asciiChar = static_cast<char>(keyCode);
				break;
			}
		}
		return false;
	}

	return true;
}

static void PostKeyboardEvent(IGpVOSEventQueue *eventQueue, GpKeyboardInputEventType_t eventType, GpKeyIDSubset_t subset, const GpKeyboardInputEvent::KeyUnion &key, uint32_t repeatCount)
{
	if (GpVOSEvent *evt = eventQueue->QueueEvent())
	{
		evt->m_eventType = GpVOSEventTypes::kKeyboardInput;

		GpKeyboardInputEvent &mEvent = evt->m_event.m_keyboardInputEvent;
		mEvent.m_key = key;
		mEvent.m_eventType = eventType;
		mEvent.m_keyIDSubset = subset;
		mEvent.m_repeatCount = repeatCount;
	}
}

void GpDisplayDriver_SDL_GL2::TranslateSDLMessage(const SDL_Event *msg, IGpVOSEventQueue *eventQueue, float pixelScaleX, float pixelScaleY, bool obstructiveTextInput)
{
	switch (msg->type)
	{
	case SDL_MOUSEMOTION:
		{
			const SDL_MouseMotionEvent *mouseEvt = reinterpret_cast<const SDL_MouseMotionEvent *>(msg);
			PostMouseEvent(eventQueue, GpMouseEventTypes::kMove, GpMouseButtons::kNone, mouseEvt->x, mouseEvt->y, pixelScaleX, pixelScaleY);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		{
			const SDL_MouseButtonEvent *mouseEvt = reinterpret_cast<const SDL_MouseButtonEvent *>(msg);
			GpMouseEventType_t evtType = GpMouseEventTypes::kDown;
			GpMouseButton_t mouseButton = GpMouseButtons::kLeft;

			if (mouseEvt->type == SDL_MOUSEBUTTONDOWN)
				evtType = GpMouseEventTypes::kDown;
			else if (mouseEvt->type == SDL_MOUSEBUTTONUP)
				evtType = GpMouseEventTypes::kUp;
			else
				break;

			if (mouseEvt->button == SDL_BUTTON_LEFT)
				mouseButton = GpMouseButtons::kLeft;
			else if (mouseEvt->button == SDL_BUTTON_RIGHT)
				mouseButton = GpMouseButtons::kRight;
			else if (mouseEvt->button == SDL_BUTTON_MIDDLE)
				mouseButton = GpMouseButtons::kMiddle;
			else if (mouseEvt->button == SDL_BUTTON_X1)
				mouseButton = GpMouseButtons::kX1;
			else if (mouseEvt->button == SDL_BUTTON_X2)
				mouseButton = GpMouseButtons::kX2;
			else
				break;

			PostMouseEvent(eventQueue, evtType, mouseButton, mouseEvt->x, mouseEvt->y, pixelScaleX, pixelScaleY);
		}
		break;
	case SDL_FINGERUP:
	case SDL_FINGERDOWN:
	case SDL_FINGERMOTION:
		{
			const SDL_TouchFingerEvent *fingerEvt = reinterpret_cast<const SDL_TouchFingerEvent *>(msg);
			GpTouchEventType_t evtType = GpTouchEventTypes::kDown;

			if (fingerEvt->type == SDL_FINGERUP)
				evtType = GpTouchEventTypes::kUp;
			else if (fingerEvt->type == SDL_FINGERDOWN)
				evtType = GpTouchEventTypes::kDown;
			else if (fingerEvt->type == SDL_FINGERMOTION)
				evtType = GpTouchEventTypes::kMove;
			else
				break;

			float unnormalizedX = static_cast<float>(m_windowWidthVirtual) * fingerEvt->x;
			float unnormalizedY = static_cast<float>(m_windowHeightVirtual) * fingerEvt->y;

			PostTouchEvent(eventQueue, evtType, static_cast<int32_t>(unnormalizedX), static_cast<int32_t>(unnormalizedY), fingerEvt->touchId, fingerEvt->fingerId);
		}
		break;
	case SDL_KEYDOWN:
		{
			const SDL_KeyboardEvent *keyEvt = reinterpret_cast<const SDL_KeyboardEvent *>(msg);

			GpKeyIDSubset_t subset;
			GpKeyboardInputEvent::KeyUnion key;
			bool isRepeat = (keyEvt->repeat != 0);
			const GpKeyboardInputEventType_t keyEventType = isRepeat ? GpKeyboardInputEventTypes::kAuto : GpKeyboardInputEventTypes::kDown;
			if (IdentifyVKey(keyEvt, subset, key))
			{
				PostKeyboardEvent(eventQueue, keyEventType, subset, key, keyEvt->repeat + 1);
				if (subset == GpKeyIDSubsets::kSpecial && key.m_specialKey == GpKeySpecials::kEnter)
				{
					const GpKeyboardInputEventType_t charEventType = isRepeat ? GpKeyboardInputEventTypes::kAutoChar : GpKeyboardInputEventTypes::kDownChar;

					GpKeyboardInputEvent::KeyUnion crKey;
					crKey.m_asciiChar = '\n';
					PostKeyboardEvent(eventQueue, charEventType, GpKeyIDSubsets::kASCII, crKey, keyEvt->repeat + 1);
				}
			}
		}
		break;
	case SDL_KEYUP:
		{
			const SDL_KeyboardEvent *keyEvt = reinterpret_cast<const SDL_KeyboardEvent *>(msg);

			GpKeyIDSubset_t subset;
			GpKeyboardInputEvent::KeyUnion key;
			if (IdentifyVKey(keyEvt, subset, key))
				PostKeyboardEvent(eventQueue, GpKeyboardInputEventTypes::kUp, subset, key, keyEvt->repeat + 1);
		}
		break;
	case SDL_TEXTINPUT:
		{
			// SDL doesn't report if the text input event is a repeat, which sucks...
			const SDL_TextInputEvent *teEvt = reinterpret_cast<const SDL_TextInputEvent *>(msg);

			size_t lenUTF8 = strlen(teEvt->text);

			size_t parseOffset = 0;
			while (parseOffset < lenUTF8)
			{
				uint32_t codePoint;
				size_t numDigested;
				GpUnicode::UTF8::Decode(reinterpret_cast<const uint8_t*>(teEvt->text) + parseOffset, lenUTF8 - parseOffset, numDigested, codePoint);

				parseOffset += numDigested;

				const GpKeyboardInputEventType_t keyEventType = GpKeyboardInputEventTypes::kDownChar;
				GpKeyboardInputEvent::KeyUnion key;
				GpKeyIDSubset_t subset = GpKeyIDSubsets::kASCII;
				if (codePoint <= 128)
					key.m_asciiChar = static_cast<char>(codePoint);
				else
				{
					subset = GpKeyIDSubsets::kUnicode;
					key.m_unicodeChar = static_cast<uint32_t>(codePoint);
				}
				PostKeyboardEvent(eventQueue, keyEventType, subset, key, 1);
			}

			if (!obstructiveTextInput)
			{
				SDL_StopTextInput();
				SDL_StartTextInput();
			}
		}
		break;
	case SDL_QUIT:
		{
			if (GpVOSEvent *evt = eventQueue->QueueEvent())
				evt->m_eventType = GpVOSEventTypes::kQuit;
		}
		break;
	default:
		break;
	}
}

void GpDisplayDriver_SDL_GL2::Shutdown()
{
	this->~GpDisplayDriver_SDL_GL2();
	free(this);
}

void GpDisplayDriver_SDL_GL2::GetInitialDisplayResolution(unsigned int *width, unsigned int *height)
{
	if (width)
		*width = m_initialWidthVirtual;

	if (height)
		*height = m_initialHeightVirtual;
}

IGpDisplayDriverSurface *GpDisplayDriver_SDL_GL2::CreateSurface(size_t width, size_t height, size_t pitch, GpPixelFormat_t pixelFormat, SurfaceInvalidateCallback_t invalidateCallback, void *invalidateContext)
{
	GpDisplayDriverSurface_GL2 *surface = GpDisplayDriverSurface_GL2::Create(this, width, height, pitch, pixelFormat, m_lastSurface, invalidateCallback, invalidateContext);
	if (surface)
	{
		m_lastSurface = surface;
		if (m_firstSurface == nullptr)
			m_firstSurface = surface;
	}
	return surface;
}

void GpDisplayDriver_SDL_GL2::DrawSurface(IGpDisplayDriverSurface *surface, int32_t x, int32_t y, size_t width, size_t height, const GpDisplayDriverSurfaceEffects *effects)
{
	if (!effects)
		effects = &gs_defaultEffects;

	GpGLVertexArray *vaPtr = m_res.m_quadVertexArray;
	size_t vbStride = sizeof(float) * 2;
	size_t zero = 0;

	GpDisplayDriverSurface_GL2 *glSurface = static_cast<GpDisplayDriverSurface_GL2*>(surface);
	GpPixelFormat_t pixelFormat = glSurface->GetPixelFormat();

	DrawQuadProgram *program = nullptr;

	if (pixelFormat == GpPixelFormats::k8BitStandard || pixelFormat == GpPixelFormats::k8BitCustom)
	{
		if (m_useICCProfile)
		{
			if (effects->m_flicker)
				program = &m_res.m_drawQuadPaletteICCFlickerProgram;
			else
				program = &m_res.m_drawQuadPaletteICCNoFlickerProgram;
		}
		else
		{
			if (effects->m_flicker)
				program = &m_res.m_drawQuadPaletteFlickerProgram;
			else
				program = &m_res.m_drawQuadPaletteNoFlickerProgram;
		}
	}
	else if (pixelFormat == GpPixelFormats::kRGB555)
	{
		return;
	}
	else if (pixelFormat == GpPixelFormats::kRGB32)
	{
		if (m_useICCProfile)
		{
			if (effects->m_flicker)
				program = &m_res.m_drawQuad32ICCFlickerProgram;
			else
				program = &m_res.m_drawQuad32ICCNoFlickerProgram;
		}
		else
		{
			if (effects->m_flicker)
				program = &m_res.m_drawQuad32FlickerProgram;
			else
				program = &m_res.m_drawQuad32NoFlickerProgram;
		}
	}
	else
	{
		return;
	}

	CheckGLError(m_gl, m_properties.m_logger);

	m_gl.UseProgram(program->m_program->GetID());

	CheckGLError(m_gl, m_properties.m_logger);

	{
		const float twoDivWidth = 2.0f / static_cast<float>(m_windowWidthVirtual);
		const float negativeTwoDivHeight = -2.0f / static_cast<float>(m_windowHeightVirtual);

		GLfloat ndcOriginAndDimensions[4] =
		{

			static_cast<GLfloat>(x) * twoDivWidth - 1.0f,
			static_cast<GLfloat>(y) * negativeTwoDivHeight + 1.0f,
			static_cast<GLfloat>(width) * twoDivWidth,
			static_cast<GLfloat>(height) * negativeTwoDivHeight,
		};

		GLfloat surfaceDimensions_TextureRegion[4] =
		{
			static_cast<GLfloat>(glSurface->GetImageWidth()),
			static_cast<GLfloat>(glSurface->GetHeight()),
			static_cast<GLfloat>(static_cast<float>(glSurface->GetImageWidth()) / static_cast<float>(glSurface->GetPaddedTextureWidth())),
			1.f
		};

		m_gl.Uniform4fv(program->m_vertexNDCOriginAndDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(ndcOriginAndDimensions));
		m_gl.Uniform4fv(program->m_vertexSurfaceDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(surfaceDimensions_TextureRegion));

		GLfloat modulation[4] = { 1.f, 1.f, 1.f, 1.f };
		GLfloat flickerAxis[2] = { 0.f, 0.f };
		GLfloat flickerStart = -1.f;
		GLfloat flickerEnd = -2.f;

		if (effects->m_flicker)
		{
			flickerAxis[0] = effects->m_flickerAxisX;
			flickerAxis[1] = effects->m_flickerAxisY;
			flickerStart = effects->m_flickerStartThreshold;
			flickerEnd = effects->m_flickerEndThreshold;
		}

		float desaturation = effects->m_desaturation;

		if (effects->m_darken)
			for (int i = 0; i < 3; i++)
				modulation[i] = 0.5f;

		m_gl.Uniform4fv(program->m_pixelModulationLocation, 1, modulation);
		m_gl.Uniform2fv(program->m_pixelFlickerAxisLocation, 1, flickerAxis);
		m_gl.Uniform1fv(program->m_pixelFlickerStartThresholdLocation, 1, &flickerStart);
		m_gl.Uniform1fv(program->m_pixelFlickerEndThresholdLocation, 1, &flickerEnd);
		m_gl.Uniform1fv(program->m_pixelDesaturationLocation, 1, &desaturation);
	}

	GLint vpos[1] = { program->m_vertexPosUVLocation };

	m_res.m_quadVertexArray->Activate(vpos);

	m_gl.ActiveTexture(GL_TEXTURE0);
	m_gl.BindTexture(GL_TEXTURE_2D, glSurface->GetTexture()->GetID());
	m_gl.Uniform1i(program->m_pixelSurfaceTextureLocation, 0);

	if (pixelFormat == GpPixelFormats::k8BitStandard || pixelFormat == GpPixelFormats::k8BitCustom)
	{
		m_gl.ActiveTexture(GL_TEXTURE1);
		m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_paletteTexture->GetID());
		m_gl.Uniform1i(program->m_pixelPaletteTextureLocation, 1);
	}

	m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_res.m_quadIndexBuffer->GetID());
	m_gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckGLError(m_gl, m_properties.m_logger);

	if (pixelFormat == GpPixelFormats::k8BitStandard || pixelFormat == GpPixelFormats::k8BitCustom)
	{
		m_gl.ActiveTexture(GL_TEXTURE1);
		m_gl.BindTexture(GL_TEXTURE_2D, 0);
	}

	m_gl.ActiveTexture(GL_TEXTURE0);
	m_gl.BindTexture(GL_TEXTURE_2D, 0);

	m_res.m_quadVertexArray->Deactivate(vpos);

	m_gl.UseProgram(0);

	CheckGLError(m_gl, m_properties.m_logger);
}


IGpCursor *GpDisplayDriver_SDL_GL2::CreateBWCursor(size_t width, size_t height, const void *pixelData, const void *maskData, size_t hotSpotX, size_t hotSpotY)
{
	SDL_Cursor *cursor = SDL_CreateCursor(static_cast<const Uint8*>(pixelData), static_cast<const Uint8*>(maskData), width, height, hotSpotX, hotSpotY);
	return new GpCursor_SDL2(cursor);
}

IGpCursor *GpDisplayDriver_SDL_GL2::CreateColorCursor(size_t width, size_t height, const void *pixelDataRGBA, size_t hotSpotX, size_t hotSpotY)
{
	uint32_t channelMasks[4];

	for (int i = 0; i < 4; i++)
	{
		channelMasks[i] = 0;
		reinterpret_cast<uint8_t*>(&channelMasks[i])[i] = 0xff;
	}

	SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, channelMasks[0], channelMasks[1], channelMasks[2], channelMasks[3]);
	if (!surface)
		return nullptr;

	size_t surfacePitch = surface->pitch;
	uint8_t *destPixels = reinterpret_cast<uint8_t*>(surface->pixels);
	for (size_t y = 0; y < height; y++)
		memcpy(destPixels + y * surfacePitch, static_cast<const uint8_t*>(pixelDataRGBA) + y * width * 4, width * 4);

	SDL_Cursor *cursor = SDL_CreateColorCursor(surface, hotSpotX, hotSpotY);
	SDL_FreeSurface(surface);

	if (!cursor)
		return nullptr;

	return new GpCursor_SDL2(cursor);
}

void GpDisplayDriver_SDL_GL2::SetCursor(IGpCursor *cursor)
{
	GpCursor_SDL2 *sdlCursor = static_cast<GpCursor_SDL2*>(cursor);

	sdlCursor->IncRef();

	if (m_pendingCursor)
		m_pendingCursor->DecRef();

	m_pendingCursor = sdlCursor;
}

void GpDisplayDriver_SDL_GL2::SetStandardCursor(EGpStandardCursor_t standardCursor)
{
	if (m_pendingCursor)
	{
		m_pendingCursor->DecRef();
		m_pendingCursor = nullptr;
	}

	m_pendingStandardCursor = standardCursor;
}

void GpDisplayDriver_SDL_GL2::UpdatePalette(const void *paletteData)
{
	memcpy(m_paletteData, paletteData, 256 * 4);

	GLenum internalFormat = SupportsSizedFormats() ? GL_RGBA8 : GL_RGBA;

	m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_paletteTexture->GetID());
	m_gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	m_gl.TexImage2D(GL_TEXTURE_2D, 0, internalFormat, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_paletteData);
	m_gl.BindTexture(GL_TEXTURE_2D, 0);
}

void GpDisplayDriver_SDL_GL2::SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint8_t rgba[4] = { r, g, b, a };
	for (int i = 0; i < 4; i++)
		m_bgColor[i] = static_cast<float>(rgba[i]) / 255.0f;
}

void GpDisplayDriver_SDL_GL2::SetBackgroundDarkenEffect(bool isDark)
{
	m_bgIsDark = isDark;
}

void GpDisplayDriver_SDL_GL2::SetUseICCProfile(bool useICCProfile)
{
	m_useICCProfile = useICCProfile;
}

void GpDisplayDriver_SDL_GL2::RequestToggleFullScreen(uint32_t timestamp)
{
	// Alt-Enter gets re-sent after a full-screen toggle, so we ignore toggle requests until half a second has elapsed
	if (timestamp == 0 || timestamp > m_lastFullScreenToggleTimeStamp + 30)
	{
		m_isFullScreenDesired = !m_isFullScreenDesired;
		m_lastFullScreenToggleTimeStamp = timestamp;
	}
}

void GpDisplayDriver_SDL_GL2::RequestResetVirtualResolution()
{
	m_isResolutionResetDesired = true;
}

bool GpDisplayDriver_SDL_GL2::IsFullScreen() const
{
	return m_isFullScreenDesired;
}

const GpDisplayDriverProperties &GpDisplayDriver_SDL_GL2::GetProperties() const
{
	return m_properties;
}

IGpPrefsHandler *GpDisplayDriver_SDL_GL2::GetPrefsHandler() const
{
	return const_cast<GpDisplayDriver_SDL_GL2*>(this);
}

bool GpDisplayDriver_SDL_GL2::SupportsSizedFormats() const
{
#if GP_GL_IS_OPENGL_4_CONTEXT
	return true;
#else
	return false;
#endif
}

void GpDisplayDriver_SDL_GL2::ApplyPrefs(const void *identifier, size_t identifierSize, const void *contents, size_t contentsSize, uint32_t version)
{
	if (version == kPrefsVersion && identifierSize == strlen(kPrefsIdentifier) && !memcmp(identifier, kPrefsIdentifier, identifierSize))
	{
		const GpDisplayDriver_SDL_GL2_Prefs *prefs = static_cast<const GpDisplayDriver_SDL_GL2_Prefs *>(contents);
		m_isFullScreenDesired = prefs->m_isFullScreen;
	}
}

bool GpDisplayDriver_SDL_GL2::SavePrefs(void *context, IGpPrefsHandler::WritePrefsFunc_t writeFunc)
{
	GpDisplayDriver_SDL_GL2_Prefs prefs;
	prefs.m_isFullScreen = m_isFullScreenDesired;

	return writeFunc(context, kPrefsIdentifier, strlen(kPrefsIdentifier), &prefs, sizeof(prefs), kPrefsVersion);
}

void GpDisplayDriver_SDL_GL2::UnlinkSurface(GpDisplayDriverSurface_GL2 *surface, GpDisplayDriverSurface_GL2 *prev, GpDisplayDriverSurface_GL2 *next)
{
	if (m_lastSurface == surface)
		m_lastSurface = prev;

	if (m_firstSurface == surface)
		m_firstSurface = next;
}

const GpGLFunctions *GpDisplayDriver_SDL_GL2::GetGLFunctions() const
{
	return &m_gl;
}



template<GLuint TShaderType>
GpComPtr<GpGLShader<TShaderType>> GpDisplayDriver_SDL_GL2::CreateShader(const char *shaderSrc)
{
	size_t shaderCodeLength = strlen(shaderSrc);
	GpComPtr<GpGLShader<TShaderType>> shader(GpGLShader<TShaderType>::Create(this));
	if (shader == nullptr)
		return shader;

	const GLchar *shaderSrcGL = reinterpret_cast<const GLchar*>(shaderSrc);
	GLint shaderLenGL = static_cast<GLint>(shaderCodeLength);
	m_gl.ShaderSource(shader->GetID(), 1, &shaderSrcGL, &shaderLenGL);
	m_gl.CompileShader(shader->GetID());

	GLint compiled = 0;
	m_gl.GetShaderiv(shader->GetID(), GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint infoLength = 0;
		m_gl.GetShaderiv(shader->GetID(), GL_INFO_LOG_LENGTH, &infoLength);

		std::vector<char> log;
		log.resize(infoLength + 1);
		log[infoLength] = '\0';

		m_gl.GetShaderInfoLog(shader->GetID(), static_cast<GLsizei>(log.size() - 1), nullptr, reinterpret_cast<GLchar*>(&log[0]));

		const char *errorMsg = &log[0];

		return GpComPtr<GpGLShader<TShaderType>>();
	}

	return shader;
}

void GpDisplayDriver_SDL_GL2::StartOpenGLForWindow(IGpLogDriver *logger)
{
	SDL_GLContext context = SDL_GL_CreateContext(m_window);
	SDL_GL_SetSwapInterval(1);
}

bool GpDisplayDriver_SDL_GL2::InitResources(uint32_t physicalWidth, uint32_t physicalHeight, uint32_t virtualWidth, uint32_t virtualHeight)
{
	IGpLogDriver *logger = m_properties.m_logger;

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "GpDisplayDriver_SDL_GL2::InitResources");

	CheckGLError(m_gl, logger);

	if (!InitBackBuffer(virtualWidth, virtualHeight))
		return false;

	// Quad index buffer
	{
		const uint16_t indexBufferData[] = { 0, 1, 2, 1, 3, 2 };

		m_res.m_quadIndexBuffer = GpGLBuffer::Create(this);
		if (!m_res.m_quadIndexBuffer)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitResources: CreateBuffer for draw quad index buffer failed");

			return false;
		}

		m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_res.m_quadIndexBuffer->GetID());
		m_gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexBufferData), indexBufferData, GL_STATIC_DRAW);
		m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	// Quad vertex buffer
	{
		const float vertexBufferData[] =
		{
			0.f, 0.f,
			1.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
		};

		m_res.m_quadVertexBufferKeepalive = GpGLBuffer::Create(this);
		if (!m_res.m_quadVertexBufferKeepalive)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitResources: GpGLBuffer::Create for draw quad vertex buffer failed");

			return false;
		}

		m_gl.BindBuffer(GL_ARRAY_BUFFER, m_res.m_quadVertexBufferKeepalive->GetID());
		m_gl.BufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);
		m_gl.BindBuffer(GL_ARRAY_BUFFER, 0);

		m_res.m_quadVertexArray = GpGLVertexArray::Create(this);
		if (!m_res.m_quadVertexBufferKeepalive)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitResources: GpGLVertexArray::Create for draw quad vertex buffer failed");

			return false;
		}

		GpGLVertexArraySpec specs[] =
		{
			m_res.m_quadVertexBufferKeepalive,
			0,					// index
			2,					// size
			GL_FLOAT,			// type
			GL_FALSE,			// normalized
			sizeof(float) * 2,	// stride
			0
		};

		if (!m_res.m_quadVertexArray->InitWithSpecs(specs, sizeof(specs) / sizeof(specs[0])))
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitResources: InitWithSpecs for draw quad vertex buffer failed");

			return false;
		}
	}

	GpComPtr<GpGLShader<GL_VERTEX_SHADER>> drawQuadVertexShader = CreateShader<GL_VERTEX_SHADER>(GpBinarizedShaders::g_drawQuadV_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuadPaletteFlickerPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuadPalettePF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuadPaletteNoFlickerPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuadPalettePNF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuad32FlickerPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuad32PF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuad32NoFlickerPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuad32PNF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuadPaletteICCFPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuadPaletteICCPF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuadPaletteICCNFPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuadPaletteICCPNF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuad32ICCFPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuad32ICCPF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> drawQuad32ICCNFPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_drawQuad32ICCPNF_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> scaleQuadPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_scaleQuadP_GL2);
	GpComPtr<GpGLShader<GL_FRAGMENT_SHADER>> copyQuadPixelShader = CreateShader<GL_FRAGMENT_SHADER>(GpBinarizedShaders::g_copyQuadP_GL2);

		
	if (!m_res.m_drawQuadPaletteFlickerProgram.Link(this, drawQuadVertexShader, drawQuadPaletteFlickerPixelShader) ||
		!m_res.m_drawQuadPaletteNoFlickerProgram.Link(this, drawQuadVertexShader, drawQuadPaletteNoFlickerPixelShader)
		|| !m_res.m_drawQuad32FlickerProgram.Link(this, drawQuadVertexShader, drawQuad32FlickerPixelShader)
		|| !m_res.m_drawQuad32NoFlickerProgram.Link(this, drawQuadVertexShader, drawQuad32NoFlickerPixelShader)
		|| !m_res.m_drawQuadPaletteICCFlickerProgram.Link(this, drawQuadVertexShader, drawQuadPaletteICCFPixelShader)
		|| !m_res.m_drawQuadPaletteICCNoFlickerProgram.Link(this, drawQuadVertexShader, drawQuadPaletteICCNFPixelShader)
		|| !m_res.m_drawQuad32ICCFlickerProgram.Link(this, drawQuadVertexShader, drawQuad32ICCFPixelShader)
		|| !m_res.m_drawQuad32ICCNoFlickerProgram.Link(this, drawQuadVertexShader, drawQuad32ICCNFPixelShader)

//		|| !m_drawQuadRGBICCProgram.Link(this, drawQuadVertexShader, drawQuadRGBICCPixelShader)
//		|| !m_drawQuad15BitICCProgram.Link(this, drawQuadVertexShader, drawQuad15BitICCPixelShader)
		|| !m_res.m_scaleQuadProgram.Link(this, drawQuadVertexShader, scaleQuadPixelShader)
		|| !m_res.m_copyQuadProgram.Link(this, drawQuadVertexShader, copyQuadPixelShader))
		return false;

	// Palette texture
	{
		m_res.m_paletteTexture = GpGLTexture::Create(this);
		if (!m_res.m_paletteTexture)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitResources: GpGLTexture::Create failed");

			return false;
		}

		GLenum internalFormat = SupportsSizedFormats() ? GL_RGBA8 : GL_RGBA;

		m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_paletteTexture->GetID());
		m_gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
		m_gl.TexImage2D(GL_TEXTURE_2D, 0, internalFormat, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_paletteData);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_gl.BindTexture(GL_TEXTURE_2D, 0);
	}

	return true;
}

void GpDisplayDriver_SDL_GL2::BecomeFullScreen()
{
	SDL_GetWindowPosition(m_window, &m_windowModeRevertX, &m_windowModeRevertY);
	SDL_GetWindowSize(m_window, &m_windowModeRevertWidth, &m_windowModeRevertHeight);
	SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	m_isFullScreen = true;
}

void GpDisplayDriver_SDL_GL2::BecomeWindowed()
{
	SDL_SetWindowFullscreen(m_window, 0);
	SDL_SetWindowPosition(m_window, m_windowModeRevertX, m_windowModeRevertY);
	SDL_SetWindowSize(m_window, m_windowModeRevertWidth, m_windowModeRevertHeight);

	m_isFullScreen = false;
}

void GpDisplayDriver_SDL_GL2::SynchronizeCursors()
{
	if (m_activeCursor)
	{
		if (m_pendingCursor != m_activeCursor)
		{
			if (m_pendingCursor == nullptr)
			{
				m_currentStandardCursor = m_pendingStandardCursor;
				ChangeToStandardCursor(m_currentStandardCursor);

				m_activeCursor->DecRef();
				m_activeCursor = nullptr;
			}
			else
			{
				ChangeToCursor(m_pendingCursor->GetCursor());

				m_pendingCursor->IncRef();
				m_activeCursor->DecRef();
				m_activeCursor = m_pendingCursor;
			}
		}
	}
	else
	{
		if (m_pendingCursor)
		{
			m_pendingCursor->IncRef();
			m_activeCursor = m_pendingCursor;

			ChangeToCursor(m_activeCursor->GetCursor());
		}
		else
		{
			if (m_pendingStandardCursor != m_currentStandardCursor)
			{
				ChangeToStandardCursor(m_pendingStandardCursor);
				m_currentStandardCursor = m_pendingStandardCursor;
			}
		}
	}
}

void GpDisplayDriver_SDL_GL2::ChangeToCursor(SDL_Cursor *cursor)
{
	if (cursor == nullptr)
	{
		if (!m_cursorIsHidden)
		{
			m_cursorIsHidden = true;
			SDL_ShowCursor(0);
		}
	}
	else
	{
		if (m_cursorIsHidden)
		{
			m_cursorIsHidden = false;
			SDL_ShowCursor(1);
		}
		SDL_SetCursor(cursor);
	}
}

void GpDisplayDriver_SDL_GL2::ChangeToStandardCursor(EGpStandardCursor_t cursor)
{
	switch (cursor)
	{
	case EGpStandardCursors::kArrow:
		SDL_SetCursor(m_arrowCursor);
		break;
	case EGpStandardCursors::kHidden:
		SDL_SetCursor(nullptr);
		break;
	case EGpStandardCursors::kIBeam:
		SDL_SetCursor(m_iBeamCursor);
		break;
	case EGpStandardCursors::kWait:
		SDL_SetCursor(m_waitCursor);
		break;
	default:
		break;
	}
}


bool GpDisplayDriver_SDL_GL2::ResizeOpenGLWindow(uint32_t &windowWidth, uint32_t &windowHeight, uint32_t desiredWidth, uint32_t desiredHeight, IGpLogDriver *logger)
{
	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "ResizeOpenGLWindow: %i x %i", static_cast<int>(desiredWidth), static_cast<int>(desiredHeight));

	if (desiredWidth > 32768)
		desiredWidth = 32768;

	if (desiredHeight > 32768)
		desiredHeight = 32768;

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "ResizeOpenGLWindow: Adjusted dimensions: %i x %i", static_cast<int>(desiredWidth), static_cast<int>(desiredHeight));

	SDL_SetWindowSize(m_window, desiredWidth, desiredHeight);

	windowWidth = desiredWidth;
	windowHeight = desiredHeight;

	return true;
}

bool GpDisplayDriver_SDL_GL2::InitBackBuffer(uint32_t width, uint32_t height)
{
	IGpLogDriver *logger = m_properties.m_logger;

	if (logger)
		logger->Printf(IGpLogDriver::Category_Information, "GpDisplayDriver_SDL_GL2::InitBackBuffer: %i x %i", static_cast<int>(width), static_cast<int>(height));

	{
		m_res.m_virtualScreenTexture = GpGLTexture::Create(this);
		if (!m_res.m_virtualScreenTexture)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: GpGLTexture::Create for virtual screen texture failed");

			return false;
		}

		GLenum internalFormat = SupportsSizedFormats() ? GL_RGBA8 : GL_RGBA;

		m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_virtualScreenTexture->GetID());
		m_gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		m_gl.TexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_gl.BindTexture(GL_TEXTURE_2D, 0);

		CheckGLError(m_gl, logger);
	}

	{
		m_res.m_virtualScreenTextureRTV = GpGLRenderTargetView::Create(this);

		if (!m_res.m_virtualScreenTextureRTV)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: GpGLRenderTargetView::Create for virtual screen texture failed");

			return false;
		}

		m_gl.BindFramebuffer(GL_FRAMEBUFFER, m_res.m_virtualScreenTextureRTV->GetID());
		m_gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_res.m_virtualScreenTexture->GetID(), 0);
		GLenum status = m_gl.CheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			if (logger)
				logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: Framebuffer complete check failed, status was %i   VST ID is %i", static_cast<int>(status), static_cast<int>(m_res.m_virtualScreenTextureRTV->GetID()));

			return false;
		}

		m_gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	m_useUpscaleFilter = (m_pixelScaleX == 1.5f || m_pixelScaleX == 2.5f || m_pixelScaleY == 1.5f || m_pixelScaleY == 2.5f);

	if (m_useUpscaleFilter)
	{
		uint32_t upscaleX = ceil(m_pixelScaleX);
		uint32_t upscaleY = ceil(m_pixelScaleY);

		{
			m_res.m_upscaleTexture = GpGLTexture::Create(this);
			if (!m_res.m_upscaleTexture)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: GpGLTexture::Create for upscale texture failed");

				return false;
			}

			m_upscaleTextureWidth = width * upscaleX;
			m_upscaleTextureHeight = height * upscaleY;

			GLenum internalFormat = SupportsSizedFormats() ? GL_RGBA8 : GL_RGBA;

			m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_upscaleTexture->GetID());
			m_gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
			m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			m_gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			m_gl.TexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_upscaleTextureWidth, m_upscaleTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_gl.BindTexture(GL_TEXTURE_2D, 0);

			CheckGLError(m_gl, logger);
		}

		{
			m_res.m_upscaleTextureRTV = GpGLRenderTargetView::Create(this);

			if (!m_res.m_upscaleTextureRTV)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: GpGLRenderTargetView::Create for upscale texture failed");

				return false;
			}

			m_gl.BindFramebuffer(GL_FRAMEBUFFER, m_res.m_upscaleTextureRTV->GetID());
			m_gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_res.m_upscaleTexture->GetID(), 0);
			GLenum status = m_gl.CheckFramebufferStatus(GL_FRAMEBUFFER);

			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				if (logger)
					logger->Printf(IGpLogDriver::Category_Error, "GpDisplayDriver_SDL_GL2::InitBackBuffer: Framebuffer complete check failed for upscale texture, status was %i   VST ID is %i", static_cast<int>(status), static_cast<int>(m_res.m_virtualScreenTextureRTV->GetID()));

				return false;
			}

			m_gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	CheckGLError(m_gl, logger);

	return true;
}


void GpDisplayDriver_SDL_GL2::ScaleVirtualScreen()
{
	if (m_useUpscaleFilter)
	{
		m_gl.BindFramebuffer(GL_FRAMEBUFFER, m_res.m_upscaleTextureRTV->GetID());

		m_gl.Viewport(0, 0, m_upscaleTextureWidth, m_upscaleTextureHeight);

		const BlitQuadProgram &program = m_res.m_scaleQuadProgram;

		float ndcOriginsAndDimensions[4] =
		{
			-1.0f,
			-1.0f,
			2.0f,
			2.0f,
		};

		float surfaceDimensions_TextureRegion[4] =
		{
			static_cast<float>(m_windowWidthVirtual),
			static_cast<float>(m_windowHeightVirtual),
			1.f,
			1.f
		};

		m_gl.UseProgram(program.m_program->GetID());
		m_gl.Uniform4fv(program.m_vertexNDCOriginAndDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(ndcOriginsAndDimensions));
		m_gl.Uniform4fv(program.m_vertexSurfaceDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(surfaceDimensions_TextureRegion));

		float dxdy_dimensions[4] =
		{
			static_cast<float>(1.0 / m_windowWidthVirtual),
			static_cast<float>(1.0 / m_windowHeightVirtual),
			static_cast<float>(m_windowWidthVirtual),
			static_cast<float>(m_windowHeightVirtual)
		};

		m_gl.Uniform4fv(program.m_pixelDXDYDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(dxdy_dimensions));

		GLint attribLocations[] = { program.m_vertexPosUVLocation };

		m_res.m_quadVertexArray->Activate(attribLocations);

		m_gl.ActiveTexture(GL_TEXTURE0 + 0);
		m_gl.BindTexture(GL_TEXTURE_2D, m_res.m_virtualScreenTexture->GetID());
		m_gl.Uniform1i(program.m_pixelSurfaceTextureLocation, 0);

		m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_res.m_quadIndexBuffer->GetID());
		m_gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_gl.UseProgram(0);

		m_gl.ActiveTexture(GL_TEXTURE0 + 0);
		m_gl.BindTexture(GL_TEXTURE_2D, 0);

		m_res.m_quadVertexArray->Deactivate(attribLocations);

		m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	m_gl.BindFramebuffer(GL_FRAMEBUFFER, 0);

	m_gl.Viewport(0, 0, m_windowWidthPhysical, m_windowHeightPhysical);

	const BlitQuadProgram &program = m_res.m_copyQuadProgram;

	{
		const float twoDivWidth = 2.0f / static_cast<float>(m_windowWidthPhysical);
		const float twoDivHeight = 2.0f / static_cast<float>(m_windowHeightPhysical);

		// Use the scaled virtual width instead of the physical width to correctly handle cases where the window boundary is in the middle of a pixel
		float fWidth = static_cast<float>(m_windowWidthVirtual) * m_pixelScaleX;
		float fHeight = static_cast<float>(m_windowHeightVirtual) * m_pixelScaleY;

		float ndcOriginsAndDimensions[4] =
		{
			-1.0f,
			-1.0f,
			2.0f,
			2.0f,
		};

		float surfaceDimensions_TextureRegion[4] =
		{
			static_cast<float>(m_windowWidthVirtual),
			static_cast<float>(m_windowHeightVirtual),
			1.f,
			1.f
		};

		m_gl.UseProgram(program.m_program->GetID());
		m_gl.Uniform4fv(program.m_vertexNDCOriginAndDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(ndcOriginsAndDimensions));
		m_gl.Uniform4fv(program.m_vertexSurfaceDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(surfaceDimensions_TextureRegion));

		float dxdy_dimensions[4] =
		{
			static_cast<float>(1.0 / m_windowWidthVirtual),
			static_cast<float>(1.0 / m_windowHeightVirtual),
			static_cast<float>(m_windowWidthVirtual),
			static_cast<float>(m_windowHeightVirtual)
		};

		m_gl.Uniform4fv(program.m_pixelDXDYDimensionsLocation, 1, reinterpret_cast<const GLfloat*>(dxdy_dimensions));
	}

	GLint attribLocations[] = { program.m_vertexPosUVLocation };

	m_res.m_quadVertexArray->Activate(attribLocations);


	GpGLTexture *inputTexture = m_useUpscaleFilter ? static_cast<GpGLTexture*>(m_res.m_upscaleTexture) : static_cast<GpGLTexture*>(m_res.m_virtualScreenTexture);

	m_gl.ActiveTexture(GL_TEXTURE0 + 0);
	m_gl.BindTexture(GL_TEXTURE_2D, inputTexture->GetID());
	m_gl.Uniform1i(program.m_pixelSurfaceTextureLocation, 0);

	m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_res.m_quadIndexBuffer->GetID());
	m_gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

	m_gl.UseProgram(0);

	m_gl.ActiveTexture(GL_TEXTURE0 + 0);
	m_gl.BindTexture(GL_TEXTURE_2D, 0);

	m_res.m_quadVertexArray->Deactivate(attribLocations);

	m_gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


bool GpDisplayDriver_SDL_GL2::DrawQuadProgram::Link(GpDisplayDriver_SDL_GL2 *driver, const GpGLShader<GL_VERTEX_SHADER> *vertexShader, const GpGLShader<GL_FRAGMENT_SHADER> *pixelShader)
{
	m_program = GpGLProgram::Create(driver);
	if (!m_program)
		return false;

	const GpGLFunctions *gl = driver->GetGLFunctions();
	gl->AttachShader(m_program->GetID(), vertexShader->GetID());
	gl->AttachShader(m_program->GetID(), pixelShader->GetID());
	gl->LinkProgram(m_program->GetID());

	GLint linked = 0;
	gl->GetProgramiv(m_program->GetID(), GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint logLength = 0;
		gl->GetProgramiv(m_program->GetID(), GL_INFO_LOG_LENGTH, &logLength);

		std::vector<char> errorMsgBuffer;
		errorMsgBuffer.resize(static_cast<size_t>(logLength) + 1);
		errorMsgBuffer[logLength] = '\0';

		gl->GetProgramInfoLog(m_program->GetID(), static_cast<size_t>(logLength), nullptr, reinterpret_cast<GLchar*>(&errorMsgBuffer[0]));
		const char *errorMsg = &errorMsgBuffer[0];

		return false;
	}

	m_vertexNDCOriginAndDimensionsLocation = gl->GetUniformLocation(m_program->GetID(), "ndcOriginAndDimensions");
	m_vertexSurfaceDimensionsLocation = gl->GetUniformLocation(m_program->GetID(), "surfaceDimensions_TextureRegion");
	m_vertexPosUVLocation = gl->GetAttribLocation(m_program->GetID(), "posUV");

	m_pixelModulationLocation = gl->GetUniformLocation(m_program->GetID(), "constants_Modulation");
	m_pixelFlickerAxisLocation = gl->GetUniformLocation(m_program->GetID(), "constants_FlickerAxis");
	m_pixelFlickerStartThresholdLocation = gl->GetUniformLocation(m_program->GetID(), "constants_FlickerStartThreshold");
	m_pixelFlickerEndThresholdLocation = gl->GetUniformLocation(m_program->GetID(), "constants_FlickerEndThreshold");
	m_pixelDesaturationLocation = gl->GetUniformLocation(m_program->GetID(), "constants_Desaturation");

	m_pixelSurfaceTextureLocation = gl->GetUniformLocation(m_program->GetID(), "surfaceTexture");
	m_pixelPaletteTextureLocation = gl->GetUniformLocation(m_program->GetID(), "paletteTexture");

	return true;
}

bool GpDisplayDriver_SDL_GL2::BlitQuadProgram::Link(GpDisplayDriver_SDL_GL2 *driver, const GpGLShader<GL_VERTEX_SHADER> *vertexShader, const GpGLShader<GL_FRAGMENT_SHADER> *pixelShader)
{
	m_program = GpGLProgram::Create(driver);
	if (!m_program)
		return false;

	const GpGLFunctions *gl = driver->GetGLFunctions();
	gl->AttachShader(m_program->GetID(), vertexShader->GetID());
	gl->AttachShader(m_program->GetID(), pixelShader->GetID());
	gl->LinkProgram(m_program->GetID());

	GLint linked = 0;
	gl->GetProgramiv(m_program->GetID(), GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint logLength = 0;
		gl->GetProgramiv(m_program->GetID(), GL_INFO_LOG_LENGTH, &logLength);

		std::vector<char> errorMsgBuffer;
		errorMsgBuffer.resize(static_cast<size_t>(logLength) + 1);
		errorMsgBuffer[logLength] = '\0';

		gl->GetProgramInfoLog(m_program->GetID(), static_cast<size_t>(logLength), nullptr, reinterpret_cast<GLchar*>(&errorMsgBuffer[0]));
		const char *errorMsg = &errorMsgBuffer[0];

		return false;
	}

	m_vertexNDCOriginAndDimensionsLocation = gl->GetUniformLocation(m_program->GetID(), "ndcOriginAndDimensions");
	m_vertexSurfaceDimensionsLocation = gl->GetUniformLocation(m_program->GetID(), "surfaceDimensions_TextureRegion");
	m_pixelDXDYDimensionsLocation = gl->GetUniformLocation(m_program->GetID(), "dxdy_dimensions");
	m_vertexPosUVLocation = gl->GetAttribLocation(m_program->GetID(), "posUV");
	m_pixelSurfaceTextureLocation = gl->GetUniformLocation(m_program->GetID(), "surfaceTexture");

	return true;
}

bool GpDisplayDriver_SDL_GL2::SyncRender()
{
	if (m_frameTimeAccumulated >= m_frameTimeSliceSize)
	{
		m_frameTimeAccumulated -= m_frameTimeSliceSize;

		return true;
	}

	SynchronizeCursors();

	float bgColor[4];

	for (int i = 0; i < 4; i++)
		bgColor[i] = m_bgColor[i];

	if (m_bgIsDark)
	{
		for (int i = 0; i < 3; i++)
			bgColor[i] *= 0.25f;
	}

	//ID3D11RenderTargetView *const rtv = m_backBufferRTV;
	GpGLRenderTargetView *const vsRTV = m_res.m_virtualScreenTextureRTV;

	m_gl.BindFramebuffer(GL_FRAMEBUFFER, vsRTV->GetID());

	m_gl.Viewport(0, 0, m_windowWidthVirtual, m_windowHeightVirtual);

	m_gl.ClearColor(m_bgColor[0], m_bgColor[1], m_bgColor[2], m_bgColor[3]);
	m_gl.Clear(GL_COLOR_BUFFER_BIT);

	m_properties.m_renderFunc(m_properties.m_renderFuncContext);

	ScaleVirtualScreen();

	CheckGLError(m_gl, m_properties.m_logger);

	SDL_GL_SwapWindow(m_window);

#ifdef __EMSCRIPTEN__
	emscripten_sleep(1);
#endif

	std::chrono::time_point<std::chrono::high_resolution_clock>::duration syncTime = std::chrono::high_resolution_clock::now().time_since_epoch();
	const intmax_t periodNum = std::chrono::high_resolution_clock::period::num;
	const intmax_t periodDen = std::chrono::high_resolution_clock::period::den;

	if (syncTime.count() != 0)
	{
		if (m_syncTimeBase.count() == 0)
			m_syncTimeBase = syncTime;

		std::chrono::time_point<std::chrono::high_resolution_clock>::duration timestamp;
		timestamp = syncTime - m_syncTimeBase;

		bool compacted = false;
		if (m_presentHistory.Size() > 0)
		{
			CompactedPresentHistoryItem &lastItem = m_presentHistory[m_presentHistory.Size() - 1];
			std::chrono::time_point<std::chrono::high_resolution_clock>::duration timeDelta = timestamp - lastItem.m_timestamp;

			if (timeDelta.count() < 0)
				timeDelta = std::chrono::time_point<std::chrono::high_resolution_clock>::duration::zero();	// This should never happen

			if (timeDelta.count() * static_cast<intmax_t>(m_properties.m_frameTimeLockDenominator) * periodNum < periodDen * static_cast<intmax_t>(m_properties.m_frameTimeLockNumerator))
			{
				lastItem.m_numFrames++;
				compacted = true;
			}
		}

		if (!compacted)
		{
			if (m_presentHistory.Size() == m_presentHistory.CAPACITY)
				m_presentHistory.RemoveFromStart();

			CompactedPresentHistoryItem *newItem = m_presentHistory.Append();
			newItem->m_timestamp = timestamp;
			newItem->m_numFrames = 1;
		}
	}

	if (m_presentHistory.Size() >= 2)
	{
		const size_t presentHistorySizeMinusOne = m_presentHistory.Size() - 1;
		unsigned int numFrames = 0;
		for (size_t i = 0; i < presentHistorySizeMinusOne; i++)
			numFrames += m_presentHistory[i].m_numFrames;

		std::chrono::high_resolution_clock::duration timeFrame = m_presentHistory[presentHistorySizeMinusOne].m_timestamp - m_presentHistory[0].m_timestamp;

		unsigned int cancelledFrames = 0;
		std::chrono::high_resolution_clock::duration cancelledTime = std::chrono::high_resolution_clock::duration::zero();

		const int overshootTolerance = 2;

		for (size_t i = 0; i < presentHistorySizeMinusOne; i++)
		{
			std::chrono::high_resolution_clock::duration blockTimeframe = m_presentHistory[i + 1].m_timestamp - m_presentHistory[i].m_timestamp;
			unsigned int blockNumFrames = m_presentHistory[i].m_numFrames;

			if (blockTimeframe.count() * static_cast<intmax_t>(numFrames) >= timeFrame.count() * static_cast<intmax_t>(blockNumFrames) * overshootTolerance)
			{
				cancelledTime += blockTimeframe;
				cancelledFrames += blockNumFrames;
			}
		}

		numFrames -= cancelledFrames;
		timeFrame -= cancelledTime;

		// timeFrame / numFrames = Frame timestep
		// Unless Frame timestep is within the frame lock range, a.k.a.
		// timeFrame / numFrames / qpFreq >= minFrameTimeNum / minFrameTimeDenom

		bool isInFrameTimeLock = false;
		if (timeFrame.count() * static_cast<intmax_t>(m_properties.m_frameTimeLockMinDenominator) * periodNum >= static_cast<intmax_t>(numFrames) * static_cast<intmax_t>(m_properties.m_frameTimeLockMinNumerator) * periodDen
			&& timeFrame.count() * static_cast<intmax_t>(m_properties.m_frameTimeLockMaxDenominator) * periodNum <= static_cast<intmax_t>(numFrames) * static_cast<intmax_t>(m_properties.m_frameTimeLockMaxNumerator) * periodDen)
		{
			isInFrameTimeLock = true;
		}

		std::chrono::high_resolution_clock::duration frameTimeStep = m_frameTimeSliceSize;
		if (!isInFrameTimeLock)
		{
			const int MAX_FRAMES_PER_STEP = 4;

			frameTimeStep = std::chrono::high_resolution_clock::duration(timeFrame.count() / numFrames);
			if (frameTimeStep > m_frameTimeSliceSize * MAX_FRAMES_PER_STEP)
				frameTimeStep = m_frameTimeSliceSize * MAX_FRAMES_PER_STEP;
		}

		m_frameTimeAccumulated += frameTimeStep;
	}

	return false;
}

IGpDisplayDriver *GpDriver_CreateDisplayDriver_SDL_GL2(const GpDisplayDriverProperties &properties)
{
	GpDisplayDriver_SDL_GL2 *driver = static_cast<GpDisplayDriver_SDL_GL2*>(malloc(sizeof(GpDisplayDriver_SDL_GL2)));
	if (!driver)
		return nullptr;

	return new (driver) GpDisplayDriver_SDL_GL2(properties);
}

template<class T>
T *GpGLObjectImpl<T>::Create(GpDisplayDriver_SDL_GL2 *driver)
{
	T *obj = static_cast<T*>(malloc(sizeof(T)));
	if (!obj)
		return nullptr;

	new (obj) T();

	obj->InitDriver(driver, driver->GetGLFunctions());

	if (!obj->Init())
	{
		obj->Destroy();
		return nullptr;
	}

	return obj;
}

#if GP_ASYNCIFY_PARANOID
bool IGpDisplayDriver::Init()
{
	return static_cast<GpDisplayDriver_SDL_GL2*>(this)->Init();
}

void IGpDisplayDriver::ServeTicks(int tickCount)
{
	static_cast<GpDisplayDriver_SDL_GL2*>(this)->ServeTicks(tickCount);
}

void IGpDisplayDriver::Shutdown()
{
	static_cast<GpDisplayDriver_SDL_GL2*>(this)->Shutdown();
}

IGpCursor *IGpDisplayDriver::CreateBWCursor(size_t width, size_t height, const void *pixelData, const void *maskData, size_t hotSpotX, size_t hotSpotY)
{
	return static_cast<GpDisplayDriver_SDL_GL2*>(this)->CreateBWCursor(width, height, pixelData, maskData, hotSpotX, hotSpotY);
}

IGpCursor *IGpDisplayDriver::CreateColorCursor(size_t width, size_t height, const void *pixelDataRGBA, size_t hotSpotX, size_t hotSpotY)
{
	return static_cast<GpDisplayDriver_SDL_GL2*>(this)->CreateColorCursor(width, height, pixelDataRGBA, hotSpotX, hotSpotY);
}

#endif


#pragma pop_macro("LoadCursor")
