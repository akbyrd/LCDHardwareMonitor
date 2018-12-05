#ifndef LHM_RENDERER
#define LHM_RENDERER

using Mesh         = List<struct MeshData>::RefT;
using VertexShader = List<struct VertexShaderData>::RefT;
using PixelShader  = List<struct PixelShaderData>::RefT;

struct DrawCall
{
	Mesh         mesh;
	Matrix       world;

	VertexShader vs;
	void*        cBufPerObjDataVS;

	PixelShader  ps;
	void*        cBufPerObjDataPS;
};

namespace StandardMesh
{
	static const Mesh Null     = { 0 };
	static const Mesh Triangle = { 1 };
	static const Mesh Quad     = { 2 };
};

enum struct ConstantBufferFrequency
{
	Null,
	PerFrame,
	PerObject,
};

// TODO: Dirty flag?
struct ConstantBufferDesc
{
	u32                     size;
	void*                   data;
	ConstantBufferFrequency frequency;

	static ConstantBufferDesc Null;
};

namespace StandardVertexShader
{
	static const VertexShader Null  = { 0 };
	static const VertexShader Debug = { 1 };
};

namespace StandardPixelShader
{
	static const PixelShader Null  = { 0 };
	static const PixelShader Debug = { 1 };
};

#endif
