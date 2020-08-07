#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma warning(push, 0)

// For some GUID magic in the DirectX/DXGI headers. Must be included before them.
#include <InitGuid.h>

#if DEBUG
	#define D3D_DEBUG_INFO
	#include <d3d11sdklayers.h>
	#include <dxgidebug.h>
	#include <dxgi1_3.h>
#endif

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
using namespace DirectX;

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#pragma warning(pop)

namespace HLSLSemantic
{
	const c8* Position = "POSITION";
	const c8* Color    = "COLOR";
	const c8* TexCoord = "TEXCOORD";
}

struct MeshData
{
	Mesh        ref;
	String      name;
	u32         vOffset;
	u32         iOffset;
	u32         iCount;
	DXGI_FORMAT iFormat;
};

struct ConstantBuffer
{
	u32                  size;
	ComPtr<ID3D11Buffer> d3dConstantBuffer;
};

struct VertexShaderData
{
	VertexShader               ref;
	String                     name;
	List<ConstantBuffer>       constantBuffers;
	ComPtr<ID3D11VertexShader> d3dVertexShader;
	ComPtr<ID3D11InputLayout>  d3dInputLayout;
	D3D_PRIMITIVE_TOPOLOGY     d3dPrimitveTopology;
};

struct PixelShaderData
{
	PixelShader               ref;
	String                    name;
	List<ConstantBuffer>      constantBuffers;
	ComPtr<ID3D11PixelShader> d3dPixelShader;
};

struct RenderTargetData
{
	RenderTarget                     ref;
	ComPtr<ID3D11Texture2D>          d3dRenderTarget;
	ComPtr<ID3D11RenderTargetView>   d3dRenderTargetView;
	ComPtr<ID3D11ShaderResourceView> d3dRenderTargetResourceView;
	HANDLE                           sharedHandle;
};

struct DepthBufferData
{
	DepthBuffer                      ref;
	ComPtr<ID3D11DepthStencilView>   d3dDepthBufferView;
	ComPtr<ID3D11ShaderResourceView> d3dDepthBufferResourceView;
};

struct CPUTextureData
{
	CPUTexture               ref;
	ComPtr<ID3D11Texture2D>  d3dCPUTexture;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
};

enum struct ResourceType
{
	Null,
	RenderTarget,
	DepthBuffer,
};

struct PixelShaderResource
{
	ResourceType type;
	u32          slot;
	union
	{
		RenderTarget renderTarget;
		DepthBuffer  depthBuffer;
	};
};

struct BoundResource
{
	ResourceType type;
	u32          slot;
	union
	{
		RenderTargetData* renderTarget;
		DepthBufferData*  depthBuffer;
	};
};

struct CopyResource
{
	RenderTarget source;
	CPUTexture   dest;
};

enum struct RenderCommandType
{
	Null,
	ConstantBufferUpdate,
	DrawMesh,
	PushRenderTarget,
	PopRenderTarget,
	PushDepthBuffer,
	PopDepthBuffer,
	PushVertexShader,
	PopVertexShader,
	PushPixelShader,
	PopPixelShader,
	PushPixelShaderResource,
	PopPixelShaderResource,
	Copy,
};

struct RenderCommand
{
	RenderCommandType type;
	union
	{
		ConstantBufferUpdate cBufUpdate;
		Mesh                 mesh;
		RenderTarget         renderTarget;
		DepthBuffer          depthBuffer;
		VertexShader         vertexShader;
		PixelShader          pixelShader;
		PixelShaderResource  psResource;
		CopyResource         copy;
	};
};

struct RendererState
{
	ComPtr<ID3D11Device>          d3dDevice;
	ComPtr<ID3D11DeviceContext>   d3dContext;
	ComPtr<ID3D11RasterizerState> d3dRasterizerStateSolid;
	ComPtr<ID3D11RasterizerState> d3dRasterizerStateWireframe;
	ComPtr<ID3D11Buffer>          d3dVertexBuffer;
	ComPtr<ID3D11Buffer>          d3dIndexBuffer;

	v2u                           renderSize;
	DXGI_FORMAT                   renderFormat;
	DXGI_FORMAT                   sharedFormat;
	u32                           multisampleCount;
	u32                           qualityCountRender;
	u32                           qualityCountShared;
	b8                            isWireframeEnabled;

	List<VertexShaderData>        vertexShaders;
	List<PixelShaderData>         pixelShaders;
	List<MeshData>                meshes;
	List<Vertex>                  vertexBuffer;
	List<u32>                     indexBuffer;
	List<RenderCommand>           commandList;
	List<RenderTargetData>        renderTargets;
	List<CPUTextureData>          cpuTextures;
	List<DepthBufferData>         depthBuffers;

	RenderTargetData              nullRenderTarget;
	DepthBufferData               nullDepthBuffer;
	VertexShaderData              nullVertexShader;
	PixelShaderData               nullPixelShader;

	// Only here so we don't allocate every frame
	List<RenderTargetData*>       renderTargetStack;
	List<DepthBufferData*>        depthBufferStack;
	List<VertexShaderData*>       vertexShaderStack;
	List<PixelShaderData*>        pixelShaderStack;
	List<BoundResource>           pixelShaderResourceStack;
};

// -------------------------------------------------------------------------------------------------
// Internal functions

#define SetDebugObjectName(resource, format, ...) \
	SetDebugObjectNameChecked<CountPlaceholders(format)>(resource, format, ##__VA_ARGS__)

template<u32 PlaceholderCount, typename T, typename... Args>
static inline void
SetDebugObjectNameChecked(ComPtr<T>& resource, StringView format, Args... args)
{
	static_assert(PlaceholderCount == sizeof...(Args));
	SetDebugObjectNameImpl(resource, format, args...);
}

template<typename T, typename... Args>
static inline void
SetDebugObjectNameImpl(ComPtr<T>& resource, StringView format, Args... args)
{
	#if DEBUG
	String name = String_FormatImpl(format, args...);
	defer { String_Free(name); };

	resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.length, name.data);
	#else
		UNUSED(resource); UNUSED(format); UNUSED_ARGS(args...);
	#endif
}

static void
DestroyRenderTarget(RendererState& s, RenderTargetData& rt)
{
	UNUSED(s);
	rt.d3dRenderTargetResourceView.Reset();
	rt.d3dRenderTargetView.Reset();
	rt.d3dRenderTarget.Reset();
	rt = {};
}

