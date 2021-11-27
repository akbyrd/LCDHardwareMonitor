#ifndef LHM_RENDERER
#define LHM_RENDERER

struct Mesh         : Index {};
struct VertexShader : Index {};
struct PixelShader  : Index {};
struct RenderTarget : Index {};
struct DepthBuffer  : Index {};

// TODO: Re-order structs in this file
enum struct ShaderStage
{
	Null,
	Vertex,
	Pixel,
};

struct Vertex
{
	v3 position;
	v4 color;
	v2 uv;
};

using MIndex = u32;

namespace StandardRenderTarget
{
	static const RenderTarget Null = {{ 0 }};
	static const RenderTarget Main = {{ 1 }};
};

namespace StandardDepthBuffer
{
	static const DepthBuffer Null = {{ 0 }};
	static const DepthBuffer Main = {{ 1 }};
};

namespace StandardMesh
{
	static const Mesh Null       = {{ 0 }};
	static const Mesh Triangle   = {{ 1 }};
	static const Mesh Quad       = {{ 2 }};
	static const Mesh Cube       = {{ 3 }};
	static const Mesh Fullscreen = {{ 4 }};
};

namespace StandardVertexShader
{
	static const VertexShader Null      = {{ 0 }};
	static const VertexShader WVP       = {{ 1 }};
	static const VertexShader ClipSpace = {{ 2 }};
};

namespace StandardPixelShader
{
	static const PixelShader Null             = {{ 0 }};
	static const PixelShader SolidColored     = {{ 1 }};
	static const PixelShader VertexColored    = {{ 2 }};
	static const PixelShader DebugCoordinates = {{ 3 }};
	static const PixelShader Composite        = {{ 4 }};
};

#endif
