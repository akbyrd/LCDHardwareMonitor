struct RendererState;
struct VertexShader;

enum struct VertexAttributeSemantic
{
	None,
	Position,
	Color,
	Count
};

enum struct VertexAttributeFormat
{
	None,
	Float3,
	Float4,
	Count
};

struct VertexAttribute
{
	VertexAttributeSemantic semantic;
	VertexAttributeFormat   format;
};

struct ConstantBufferDesc
{
	//b32   dirty;
	u32   size;
	//void* data;
};

// TODO: Mesh struct, pre-set pointers
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

//using MeshRef = List<struct Mesh>::RefT;
struct DrawCall
{
	Mesh          mesh         = Mesh::Null;
	VertexShader* vs           = nullptr;
	r32           worldM[4][4] = {};
};

b32       Renderer_Initialize   (RendererState*, V2i renderSize);
void      Renderer_Teardown     (RendererState*);
DrawCall* Renderer_PushDrawCall (RendererState*);
b32       Renderer_Render       (RendererState*);