static RenderTarget
CreateRenderTargetImpl(RendererState& s, StringView name, b8 resource, D3D11_TEXTURE2D_DESC& desc)
{
	HRESULT hr;

	RenderTargetData& renderTargetData = List_Append(s.renderTargets);
	renderTargetData.ref = List_GetLastRef(s.renderTargets);

	auto renderTargetGuard = guard {
		DestroyRenderTarget(s, renderTargetData);
	List_RemoveLast(s.renderTargets);
	};

	String index = {};
	defer { String_Free(index); };
	if (name.length == 0)
	{
		index = String_Format("%", s.renderTargets.length);
		name = index;
	}

	// Create texture
	hr = s.d3dDevice->CreateTexture2D(&desc, nullptr, &renderTargetData.d3dRenderTarget);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Fatal, "Failed to create render target");
	SetDebugObjectName(renderTargetData.d3dRenderTarget, "Render Target %", name);

	hr = s.d3dDevice->CreateRenderTargetView(renderTargetData.d3dRenderTarget.Get(), nullptr, &renderTargetData.d3dRenderTargetView);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Fatal, "Failed to create render target view");
	SetDebugObjectName(renderTargetData.d3dRenderTargetView, "Render Target View %", name);

	// Create shader resource
	if (resource)
	{
		s.d3dDevice->CreateShaderResourceView(renderTargetData.d3dRenderTarget.Get(), nullptr, &renderTargetData.d3dRenderTargetResourceView);
		SetDebugObjectName(renderTargetData.d3dRenderTargetResourceView, "Render Target Resource View %", name);
	}

	renderTargetGuard.dismiss = true;
	return renderTargetData.ref;
}

static void
DestroyCPUTexture(RendererState& s, CPUTextureData& ct)
{
	UNUSED(s);
	ct.d3dCPUTexture.Reset();
	ct = {};
}

static void
DestroyDepthBuffer(RendererState& s, DepthBufferData& db)
{
	UNUSED(s);
	db.d3dDepthBufferResourceView.Reset();
	db.d3dDepthBufferView.Reset();
	db = {};
}

static void
DestroyMesh(RendererState& s, MeshData& mesh)
{
	UNUSED(s);
	String_Free(mesh.name);
	mesh = {};
}

static b8
CreateConstantBuffer(RendererState& s, StringView shaderName, u32 index, ConstantBuffer& cBuf)
{
	// TODO: Decide where validation should be handled: simulation or renderer

	LOG_IF(!IsMultipleOf(cBuf.size, 16), return false,
		Severity::Error, "Constant buffer size '%' is not a multiple of 16", cBuf.size);

	D3D11_BUFFER_DESC d3dDesc = {};
	d3dDesc.ByteWidth           = cBuf.size;
	d3dDesc.Usage               = D3D11_USAGE_DYNAMIC;
	d3dDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	d3dDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	d3dDesc.MiscFlags           = 0;
	d3dDesc.StructureByteStride = 0;

	HRESULT hr = s.d3dDevice->CreateBuffer(&d3dDesc, nullptr, &cBuf.d3dConstantBuffer);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create constant buffer");
	SetDebugObjectName(cBuf.d3dConstantBuffer, "% Constant Buffer %", shaderName, index);

	return true;
}

static void
DestroyVertexShader(RendererState& s, VertexShaderData& vs)
{
	UNUSED(s);
	vs.d3dVertexShader.Reset();
	vs.d3dInputLayout.Reset();
	for (u32 j = 0; j < vs.constantBuffers.length; j++)
		vs.constantBuffers[j].d3dConstantBuffer.Reset();
	List_Free(vs.constantBuffers);
	String_Free(vs.name);
	vs = {};
}

static void
DestroyPixelShader(RendererState& s, PixelShaderData& ps)
{
	UNUSED(s);
	ps.d3dPixelShader.Reset();
	for (u32 j = 0; j < ps.constantBuffers.length; j++)
		ps.constantBuffers[j].d3dConstantBuffer.Reset();
	List_Free(ps.constantBuffers);
	String_Free(ps.name);
	ps = {};
}

static void
UpdateRasterizerState(RendererState& s)
{
	if (s.isWireframeEnabled)
	{
		s.d3dContext->RSSetState(s.d3dRasterizerStateWireframe.Get());
	}
	else
	{
		s.d3dContext->RSSetState(s.d3dRasterizerStateSolid.Get());
	}
}

static b8
UpdateConstantBuffer(RendererState& s, ConstantBuffer& cBuf, void* data)
{
	D3D11_MAPPED_SUBRESOURCE map = {};
	HRESULT hr = s.d3dContext->Map(cBuf.d3dConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to map constant buffer");

	memcpy(map.pData, data, cBuf.size);
	s.d3dContext->Unmap(cBuf.d3dConstantBuffer.Get(), 0);

	return true;
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Resource Creation

// TODO: Try using the keyed mutex flag to synchronize sharing
RenderTarget
Renderer_CreateRenderTarget(RendererState& s, StringView name, b8 resource)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width              = s.renderSize.x;
	desc.Height             = s.renderSize.y;
	desc.MipLevels          = 1;
	desc.ArraySize          = 1;
	desc.Format             = s.renderFormat;
	desc.SampleDesc.Count   = s.multisampleCount;
	desc.SampleDesc.Quality = s.qualityCountRender - 1;
	desc.Usage              = D3D11_USAGE_DEFAULT;
	desc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags     = 0;
	desc.MiscFlags          = 0;
	return CreateRenderTargetImpl(s, name, resource, desc);
}

RenderTarget
Renderer_CreateSharedRenderTarget(RendererState& s, StringView name, b8 resource)
{
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width              = s.renderSize.x;
	desc.Height             = s.renderSize.y;
	desc.MipLevels          = 1;
	desc.ArraySize          = 1;
	desc.Format             = s.sharedFormat;
	desc.SampleDesc.Count   = s.multisampleCount;
	desc.SampleDesc.Quality = s.qualityCountRender - 1;
	desc.Usage              = D3D11_USAGE_DEFAULT;
	desc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags     = 0;
	desc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;
	RenderTarget rt = CreateRenderTargetImpl(s, name, resource, desc);
	if (!rt) return {};

	RenderTargetData& renderTargetData = s.renderTargets[rt];

	auto renderTargetGuard = guard {
		DestroyRenderTarget(s, renderTargetData);
		List_RemoveLast(s.renderTargets);
	};

	HRESULT hr;

	// Shared handle
	ComPtr<IDXGIResource> dxgiRenderTarget;
	hr = renderTargetData.d3dRenderTarget.As(&dxgiRenderTarget);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Error, "Failed to get DXGI render target");

	// TODO: Docs recommend IDXGIResource1::CreateSharedHandle
	// https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiresource-getsharedhandle
	hr = dxgiRenderTarget->GetSharedHandle(&renderTargetData.sharedHandle);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Error, "Failed to get DXGI render target handle");

	renderTargetGuard.dismiss = true;
	return rt;
}

