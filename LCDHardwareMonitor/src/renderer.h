namespace LCDHardwareMonitor {

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
	v2,
	v3,
	v4,
	Count
};

struct VertexAttribute
{
	VertexAttributeSemantic semantic;
	VertexAttributeFormat   format;
};

// TODO: Move these back to public API?
struct ConstantBufferUpdate
{
	Material    material;
	ShaderStage shaderStage;
	u32         index;
	void*       data;
};

struct DrawCall
{
	Material material;
};

b8                    Renderer_Initialize                   (RendererState&, v2u renderSize);
b8                    Renderer_RebuildSharedGeometryBuffers (RendererState&);
void                  Renderer_Teardown                     (RendererState&);
Mesh                  Renderer_CreateMesh                   (RendererState&, StringView name, Slice<Vertex> vertices, Slice<Index> indices);
VertexShader          Renderer_LoadVertexShader             (RendererState&, StringView name, StringView path, Slice<VertexAttribute> attributes, Slice<u32> cBufSizes);
PixelShader           Renderer_LoadPixelShader              (RendererState&, StringView name, StringView path, Slice<u32> cBufSizes);
// TODO: Combine with CreateMesh, LoadVertexShader, and LoadPixelShader
Material              Renderer_CreateMaterial               (RendererState&, Mesh mesh, VertexShader vs, PixelShader ps);
ConstantBufferUpdate* Renderer_PushConstantBufferUpdate     (RendererState&);
DrawCall*             Renderer_PushDrawCall                 (RendererState&);
b8                    Renderer_Render                       (RendererState&);
void*                 Renderer_GetSharedRenderSurface       (RendererState&);

}
