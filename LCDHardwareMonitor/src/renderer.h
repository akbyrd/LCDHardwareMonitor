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
struct VSConstantBufferUpdate
{
	VertexShader vs;
	u32          index;
	void*        data;
};

struct PSConstantBufferUpdate
{
	PixelShader ps;
	u32         index;
	void*       data;
};

using CPUTexture = List<struct CPUTextureData>::RefT;

struct CPUTextureBytes
{
	ByteSlice bytes;
	v2u size;
	u32 pixelStride;
	u32 rowStride;
};

b8              Renderer_Initialize                     (RendererState&, v2u renderSize);
void            Renderer_Teardown                       (RendererState&);
b8              Renderer_Render                         (RendererState&);

b8              Renderer_RebuildSharedGeometryBuffers   (RendererState&);
Mesh            Renderer_CreateMesh                     (RendererState&, StringView name, Slice<Vertex> vertices, Slice<Index> indices);
VertexShader    Renderer_LoadVertexShader               (RendererState&, StringView name, StringView path, Slice<VertexAttribute> attributes, Slice<u32> cBufSizes);
PixelShader     Renderer_LoadPixelShader                (RendererState&, StringView name, StringView path, Slice<u32> cBufSizes);
RenderTarget    Renderer_CreateRenderTarget             (RendererState&, StringView name, b8 resource);
RenderTarget    Renderer_CreateSharedRenderTarget       (RendererState&, StringView name, b8 resource);
CPUTexture      Renderer_CreateCPUTexture               (RendererState&, StringView name);
DepthBuffer     Renderer_CreateDepthBuffer              (RendererState&, StringView name, b8 resource);

void            Renderer_PushRenderTarget               (RendererState&, RenderTarget);
void            Renderer_PopRenderTarget                (RendererState&);
void            Renderer_PushDepthBuffer                (RendererState&, DepthBuffer);
void            Renderer_PopDepthBuffer                 (RendererState&);
void            Renderer_PushVertexShader               (RendererState&, VertexShader);
void            Renderer_PopVertexShader                (RendererState&);
void            Renderer_PushPixelShader                (RendererState&, PixelShader);
void            Renderer_PopPixelShader                 (RendererState&);
void            Renderer_PushPixelShaderResource        (RendererState&, RenderTarget, u32 slot);
void            Renderer_PushPixelShaderResource        (RendererState&, DepthBuffer, u32 slot);
void            Renderer_PopPixelShaderResource         (RendererState&);
void            Renderer_UpdateVSConstantBuffer         (RendererState&, VSConstantBufferUpdate&);
void            Renderer_UpdatePSConstantBuffer         (RendererState&, PSConstantBufferUpdate&);
void            Renderer_DrawMesh                       (RendererState&, Mesh);
void            Renderer_Copy                           (RendererState&, RenderTarget, CPUTexture);

CPUTextureBytes Renderer_GetCPUTextureBytes             (RendererState&, CPUTexture);
size            Renderer_GetSharedRenderTargetHandle    (RendererState&, RenderTarget);

void            Renderer_ValidateMesh                   (RendererState&, Mesh);
void            Renderer_ValidateVertexShader           (RendererState&, VertexShader);
void            Renderer_ValidatePixelShader            (RendererState&, PixelShader);
void            Renderer_ValidateVSConstantBufferUpdate (RendererState&, VSConstantBufferUpdate&);
void            Renderer_ValidatePSConstantBufferUpdate (RendererState&, PSConstantBufferUpdate&);