CPUTexture
Renderer_CreateCPUTexture(RendererState& s, StringView name)
{
	CPUTextureData& cpuTextureData = List_Append(s.cpuTextures);
	cpuTextureData.ref = List_GetLastRef(s.cpuTextures);

	auto cpuTextureGuard = guard {
		DestroyCPUTexture(s, cpuTextureData);
		List_RemoveLast(s.cpuTextures);
	};

	String index = {};
	defer { String_Free(index); };
	if (name.length == 0)
	{
		index = String_Format("%", s.cpuTextures.length);
		name = index;
	}

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width              = s.renderSize.x;
	desc.Height             = s.renderSize.y;
	desc.MipLevels          = 1;
	desc.ArraySize          = 1;
	desc.Format             = s.renderFormat;
	desc.SampleDesc.Count   = s.multisampleCount;
	desc.SampleDesc.Quality = s.qualityCountRender - 1;
	desc.Usage              = D3D11_USAGE_STAGING;
	desc.BindFlags          = 0;
	desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags          = 0;

	HRESULT hr = s.d3dDevice->CreateTexture2D(&desc, nullptr, &cpuTextureData.d3dCPUTexture);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Fatal, "Failed to create CPU texture");
	SetDebugObjectName(cpuTextureData.d3dCPUTexture, "CPU Texture %", name);

	cpuTextureGuard.dismiss = true;
	return cpuTextureData.ref;
}

DepthBuffer
Renderer_CreateDepthBuffer(RendererState& s, StringView name, b8 resource)
{
	DepthBufferData& depthBufferData = List_Append(s.depthBuffers);
	depthBufferData.ref = List_GetLastRef(s.depthBuffers);

	auto depthBufferGuard = guard {
		DestroyDepthBuffer(s, depthBufferData);
		List_RemoveLast(s.depthBuffers);
	};

	String index = {};
	defer { String_Free(index); };
	if (name.length == 0)
	{
		index = String_Format("%", s.depthBuffers.length);
		name = index;
	}

	// Create texture
	HRESULT hr;

	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width              = s.renderSize.x;
	depthDesc.Height             = s.renderSize.y;
	depthDesc.MipLevels          = 1;
	depthDesc.ArraySize          = 1;
	depthDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// TODO: Doubt this is correct...
	depthDesc.SampleDesc.Count   = s.multisampleCount;
	depthDesc.SampleDesc.Quality = s.qualityCountRender - 1;
	depthDesc.Usage              = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags     = 0;
	depthDesc.MiscFlags          = 0;

	if (resource)
	{
		depthDesc.Format     = DXGI_FORMAT_R24G8_TYPELESS;
		depthDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	}

	ComPtr<ID3D11Texture2D> d3dDepthBuffer;
	hr = s.d3dDevice->CreateTexture2D(&depthDesc, nullptr, &d3dDepthBuffer);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Fatal, "Failed to create depth buffer");
	SetDebugObjectName(d3dDepthBuffer, "Depth Buffer %", name);

	// TODO: Is this needed?
	D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {};
	depthViewDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthViewDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthViewDesc.Flags              = 0;
	depthViewDesc.Texture2D.MipSlice = 0;

	hr = s.d3dDevice->CreateDepthStencilView(d3dDepthBuffer.Get(), &depthViewDesc, &depthBufferData.d3dDepthBufferView);
	LOG_HRESULT_IF_FAILED(hr, return {},
		Severity::Fatal, "Failed to create depth buffer view");
	SetDebugObjectName(depthBufferData.d3dDepthBufferView, "Depth Buffer View %", name);

	// Create shader resource
	if (resource)
	{
		// TODO: Why can't this be DXGI_FORMAT_D24_UNORM_S8_UINT? Will we be able to read stencil bits?
		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = {};
		resourceDesc.Format                    = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		resourceDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MostDetailedMip = 0;
		resourceDesc.Texture2D.MipLevels       = UINT(-1);

		s.d3dDevice->CreateShaderResourceView(d3dDepthBuffer.Get(), &resourceDesc, &depthBufferData.d3dDepthBufferResourceView);
		SetDebugObjectName(depthBufferData.d3dDepthBufferResourceView, "Depth Buffer Resource View %", name);
	}

	depthBufferGuard.dismiss = true;
	return depthBufferData.ref;
}

b8
Renderer_RebuildSharedGeometryBuffers(RendererState&s)
{
	// Finalize vertex buffer
	{
		D3D11_BUFFER_DESC vertBuffDesc = {};
		vertBuffDesc.ByteWidth           = (u32) List_SizeOf(s.vertexBuffer);
		vertBuffDesc.Usage               = D3D11_USAGE_DYNAMIC;
		vertBuffDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
		vertBuffDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
		vertBuffDesc.MiscFlags           = 0;
		vertBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vertBuffInitData = {};
		vertBuffInitData.pSysMem          = s.vertexBuffer.data;
		vertBuffInitData.SysMemPitch      = 0;
		vertBuffInitData.SysMemSlicePitch = 0;

		s.d3dVertexBuffer.Reset();
		HRESULT hr = s.d3dDevice->CreateBuffer(&vertBuffDesc, &vertBuffInitData, &s.d3dVertexBuffer);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create shared vertex buffer");
		SetDebugObjectName(s.d3dVertexBuffer, "Shared Vertex Buffer");
	}

	// Finalize index buffer
	{
		D3D11_BUFFER_DESC indexBuffDesc = {};
		indexBuffDesc.ByteWidth           = (u32) List_SizeOf(s.indexBuffer);
		indexBuffDesc.Usage               = D3D11_USAGE_IMMUTABLE;
		indexBuffDesc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
		indexBuffDesc.CPUAccessFlags      = 0;
		indexBuffDesc.MiscFlags           = 0;
		indexBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexBuffInitData = {};
		indexBuffInitData.pSysMem          = s.indexBuffer.data;
		indexBuffInitData.SysMemPitch      = 0;
		indexBuffInitData.SysMemSlicePitch = 0;

		s.d3dIndexBuffer.Reset();
		HRESULT hr = s.d3dDevice->CreateBuffer(&indexBuffDesc, &indexBuffInitData, &s.d3dIndexBuffer);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create shared index buffer");
		SetDebugObjectName(s.d3dIndexBuffer, "Shared Index Buffer");
	}

	return true;
}

