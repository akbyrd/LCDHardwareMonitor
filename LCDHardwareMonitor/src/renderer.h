struct RendererState;

enum Mesh
{
	Null,
	Quad,
	//Sphere,
	//Cube,
	//Cylinder,
	//Grid,

	Count
};

struct DrawCall
{
	Mesh mesh         = Mesh::Null;
	r32  worldM[4][4] = {};
};

DrawCall* PushDrawCall(RendererState*);