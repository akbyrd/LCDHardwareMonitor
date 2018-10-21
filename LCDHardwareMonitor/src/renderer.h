struct RendererState;

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

b32          Renderer_Initialize      (RendererState*, v2i renderSize);
void         Renderer_Teardown        (RendererState*);
VertexShader Renderer_LoadVertexShader(RendererState*, c8* path, List<VertexAttribute> attributes, ConstantBufferDesc cBufDesc);
PixelShader  Renderer_LoadPixelShader (RendererState*, c8* path);
DrawCall*    Renderer_PushDrawCall    (RendererState*);
b32          Renderer_Render          (RendererState*);