Mesh
Renderer_CreateMesh(RendererState& s, StringView name, Slice<Vertex> vertices, Slice<Index> indices)
{
	MeshData& mesh = List_Append(s.meshes);
	mesh.ref  = List_GetLastRef(s.meshes);
	mesh.name = String_FromView(name);

	// Copy Data
	{
		mesh.vOffset = s.vertexBuffer.length;
		mesh.iOffset = s.indexBuffer.length;
		mesh.iCount  = indices.length;
		mesh.iFormat = DXGI_FORMAT_R32_UINT;

		List_AppendRange(s.vertexBuffer, vertices);
		List_AppendRange(s.indexBuffer, indices);
	}

	return mesh.ref;
}

void
Renderer_ValidateMesh(RendererState& s, Mesh mesh)
{
	Assert(List_IsRefValid(s.meshes, mesh));
}

VertexShader
Renderer_LoadVertexShader(RendererState& s, StringView name, StringView path, Slice<VertexAttribute> attributes, Slice<u32> cBufSizes)
{
	// Vertex Shader
	VertexShaderData& vs = List_Append(s.vertexShaders);
	vs.ref = List_GetLastRef(s.vertexShaders);

	auto vsGuard = guard {
		DestroyVertexShader(s, vs);
		List_RemoveLast(s.vertexShaders);
	};

	vs.name = String_FromView(name);

	// Load
	Bytes vsBytes = {};
	defer { List_Free(vsBytes); };
	{
		vsBytes = Platform_LoadFileBytes(path);
		if (!vsBytes.data) return VertexShader::Null;
	}

	// Create
	{
		HRESULT hr = s.d3dDevice->CreateVertexShader(vsBytes.data, vsBytes.length, nullptr, &vs.d3dVertexShader);
		LOG_HRESULT_IF_FAILED(hr, return VertexShader::Null,
			Severity::Error, "Failed to create vertex shader '%'", path);
		SetDebugObjectName(vs.d3dVertexShader, "Vertex Shader: %", name);
	}

	// Constant Buffers
	if (cBufSizes.length)
	{
		List_Reserve(vs.constantBuffers, cBufSizes.length);
		for (u32 i = 0; i < cBufSizes.length; i++)
		{
			ConstantBuffer& cBuf = List_Append(vs.constantBuffers);
			cBuf.size = cBufSizes[i];

			b8 success = CreateConstantBuffer(s, name, i, cBuf);
			LOG_IF(!success, return VertexShader::Null,
				Severity::Error, "Failed to create VS constant buffer % for '%'", i, path);
		}
	}

	// Input layout
	{
		List<D3D11_INPUT_ELEMENT_DESC> vsInputDescs = {};
		List_Reserve(vsInputDescs, attributes.length);
		for (u32 i = 0; i < attributes.length; i++)
		{
			const c8* semantic;
			switch (attributes[i].semantic)
			{
				case VertexAttributeSemantic::Position: semantic = HLSLSemantic::Position; break;
				case VertexAttributeSemantic::Color:    semantic = HLSLSemantic::Color;    break;
				case VertexAttributeSemantic::TexCoord: semantic = HLSLSemantic::TexCoord; break;

				default:
					LOG(Severity::Error, "Unrecognized VS attribute semantic % '%'", (i32) attributes[i].semantic, path);
					return VertexShader::Null;
			}

			DXGI_FORMAT format;
			switch (attributes[i].format)
			{
				case VertexAttributeFormat::v2: format = DXGI_FORMAT_R32G32_FLOAT;       break;
				case VertexAttributeFormat::v3: format = DXGI_FORMAT_R32G32B32_FLOAT;    break;
				case VertexAttributeFormat::v4: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;

				default:
					LOG(Severity::Error, "Unrecognized VS attribute format % '%'", (i32) attributes[i].format, path);
					return VertexShader::Null;
			}

			D3D11_INPUT_ELEMENT_DESC inputDesc = {};
			inputDesc.SemanticName         = semantic;
			inputDesc.SemanticIndex        = 0;
			inputDesc.Format               = format;
			inputDesc.InputSlot            = 0;
			inputDesc.AlignedByteOffset    = D3D11_APPEND_ALIGNED_ELEMENT;
			inputDesc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
			inputDesc.InstanceDataStepRate = 0;
			List_Append(vsInputDescs, inputDesc);
		}

		HRESULT hr = s.d3dDevice->CreateInputLayout(vsInputDescs.data, vsInputDescs.length, vsBytes.data, vsBytes.length, &vs.d3dInputLayout);
		LOG_HRESULT_IF_FAILED(hr, return VertexShader::Null,
			Severity::Error, "Failed to create VS input layout '%'", path);
		SetDebugObjectName(vs.d3dInputLayout, "Input Layout: %", name);

		vs.d3dPrimitveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	vsGuard.dismiss = true;
	return vs.ref;
}

void
Renderer_ValidateVertexShader(RendererState& s, VertexShader vs)
{
	Assert(List_IsRefValid(s.vertexShaders, vs));
}

PixelShader
Renderer_LoadPixelShader(RendererState& s, StringView name, StringView path, Slice<u32> cBufSizes)
{
	PixelShaderData& ps = List_Append(s.pixelShaders);
	ps.ref = List_GetLastRef(s.pixelShaders);

	auto psGuard = guard {
		DestroyPixelShader(s, ps);
		List_RemoveLast(s.pixelShaders);
	};

	ps.name = String_FromView(name);

	// Load
	Bytes psBytes = {};
	defer { List_Free(psBytes); };
	{
		psBytes = Platform_LoadFileBytes(path);
		if (!psBytes.data) return PixelShader::Null;
	}

	// Create
	{
		HRESULT hr = s.d3dDevice->CreatePixelShader(psBytes.data, (u64) psBytes.length, nullptr, &ps.d3dPixelShader);
		LOG_HRESULT_IF_FAILED(hr, return PixelShader::Null,
			Severity::Error, "Failed to create pixel shader '%'", path);
		SetDebugObjectName(ps.d3dPixelShader, "Pixel Shader: %", name);
	}

	// Constant Buffers
	if (cBufSizes.length)
	{
		List_Reserve(ps.constantBuffers, cBufSizes.length);
		for (u32 i = 0; i < cBufSizes.length; i++)
		{
			ConstantBuffer& cBuf = List_Append(ps.constantBuffers);
			cBuf.size = cBufSizes[i];

			b8 success = CreateConstantBuffer(s, name, i, cBuf);
			LOG_IF(!success, return PixelShader::Null,
				Severity::Error, "Failed to create PS constant buffer % for '%'", i, path);
		}
	}

	psGuard.dismiss = true;
	return ps.ref;
}

void
Renderer_ValidatePixelShader(RendererState& s, PixelShader ps)
{
	Assert(List_IsRefValid(s.pixelShaders, ps));
}

void
Renderer_ValidateMaterial(RendererState& s, Material& material)
{
	Renderer_ValidateMesh(s, material.mesh);
	Renderer_ValidateVertexShader(s, material.vs);
	Renderer_ValidatePixelShader(s, material.ps);
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Rendering Operations

void
Renderer_PushConstantBufferUpdate(RendererState& s, ConstantBufferUpdate& cbu)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::ConstantBufferUpdate;
	renderCommand.cBufUpdate = cbu;
}

void
Renderer_ValidateConstantBufferUpdate(RendererState& s, ConstantBufferUpdate& cbu)
{
	Renderer_ValidateMaterial(s, cbu.material);

	switch (cbu.shaderStage)
	{
		case ShaderStage::Null: Assert(false); break;

		case ShaderStage::Vertex:
		{
			VertexShaderData& vs = s.vertexShaders[cbu.material.vs];
			Assert(cbu.index < vs.constantBuffers.length);
			break;
		}

		case ShaderStage::Pixel:
		{
			PixelShaderData& ps = s.pixelShaders[cbu.material.ps];
			Assert(cbu.index < ps.constantBuffers.length);
			break;
		}
	}
}

void
Renderer_PushDrawMesh(RendererState& s, Mesh mesh)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::DrawMesh;
	renderCommand.mesh = mesh;
}

