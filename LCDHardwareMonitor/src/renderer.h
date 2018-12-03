struct RendererState;

enum struct VertexAttributeSemantic
{
	Null,
	Position,
	Color,
	TexCoord,
	Count
};

enum struct VertexAttributeFormat
{
	Null,
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

b32          Renderer_Initialize       (RendererState*, v2i renderSize);
void         Renderer_Teardown         (RendererState*);
VertexShader Renderer_LoadVertexShader (RendererState*, Slice<c8> name, c8* path, Slice<VertexAttribute> attributes, Slice<ConstantBufferDesc> cBufDescs);
PixelShader  Renderer_LoadPixelShader  (RendererState*, Slice<c8> name, c8* path, Slice<ConstantBufferDesc> cBufDescs);
Matrix*      Renderer_GetWVPPointer    (RendererState*);
DrawCall*    Renderer_PushDrawCall     (RendererState*);
b32          Renderer_Render           (RendererState*);
