#ifndef LHM_RENDERER
#define LHM_RENDERER

using Mesh         = List<struct MeshData>::RefT;
using VertexShader = List<struct VertexShaderData>::RefT;
using PixelShader  = List<struct PixelShaderData>::RefT;

struct DrawCall
{
	Mesh         mesh;
	VertexShader vs;
	PixelShader  ps;
	r32          worldM[4][4];
};

namespace StandardMesh
{
	static const Mesh Quad     = { 0 };
	//static const Mesh Null     = { 0 };
	//static const Mesh Triangle = { 1 };
	//static const Mesh Quad     = { 2 };
	//static const Mesh Sphere   = { 3 };
	//static const Mesh Cube     = { 4 };
	//static const Mesh Cylinder = { 5 };
	//static const Mesh Grid     = { 6 };
};

namespace StandardVertexShader
{
	static const VertexShader Debug = { 0 };
	//static const VertexShader Null  = { 0 };
	//static const VertexShader Debug = { 1 };
};

namespace StandardPixelShader
{
	static const PixelShader Debug = { 0 };
	//static const PixelShader Null  = { 0 };
	//static const PixelShader Debug = { 1 };
};

#endif