void
Renderer_PushRenderTarget(RendererState& s, RenderTarget rt)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushRenderTarget;
	renderCommand.renderTarget = rt;
}

void
Renderer_PopRenderTarget(RendererState& s)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PopRenderTarget;
}

void
Renderer_PushDepthBuffer(RendererState& s, DepthBuffer db)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushDepthBuffer;
	renderCommand.depthBuffer = db;
}

void
Renderer_PopDepthBuffer(RendererState& s)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PopDepthBuffer;
}

void
Renderer_PushVertexShader(RendererState& s, VertexShader vs)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushVertexShader;
	renderCommand.vertexShader = vs;
}

void
Renderer_PopVertexShader(RendererState& s)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PopVertexShader;
}

void
Renderer_PushPixelShader(RendererState& s, PixelShader ps)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushPixelShader;
	renderCommand.pixelShader = ps;
}

void
Renderer_PopPixelShader(RendererState& s)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PopPixelShader;
}

void
Renderer_PushPixelShaderResource(RendererState& s, RenderTarget rt, u32 slot)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushPixelShaderResource;
	renderCommand.psResource.type = ResourceType::RenderTarget;
	renderCommand.psResource.slot = slot;
	renderCommand.psResource.renderTarget = rt;
}

void
Renderer_PushPixelShaderResource(RendererState& s, DepthBuffer db, u32 slot)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PushPixelShaderResource;
	renderCommand.psResource.type = ResourceType::DepthBuffer;
	renderCommand.psResource.slot = slot;
	renderCommand.psResource.depthBuffer = db;
}

void
Renderer_PopPixelShaderResource(RendererState& s)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::PopPixelShaderResource;
}

