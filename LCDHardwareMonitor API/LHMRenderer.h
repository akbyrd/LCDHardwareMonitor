#ifndef LHM_RENDERER
#define LHM_RENDERER

using Mesh         = List<struct MeshData>::RefT;
using VertexShader = List<struct VertexShaderData>::RefT;
using PixelShader  = List<struct PixelShaderData>::RefT;
using RenderTarget = List<struct RenderTargetData>::RefT;
using DepthBuffer  = List<struct DepthBufferData>::RefT;

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

// TODO: WTF?
using Index = u32;

namespace StandardRenderTarget
{
	static const RenderTarget Null = { 1 };
	static const RenderTarget Main = { 2 };
};

namespace StandardDepthBuffer
{
	static const DepthBuffer Null = { 1 };
	static const DepthBuffer Main = { 2 };
};

namespace StandardMesh
{
	static const Mesh Null       = { 1 };
	static const Mesh Triangle   = { 2 };
	static const Mesh Quad       = { 3 };
	static const Mesh Cube       = { 4 };
	static const Mesh Fullscreen = { 5 };
};

namespace StandardVertexShader
{
	static const VertexShader Null      = { 1 };
	static const VertexShader WVP       = { 2 };
	static const VertexShader ClipSpace = { 3 };
};

namespace StandardPixelShader
{
	static const PixelShader Null             = { 1 };
	static const PixelShader SolidColored     = { 2 };
	static const PixelShader VertexColored    = { 3 };
	static const PixelShader DebugCoordinates = { 4 };
	static const PixelShader Composite        = { 5 };
};

#endif
