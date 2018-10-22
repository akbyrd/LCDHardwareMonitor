struct RendererState;

enum struct VertexAttributeSemantic
{
	None,
	Position,
	Color,
	TexCoord,
	Count
};

enum struct VertexAttributeFormat
{
	None,
	Float2,
	Float3,
	Float4,
	Count
};

struct VertexAttribute
{
	VertexAttributeSemantic semantic;
	VertexAttributeFormat   format;
};

b32          Renderer_Initialize      (RendererState*, v2i renderSize);
void         Renderer_Teardown        (RendererState*);
VertexShader Renderer_LoadVertexShader(RendererState*, c8* path, List<VertexAttribute> attributes, ConstantBufferDesc cBufDesc);
PixelShader  Renderer_LoadPixelShader (RendererState*, c8* path, ConstantBufferDesc cBufDesc);
void*        Renderer_GetWVPPointer   (RendererState*);
DrawCall*    Renderer_PushDrawCall    (RendererState*);
b32          Renderer_Render          (RendererState*);