void
Renderer_Copy(RendererState& s, RenderTarget rt, CPUTexture ct)
{
	RenderCommand& renderCommand = List_Append(s.commandList);
	renderCommand.type = RenderCommandType::Copy;
	renderCommand.copy.source = rt;
	renderCommand.copy.dest = ct;
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Misc

size
Renderer_GetSharedRenderTargetHandle(RendererState& s, RenderTarget rt)
{
	RenderTargetData& renderTargetData = s.renderTargets[rt];
	return (size) renderTargetData.sharedHandle;
}

CPUTextureBytes
Renderer_GetCPUTextureBytes(RendererState& s, CPUTexture ct)
{
	CPUTextureData& cpuTextureData = s.cpuTextures[ct];

	CPUTextureBytes textureBytes = {};
	textureBytes.bytes.data   = (u8*) cpuTextureData.mappedResource.pData;
	textureBytes.bytes.length = cpuTextureData.mappedResource.DepthPitch;
	textureBytes.size         = s.renderSize;
	textureBytes.rowStride    = cpuTextureData.mappedResource.RowPitch;
	textureBytes.pixelStride  = 2;

	Assert(s.renderFormat == DXGI_FORMAT_B5G6R5_UNORM);
	return textureBytes;
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Core Functions

b8
Renderer_Initialize(RendererState& s, v2u renderSize)
{
	Assert(renderSize.x < i32Max && renderSize.y < i32Max);

	s.renderSize       = renderSize;
	s.renderFormat     = DXGI_FORMAT_B5G6R5_UNORM;
	s.sharedFormat     = DXGI_FORMAT_B8G8R8A8_UNORM;
	s.multisampleCount = 1;


	// Create device
	{
		HRESULT hr;

		UINT createDeviceFlags = 0;
		// TODO: Using this flag causes a crash when RTSS tries to hook the process
		//createDeviceFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
		#if DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUGGABLE; //11_1+
		#endif

		D3D_FEATURE_LEVEL featureLevel = {};

		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			nullptr, 0, // NOTE: 11_1 will never be created by default
			D3D11_SDK_VERSION,
			&s.d3dDevice,
			&featureLevel,
			&s.d3dContext
		);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create D3D device");
		SetDebugObjectName(s.d3dDevice,  "Device");
		SetDebugObjectName(s.d3dContext, "Device Context");

		// Check feature level
		LOG_IF(!HAS_FLAG(featureLevel, D3D_FEATURE_LEVEL_11_0), return false,
			Severity::Fatal, "Created Device does not support D3D 11");

		// Obtain the DXGI factory used to create the current device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		hr = s.d3dDevice.As(&dxgiDevice);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI device");
		// NOTE: It looks like the IDXGIDevice is actually the same object as
		// the ID3D11Device. Using SetPrivateData to set its name clobbers the
		// D3D device name and outputs a warning.

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI adapter");

		// Check for the WARP driver
		DXGI_ADAPTER_DESC desc = {};
		hr = dxgiAdapter->GetDesc(&desc);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI adapter description");

		b8 warpDriverInUse = (desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c);
		LOG_IF(warpDriverInUse, IGNORE,
			Severity::Warning, "WARP driver in use.");
	}


	// Configure debugging
	{
		#if DEBUG
		HRESULT hr;

		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Warning, "Failed to get DXGI debug interface");

		dxgiDebug->EnableLeakTrackingForThread();

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Warning, "Failed to get DXGI info queue");

		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		LOG_HRESULT_IF_FAILED(hr, IGNORE,
			Severity::Warning, "Failed to set DXGI break on error");
		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		LOG_HRESULT_IF_FAILED(hr, IGNORE,
			Severity::Warning, "Failed to set DXGI break on corruption");

		// Ignore warning for not using a flip-mode swap chain
		DXGI_INFO_QUEUE_MESSAGE_ID ids[] = { 294 };
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs  = (u32) ArrayLength(ids);
		filter.DenyList.pIDList = ids;
		hr = dxgiInfoQueue->PushStorageFilter(DXGI_DEBUG_DXGI, &filter);
		LOG_HRESULT_IF_FAILED(hr, IGNORE,
			Severity::Warning, "Failed to set DXGI warning filter");
		#endif
	}


	// Query Quality Levels
	{
		HRESULT hr;

		hr = s.d3dDevice->CheckMultisampleQualityLevels(s.renderFormat, s.multisampleCount, &s.qualityCountRender);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to query supported multisample levels for render targets");

		hr = s.d3dDevice->CheckMultisampleQualityLevels(s.sharedFormat, s.multisampleCount, &s.qualityCountShared);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to query supported multisample levels for shared textures");
	}


	// Initialize viewport
	{
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width    = (r32) s.renderSize.x;
		viewport.Height   = (r32) s.renderSize.y;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;

		s.d3dContext->RSSetViewports(1, &viewport);
	}


	// Initialize rasterizer state
	{
		HRESULT hr;

		// NOTE: MultisampleEnable toggles between quadrilateral AA (true) and alpha AA (false).
		// Alpha AA has a massive performance impact while quadrilateral is much smaller
		// (negligible for the mesh drawn here). Visually, it's hard to tell the difference between
		// quadrilateral AA on and off in this demo. Alpha AA on the other hand is more obvious. It
		// causes the wireframe to draw lines 2 px wide instead of 1. See remarks:
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
		const b8 useQuadrilateralLineAA = true;

		// Solid
		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode              = D3D11_CULL_NONE;
		rasterizerDesc.FrontCounterClockwise = false;
		rasterizerDesc.DepthBias             = 0;
		rasterizerDesc.DepthBiasClamp        = 0;
		rasterizerDesc.SlopeScaledDepthBias  = 0;
		rasterizerDesc.DepthClipEnable       = true;
		rasterizerDesc.ScissorEnable         = false;
		rasterizerDesc.MultisampleEnable     = s.multisampleCount > 1 && useQuadrilateralLineAA;
		rasterizerDesc.AntialiasedLineEnable = s.multisampleCount > 1;

		hr = s.d3dDevice->CreateRasterizerState(&rasterizerDesc, &s.d3dRasterizerStateSolid);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create solid rasterizer state");
		SetDebugObjectName(s.d3dRasterizerStateSolid, "Rasterizer State (Solid)");

		// Wireframe
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;

		hr = s.d3dDevice->CreateRasterizerState(&rasterizerDesc, &s.d3dRasterizerStateWireframe);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create wireframe rasterizer state");
		SetDebugObjectName(s.d3dRasterizerStateWireframe, "Rasterizer State (Wireframe)");

		// Start off in correct state
		UpdateRasterizerState(s);
	}


	// Initialize blend state
	{
		HRESULT hr;

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable                 = false;
		blendDesc.IndependentBlendEnable                = false;
		blendDesc.RenderTarget[0].BlendEnable           = true;
		blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		ComPtr<ID3D11BlendState> blendState;
		hr = s.d3dDevice->CreateBlendState(&blendDesc, &blendState);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create alpha blend state");
		SetDebugObjectName(blendState, "Blend State (Alpha)");

		s.d3dContext->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);
	}


	// DEBUG: Default sampler
	{
		HRESULT hr;

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias     = 0;
		samplerDesc.MaxAnisotropy  = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.MinLOD         = 0;
		samplerDesc.MaxLOD         = 0;

		ComPtr<ID3D11SamplerState> samplerState;
		hr = s.d3dDevice->CreateSamplerState(&samplerDesc, &samplerState);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create default sampler state");

		s.d3dContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());
		SetDebugObjectName(samplerState, "Debug Sampler State");
	}

	return true;
}

void
Renderer_Teardown(RendererState& s)
{
	List_Free(s.pixelShaderResourceStack);
	List_Free(s.pixelShaderStack);
	List_Free(s.vertexShaderStack);
	List_Free(s.depthBufferStack);
	List_Free(s.renderTargetStack);
	List_Free(s.commandList);
	List_Free(s.indexBuffer);
	List_Free(s.vertexBuffer);

	for (u32 i = 0; i < s.cpuTextures.length; i++)
	{
		CPUTextureData& ct = s.cpuTextures[i];
		DestroyCPUTexture(s, ct);
	}
	List_Free(s.renderTargets);

	for (u32 i = 0; i < s.depthBuffers.length; i++)
	{
		DepthBufferData& db = s.depthBuffers[i];
		DestroyDepthBuffer(s, db);
	}
	List_Free(s.depthBuffers);

	for (u32 i = 0; i < s.renderTargets.length; i++)
	{
		RenderTargetData& rt = s.renderTargets[i];
		DestroyRenderTarget(s, rt);
	}
	List_Free(s.renderTargets);

	for (u32 i = 0; i < s.meshes.length; i++)
	{
		MeshData& mesh = s.meshes[i];
		DestroyMesh(s, mesh);
	}
	List_Free(s.meshes);

	for (u32 i = 0; i < s.pixelShaders.length; i++)
	{
		PixelShaderData& ps = s.pixelShaders[i];
		DestroyPixelShader(s, ps);
	}
	List_Free(s.pixelShaders);

	for (u32 i = 0; i < s.vertexShaders.length; i++)
	{
		VertexShaderData& vs = s.vertexShaders[i];
		DestroyVertexShader(s, vs);
	}
	List_Free(s.vertexShaders);

	s.d3dIndexBuffer             .Reset();
	s.d3dVertexBuffer            .Reset();
	s.d3dRasterizerStateWireframe.Reset();
	s.d3dRasterizerStateSolid    .Reset();
	s.d3dContext                 .Reset();
	s.d3dDevice                  .Reset();

	// Log live objects
	{
		#if DEBUG
		HRESULT hr;

		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_HRESULT_IF_FAILED(hr, return,
			Severity::Warning, "Failed to get DXGI debug interface");

		// NOTE: Only available with the Windows SDK installed. That's not
		// unituitive and doesn't lead to cryptic errors or anything. Fuck you,
		// Microsoft.
		hr = dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		LOG_HRESULT_IF_FAILED(hr, return,
			Severity::Warning, "Failed to report live D3D objects");

		Platform_Print("\n");
		#endif
	}
}

