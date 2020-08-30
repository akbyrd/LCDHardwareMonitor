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

b8              Renderer_Initialize                     (RendererState&);
void            Renderer_Teardown                       (RendererState&);
b8              Renderer_Render                         (RendererState&);

void            Renderer_SetRenderSize                  (RendererState&, v2u renderSize);
b8              Renderer_FinalizeResourceCreation       (RendererState&);
Mesh            Renderer_CreateMesh                     (RendererState&, StringView name, Slice<Vertex> vertices, Slice<Index> indices);
VertexShader    Renderer_LoadVertexShader               (RendererState&, StringView name, StringView path, Slice<VertexAttribute> attributes, Slice<u32> cBufSizes);
PixelShader     Renderer_LoadPixelShader                (RendererState&, StringView name, StringView path, Slice<u32> cBufSizes);
RenderTarget    Renderer_CreateRenderTarget             (RendererState&, StringView name, b8 resource);
RenderTarget    Renderer_CreateSharedRenderTarget       (RendererState&, StringView name, b8 resource);
CPUTexture      Renderer_CreateCPUTexture               (RendererState&, StringView name);
DepthBuffer     Renderer_CreateDepthBuffer              (RendererState&, StringView name, b8 resource);

void            Renderer_PushRenderTarget               (RendererState&, RenderTarget);
void            Renderer_PopRenderTarget                (RendererState&);
void            Renderer_ClearRenderTarget              (RendererState&, v4 color);
void            Renderer_PushDepthBuffer                (RendererState&, DepthBuffer);
void            Renderer_PopDepthBuffer                 (RendererState&);
void            Renderer_ClearDepthBuffer               (RendererState&);
void            Renderer_PushVertexShader               (RendererState&, VertexShader);
void            Renderer_PopVertexShader                (RendererState&);
void            Renderer_PushPixelShader                (RendererState&, PixelShader);
void            Renderer_PopPixelShader                 (RendererState&);
void            Renderer_PushPixelShaderResource        (RendererState&, RenderTarget, u32 slot);
void            Renderer_PushPixelShaderResource        (RendererState&, DepthBuffer, u32 slot);
// TODO: I don't like having to specify the slot here...
void            Renderer_PopPixelShaderResource         (RendererState&, u32 slot);
void            Renderer_UpdateVSConstantBuffer         (RendererState&, VSConstantBufferUpdate&);
void            Renderer_UpdatePSConstantBuffer         (RendererState&, PSConstantBufferUpdate&);
void            Renderer_DrawMesh                       (RendererState&, Mesh);
void            Renderer_Copy                           (RendererState&, RenderTarget, CPUTexture);

CPUTextureBytes Renderer_GetCPUTextureBytes             (RendererState&, CPUTexture);
size            Renderer_GetSharedRenderTargetHandle    (RendererState&, RenderTarget);

b8              Renderer_ValidateMesh                   (RendererState&, Mesh);
b8              Renderer_ValidateVertexShader           (RendererState&, VertexShader);
b8              Renderer_ValidatePixelShader            (RendererState&, PixelShader);
b8              Renderer_ValidateVSConstantBufferUpdate (RendererState&, VSConstantBufferUpdate&);
b8              Renderer_ValidatePSConstantBufferUpdate (RendererState&, PSConstantBufferUpdate&);
b8              Renderer_ValidateRenderTarget           (RendererState&, RenderTarget);
b8              Renderer_ValidateDepthBuffer            (RendererState&, DepthBuffer);
b8              Renderer_ValidatePixelShaderResource    (RendererState&, RenderTarget, u32 slot);
b8              Renderer_ValidatePixelShaderResource    (RendererState&, DepthBuffer, u32 slot);
b8              Renderer_ValidateCopy                   (RendererState&, RenderTarget, CPUTexture);
