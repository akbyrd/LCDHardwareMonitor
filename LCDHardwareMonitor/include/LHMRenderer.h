#ifndef LHM_RENDERER
#define LHM_RENDERER

struct MeshData;
struct VertexShaderData;
struct PixelShaderData;
struct RenderTargetData;
struct DepthBufferData;

using Mesh         = typename List<MeshData>::RefT;
using VertexShader = typename List<VertexShaderData>::RefT;
using PixelShader  = typename List<PixelShaderData>::RefT;
using RenderTarget = typename List<RenderTargetData>::RefT;
using DepthBuffer  = typename List<DepthBufferData>::RefT;

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

using Index = u32;

namespace StandardRenderTarget
{
	constexpr RenderTarget Null = { 1 };
	constexpr RenderTarget Main = { 2 };
};

namespace StandardDepthBuffer
{
	constexpr DepthBuffer Null = { 1 };
	constexpr DepthBuffer Main = { 2 };
};

namespace StandardMesh
{
	constexpr Mesh Null       = { 1 };
	constexpr Mesh Triangle   = { 2 };
	constexpr Mesh Quad       = { 3 };
	constexpr Mesh Cube       = { 4 };
	constexpr Mesh Fullscreen = { 5 };
};

namespace StandardVertexShader
{
	constexpr VertexShader Null      = { 1 };
	constexpr VertexShader WVP       = { 2 };
	constexpr VertexShader ClipSpace = { 3 };
};

namespace StandardPixelShader
{
	constexpr PixelShader Null             = { 1 };
	constexpr PixelShader SolidColored     = { 2 };
	constexpr PixelShader VertexColored    = { 3 };
	constexpr PixelShader DebugCoordinates = { 4 };
	constexpr PixelShader Composite        = { 5 };
};

#endif