b8
Renderer_Render(RendererState& s)
{
	// OPTIMIZE: Make clears a manual call so we don't do unnecessary clears
	for (u32 i = 0; i < s.renderTargets.length; i++)
	{
		RenderTargetData& rt = s.renderTargets[i];
		s.d3dContext->ClearRenderTargetView(rt.d3dRenderTargetView.Get(), DirectX::Colors::Black);
	}

	for (u32 i = 0; i < s.depthBuffers.length; i++)
	{
		DepthBufferData& db = s.depthBuffers[i];
		s.d3dContext->ClearDepthStencilView(db.d3dDepthBufferView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	}

	for (u32 i = 0; i < s.cpuTextures.length; i++)
	{
		CPUTextureData& ct = s.cpuTextures[i];
		if (ct.mappedResource.pData)
		{
			s.d3dContext->Unmap(ct.d3dCPUTexture.Get(), 0);
			ct.mappedResource = {};
		}
	}

	// TODO: No reason for this to be all one struct. Split it up.
	struct BindState
	{
		RenderTargetData* rt;
		DepthBufferData*  db;
		VertexShaderData* vs;
		PixelShaderData*  ps;
		BoundResource     psr;
	};

	// TODO: Is there a better way to initialize these than having dummy null objects?
	BindState bindState = {};
	bindState.rt = &s.nullRenderTarget;
	bindState.db = &s.nullDepthBuffer;
	bindState.vs = &s.nullVertexShader;
	bindState.ps = &s.nullPixelShader;

	// OPTIMIZE: Sort draws?
	// OPTIMIZE: Adding a "SetMaterial" command would remove some redundant shader lookups.
	// OPTIMIZE: Add checks to only issue driver calls if state really changed
	// OPTIMIZE: Don't reset the BindState every frame?

	// TODO: Pixel shader blend appears wrong. Use a gray clear to diagnose.

	for (u32 i = 0; i < s.commandList.length; i++)
	{
		RenderCommand& renderCommand = s.commandList[i];
		switch (renderCommand.type)
		{
			case RenderCommandType::Null: Assert(false); break;

			case RenderCommandType::ConstantBufferUpdate:
			{
				ConstantBufferUpdate& cbu = renderCommand.cBufUpdate;

				// TODO: Do validation in API call (ps & vs refs, shader stage, etc)
				// TODO: If we fail to update a constant buffer do we skip drawing?
				ConstantBuffer* cBuf = nullptr;
				switch (cbu.shaderStage)
				{
					case ShaderStage::Null: Assert(false); break;

					case ShaderStage::Vertex:
					{
						VertexShaderData& vs = s.vertexShaders[cbu.material.vs];
						cBuf = &vs.constantBuffers[cbu.index];
						break;
					}

					case ShaderStage::Pixel:
					{
						PixelShaderData& ps = s.pixelShaders[cbu.material.ps];
						cBuf = &ps.constantBuffers[cbu.index];
						break;
					}
				}

				b8 success = UpdateConstantBuffer(s, *cBuf, cbu.data);
				if (!success) break;
				break;
			}

			case RenderCommandType::DrawMesh:
			{
				MeshData& mesh = s.meshes[renderCommand.mesh];

				Assert(mesh.vOffset < i32Max);
				s.d3dContext->DrawIndexed(mesh.iCount, mesh.iOffset, (i32) mesh.vOffset);
				break;
			}

			case RenderCommandType::PushRenderTarget:
			{
				RenderTargetData& rt = renderCommand.renderTarget ? s.renderTargets[renderCommand.renderTarget] : s.nullRenderTarget;
				// TODO: Why do we need null object here?
				DepthBufferData&  db = bindState.db ? *bindState.db : s.nullDepthBuffer;

				List_Push(s.renderTargetStack, bindState.rt);
				bindState.rt = &rt;

				s.d3dContext->OMSetRenderTargets(1, rt.d3dRenderTargetView.GetAddressOf(), db.d3dDepthBufferView.Get());
				break;
			}

			case RenderCommandType::PopRenderTarget:
			{
				Assert(s.renderTargetStack.length != 0);
				RenderTargetData& rt = *List_Pop(s.renderTargetStack);
				DepthBufferData&  db = bindState.db ? *bindState.db : s.nullDepthBuffer;

				bindState.rt = &rt;

				s.d3dContext->OMSetRenderTargets(1, rt.d3dRenderTargetView.GetAddressOf(), db.d3dDepthBufferView.Get());
				break;
			}

			case RenderCommandType::PushDepthBuffer:
			{
				DepthBufferData&  db = renderCommand.depthBuffer? s.depthBuffers[renderCommand.depthBuffer] : s.nullDepthBuffer;
				RenderTargetData& rt = bindState.rt ? *bindState.rt : s.nullRenderTarget;

				List_Push(s.depthBufferStack, bindState.db);
				bindState.db = &db;

				s.d3dContext->OMSetRenderTargets(1, rt.d3dRenderTargetView.GetAddressOf(), db.d3dDepthBufferView.Get());
				break;
			}

			case RenderCommandType::PopDepthBuffer:
			{
				Assert(s.depthBufferStack.length != 0);
				DepthBufferData&  db = *List_Pop(s.depthBufferStack);
				RenderTargetData& rt = bindState.rt ? *bindState.rt : s.nullRenderTarget;

				bindState.db = &db;

				s.d3dContext->OMSetRenderTargets(1, rt.d3dRenderTargetView.GetAddressOf(), db.d3dDepthBufferView.Get());
				break;
			}

			// TODO: Can't pop draw calls, so maybe don't call it a push?
			case RenderCommandType::PushVertexShader:
			{
				VertexShaderData& vs = s.vertexShaders[renderCommand.vertexShader];

				List_Push(s.vertexShaderStack, bindState.vs);

				if (bindState.vs != &vs)
				{
					bindState.vs = &vs;

					u32 vStride = (u32) sizeof(Vertex);
					u32 vOffset = 0;

					s.d3dContext->VSSetShader(vs.d3dVertexShader.Get(), nullptr, 0);
					s.d3dContext->IASetInputLayout(vs.d3dInputLayout.Get());
					s.d3dContext->IASetVertexBuffers(0, 1, s.d3dVertexBuffer.GetAddressOf(), &vStride, &vOffset);
					s.d3dContext->IASetIndexBuffer(s.d3dIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					s.d3dContext->IASetPrimitiveTopology(vs.d3dPrimitveTopology);

					for (u32 j = 0; j < vs.constantBuffers.length; j++)
					{
						ConstantBuffer& cBuf = vs.constantBuffers[j];
						s.d3dContext->VSSetConstantBuffers(j, 1, cBuf.d3dConstantBuffer.GetAddressOf());
					}
				}
				break;
			}

			case RenderCommandType::PopVertexShader:
			{
				Assert(s.vertexShaderStack.length != 0);
				VertexShaderData& vs = *List_Pop(s.vertexShaderStack);

				if (bindState.vs != &vs)
				{
					bindState.vs = &vs;

					u32 vStride = (u32) sizeof(Vertex);
					u32 vOffset = 0;

					s.d3dContext->VSSetShader(vs.d3dVertexShader.Get(), nullptr, 0);
					s.d3dContext->IASetInputLayout(vs.d3dInputLayout.Get());
					s.d3dContext->IASetVertexBuffers(0, 1, s.d3dVertexBuffer.GetAddressOf(), &vStride, &vOffset);
					s.d3dContext->IASetIndexBuffer(s.d3dIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					s.d3dContext->IASetPrimitiveTopology(vs.d3dPrimitveTopology);

					for (u32 j = 0; j < vs.constantBuffers.length; j++)
					{
						ConstantBuffer& cBuf = vs.constantBuffers[j];
						s.d3dContext->VSSetConstantBuffers(j, 1, cBuf.d3dConstantBuffer.GetAddressOf());
					}
				}
				break;
			}

			case RenderCommandType::PushPixelShader:
			{
				PixelShaderData& ps = s.pixelShaders[renderCommand.pixelShader];

				List_Push(s.pixelShaderStack, bindState.ps);

				if (bindState.ps != &ps)
				{
					bindState.ps = &ps;
					s.d3dContext->PSSetShader(ps.d3dPixelShader.Get(), nullptr, 0);

					for (u32 j = 0; j < ps.constantBuffers.length; j++)
					{
						ConstantBuffer& cBuf = ps.constantBuffers[j];
						s.d3dContext->PSSetConstantBuffers(j, 1, cBuf.d3dConstantBuffer.GetAddressOf());
					}
				}
				break;
			}

			case RenderCommandType::PopPixelShader:
			{
				Assert(s.pixelShaderStack.length != 0);
				PixelShaderData& ps = *List_Pop(s.pixelShaderStack);

				if (bindState.ps != &ps)
				{
					bindState.ps = &ps;
					s.d3dContext->PSSetShader(ps.d3dPixelShader.Get(), nullptr, 0);

					for (u32 j = 0; j < ps.constantBuffers.length; j++)
					{
						ConstantBuffer& cBuf = ps.constantBuffers[j];
						s.d3dContext->PSSetConstantBuffers(j, 1, cBuf.d3dConstantBuffer.GetAddressOf());
					}
				}
				break;
			}

			case RenderCommandType::PushPixelShaderResource:
			{
				PixelShaderResource& psr = renderCommand.psResource;

				bindState.psr.type = psr.type;
				bindState.psr.slot = psr.slot;
				List_Push(s.pixelShaderResourceStack, bindState.psr);

				switch (psr.type)
				{
					case ResourceType::Null: Assert(false); break;

					case ResourceType::RenderTarget:
					{
						RenderTargetData& rt = s.renderTargets[psr.renderTarget];

						if (bindState.psr.renderTarget != &rt)
						{
							bindState.psr.renderTarget = &rt;
							s.d3dContext->PSSetShaderResources(psr.slot, 1, rt.d3dRenderTargetResourceView.GetAddressOf());
						}
						break;
					}

					case ResourceType::DepthBuffer:
					{
						DepthBufferData& db = s.depthBuffers[psr.depthBuffer];

						if (bindState.psr.depthBuffer != &db)
						{
							bindState.psr.depthBuffer = &db;
							s.d3dContext->PSSetShaderResources(psr.slot, 1, db.d3dDepthBufferResourceView.GetAddressOf());
						}
						break;
					}
				}
				break;
			}

			case RenderCommandType::PopPixelShaderResource:
			{
				Assert(s.pixelShaderResourceStack.length != 0);
				BoundResource psr = List_Pop(s.pixelShaderResourceStack);

				switch (psr.type)
				{
					case ResourceType::Null: Assert(false); break;

					case ResourceType::RenderTarget:
					{
						RenderTargetData& rt = psr.renderTarget ? *psr.renderTarget : s.nullRenderTarget;
						if (bindState.psr.renderTarget != &rt)
						{
							bindState.psr.renderTarget = &rt;
							s.d3dContext->PSSetShaderResources(psr.slot, 1, rt.d3dRenderTargetResourceView.GetAddressOf());
						}
						break;
					}

					case ResourceType::DepthBuffer:
					{
						DepthBufferData& db = psr.depthBuffer ? *psr.depthBuffer : s.nullDepthBuffer;
						if (bindState.psr.depthBuffer != &db)
						{
							bindState.psr.depthBuffer = &db;
							s.d3dContext->PSSetShaderResources(psr.slot, 1, db.d3dDepthBufferResourceView.GetAddressOf());
						}
						break;
					}
				}
				break;
			}

			case RenderCommandType::Copy:
			{
				CopyResource&     copy   = renderCommand.copy;
				RenderTargetData& source = s.renderTargets[copy.source];
				CPUTextureData&   dest   = s.cpuTextures[copy.dest];
				s.d3dContext->CopyResource(dest.d3dCPUTexture.Get(), source.d3dRenderTarget.Get());
				break;
			}
		}
	}
	List_Clear(s.commandList);

	Assert(s.renderTargetStack.length == 0);
	Assert(s.depthBufferStack.length == 0);
	Assert(s.vertexShaderStack.length == 0);
	Assert(s.pixelShaderStack.length == 0);
	Assert(s.pixelShaderResourceStack.length == 0);

	// NOTE: Technically unnecessary, since the Map call will sync
	// NOTE: This doesn't actually guarantee rendering has occurred
	// NOTE: Flush unmaps resources
	s.d3dContext->Flush();

	for (u32 i = 0; i < s.cpuTextures.length; i++)
	{
		CPUTextureData& ct = s.cpuTextures[i];

		HRESULT hr = s.d3dContext->Map(ct.d3dCPUTexture.Get(), 0, D3D11_MAP_READ, 0, &ct.mappedResource);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Warning, "Failed to map CPU texture");
	}

	return true;
}
