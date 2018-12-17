// TODO: What happens if we call random renderer api and the device is
// disconnected? Presumably it can happen at anytime, not just during an update?

// TODO: Handle unloading assets (will require reference counting, I think)

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma warning(push, 0)

// For some GUID magic in the DirectX/DXGI headers. Must be included before them.
#include <InitGuid.h>

#ifdef DEBUG
	#define D3D_DEBUG_INFO
	#include <d3d11sdklayers.h>
	#include <dxgidebug.h>
	#include <dxgi1_3.h>
#endif

#include <d3d9.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DirectXMath.h"
#include "DirectXColors.h"
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
	Mesh        ref     = {};
	String      name    = {};
	u32         vOffset = 0;
	u32         iOffset = 0;
	u32         iCount  = 0;
	DXGI_FORMAT iFormat = DXGI_FORMAT_UNKNOWN;
};

// TODO: Name?
struct ConstantBufferData
{
	ConstantBufferDesc   desc;
	ComPtr<ID3D11Buffer> d3dConstantBuffer;
};

struct VertexShaderData
{
	VertexShader               ref;
	String                     name;
	List<ConstantBufferData>   constantBuffers;
	ComPtr<ID3D11VertexShader> d3dVertexShader;
	ComPtr<ID3D11InputLayout>  d3dInputLayout;
	D3D_PRIMITIVE_TOPOLOGY     d3dPrimitveTopology;
};

struct PixelShaderData
{
	PixelShader               ref;
	String                    name;
	List<ConstantBufferData>  constantBuffers;
	ComPtr<ID3D11PixelShader> d3dPixelShader;
};

enum struct RenderCommandType
{
	Null,
	ConstantBufferUpdate,
	DrawCall,
};

struct RenderCommand
{
	RenderCommandType type;
	union
	{
		ConstantBufferUpdate cBufUpdate;
		DrawCall             drawCall;
	};
};

struct RendererState
{
	ComPtr<ID3D11Device>           d3dDevice;
	ComPtr<ID3D11DeviceContext>    d3dContext;
	ComPtr<IDXGIFactory1>          dxgiFactory;

	ComPtr<ID3D11Texture2D>        d3dRenderTexture;
	ComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
	ComPtr<ID3D11DepthStencilView> d3dDepthBufferView;

	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateSolid;
	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateWireframe;

	ComPtr<ID3D11Buffer>           d3dVertexBuffer;
	ComPtr<ID3D11Buffer>           d3dIndexBuffer;

	ComPtr<IDirect3D9Ex>           d3d9;
	ComPtr<IDirect3DDevice9Ex>     d3d9Device;
	ComPtr<IDirect3DTexture9>      d3d9RenderTexture;
	ComPtr<IDirect3DSurface9>      d3d9RenderSurface0;

	v2u         renderSize;
	DXGI_FORMAT renderFormat;
	u32         multisampleCount;
	u32         qualityLevelCount;
	b32         isWireframeEnabled;

	List<VertexShaderData> vertexShaders;
	List<PixelShaderData>  pixelShaders;
	List<MeshData>         meshes;
	List<Vertex>           vertexBuffer;
	List<u32>              indexBuffer;
	List<RenderCommand>    commandList;
};

template<typename T>
static inline void
SetDebugObjectName(const ComPtr<T>& resource, Slice<c8> name)
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.length - 1, name.data);
	#endif
}

template<typename T, typename... Args>
static inline void
SetDebugObjectName(const ComPtr<T>& resource, Slice<c8> format, Args... args)
{
	#if defined(DEBUG)
	String name = {};
	defer { List_Free(name); };

	b32 success = String_Format(name, (c8*) format.data, args...);
	if (!success)
	{
		LOG(Severity::Warning, "Failed to format D3D object name");
		SetDebugObjectName(resource, format);
		return;
	}

	// HACK
	SetDebugObjectName(resource, { name.length, name.data });
	#endif
}

static void UpdateRasterizerState(RendererState* s);

b32
Renderer_Initialize(RendererState* s, v2u renderSize)
{
	Assert(renderSize.x < i32Max && renderSize.y < i32Max);

	s->multisampleCount = 1;

	// Create device
	{
		HRESULT hr;

		UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
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
			&s->d3dDevice,
			&featureLevel,
			&s->d3dContext
		);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create D3D device");
		SetDebugObjectName(s->d3dDevice,  "Device");
		SetDebugObjectName(s->d3dContext, "Device Context");

		// Check feature level
		if (!HAS_FLAG(featureLevel, D3D_FEATURE_LEVEL_11_0))
		{
			LOG(Severity::Fatal, "Created Device does not support D3D 11");
			return false;
		}

		// Obtain the DXGI factory used to create the current device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		hr = s->d3dDevice.As(&dxgiDevice);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI device");
		// NOTE: It looks like the IDXGIDevice is actually the same object as
		// the ID3D11Device. Using SetPrivateData to set its name clobbers the
		// D3D device name and outputs a warning.

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI adapter");
		SetDebugObjectName(dxgiAdapter, "DXGI Adapter");

		// TODO: Only needed for the swap chain
		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&s->dxgiFactory));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI factory");
		SetDebugObjectName(s->dxgiFactory, "DXGI Factory");

		// Check for the WARP driver
		DXGI_ADAPTER_DESC desc = {};
		hr = dxgiAdapter->GetDesc(&desc);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI adapter description");

		if ((desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c))
		{
			// WARNING: Microsoft Basic Render Driver is active. Performance of this
			// application may be unsatisfactory. Please ensure that your video card is
			// Direct3D10/11 capable and has the appropriate driver installed.
			LOG(Severity::Warning, "WARP driver in use.");
		}
	}


	// Configure debugging
	{
		HRESULT hr;

		// TODO: Maybe replace DEBUG with something app specific
		#ifdef DEBUG
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
		#endif
	}


	// Create render texture
	D3D11_TEXTURE2D_DESC renderTextureDesc = {};
	{
		// Create texture
		{
			HRESULT hr;

			// Query and set MSAA quality levels
			hr = s->d3dDevice->CheckMultisampleQualityLevels(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				s->multisampleCount,
				&s->qualityLevelCount
			);
			LOG_HRESULT_IF_FAILED(hr, return false,
				Severity::Fatal, "Failed to query supported multisample levels");

			renderTextureDesc.Width              = renderSize.x;
			renderTextureDesc.Height             = renderSize.y;
			renderTextureDesc.MipLevels          = 1;
			renderTextureDesc.ArraySize          = 1;
			// TODO: Switch to DXGI_FORMAT_B8G8R8A8_UNORM_SRGB for linear rendering
			// DirectXMath colors are defined in sRGB colorspace. Update the clear:
			// color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
			// context->ClearRenderTargetView(renderTarget, color);
			// https://blog.molecular-matters.com/2011/11/21/gamma-correct-rendering/
			// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch24.html
			// http://filmicgames.com/archives/299
			renderTextureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
			renderTextureDesc.SampleDesc.Count   = s->multisampleCount;
			renderTextureDesc.SampleDesc.Quality = s->qualityLevelCount - 1;
			renderTextureDesc.Usage              = D3D11_USAGE_DEFAULT;
			renderTextureDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			renderTextureDesc.CPUAccessFlags     = 0;
			renderTextureDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

			hr = s->d3dDevice->CreateTexture2D(&renderTextureDesc, nullptr, &s->d3dRenderTexture);
			LOG_HRESULT_IF_FAILED(hr, return false,
				Severity::Fatal, "Failed to create render texture");
			SetDebugObjectName(s->d3dRenderTexture, "Render Texture");

			hr = s->d3dDevice->CreateRenderTargetView(s->d3dRenderTexture.Get(), nullptr, &s->d3dRenderTargetView);
			LOG_HRESULT_IF_FAILED(hr, return false,
				Severity::Fatal, "Failed to create render target view");
			SetDebugObjectName(s->d3dRenderTargetView, "Render Target View");

			s->renderSize   = renderSize;
			s->renderFormat = renderTextureDesc.Format;
		}


		// Create depth buffer
		{
			HRESULT hr;

			D3D11_TEXTURE2D_DESC depthDesc = {};
			depthDesc.Width              = s->renderSize.x;
			depthDesc.Height             = s->renderSize.y;
			depthDesc.MipLevels          = 1;
			depthDesc.ArraySize          = 1;
			depthDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthDesc.SampleDesc.Count   = s->multisampleCount;
			depthDesc.SampleDesc.Quality = s->qualityLevelCount - 1;
			depthDesc.Usage              = D3D11_USAGE_DEFAULT;
			depthDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
			depthDesc.CPUAccessFlags     = 0;
			depthDesc.MiscFlags          = 0;

			ComPtr<ID3D11Texture2D> d3dDepthBuffer;
			hr = s->d3dDevice->CreateTexture2D(&depthDesc, nullptr, &d3dDepthBuffer);
			LOG_HRESULT_IF_FAILED(hr, return false,
				Severity::Fatal, "Failed to create depth buffer");
			SetDebugObjectName(d3dDepthBuffer, "Depth Buffer");

			hr = s->d3dDevice->CreateDepthStencilView(d3dDepthBuffer.Get(), nullptr, &s->d3dDepthBufferView);
			LOG_HRESULT_IF_FAILED(hr, return false,
				Severity::Fatal, "Failed to create depth buffer view");
			SetDebugObjectName(s->d3dDepthBufferView, "Depth Buffer View");
		}


		// Initialize output merger
		s->d3dContext->OMSetRenderTargets(1, s->d3dRenderTargetView.GetAddressOf(), s->d3dDepthBufferView.Get());


		// Initialize viewport
		{
			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width    = (r32) s->renderSize.x;
			viewport.Height   = (r32) s->renderSize.y;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1;

			s->d3dContext->RSSetViewports(1, &viewport);
		}
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
		const b32 useQuadrilateralLineAA = true;

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
		rasterizerDesc.MultisampleEnable     = s->multisampleCount > 1 && useQuadrilateralLineAA;
		rasterizerDesc.AntialiasedLineEnable = s->multisampleCount > 1;

		hr = s->d3dDevice->CreateRasterizerState(&rasterizerDesc, &s->d3dRasterizerStateSolid);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create solid rasterizer state");
		SetDebugObjectName(s->d3dRasterizerStateSolid, "Rasterizer State (Solid)");

		// Wireframe
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;

		hr = s->d3dDevice->CreateRasterizerState(&rasterizerDesc, &s->d3dRasterizerStateWireframe);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create wireframe rasterizer state");
		SetDebugObjectName(s->d3dRasterizerStateWireframe, "Rasterizer State (Wireframe)");

		// Start off in correct state
		UpdateRasterizerState(s);
	}

	return true;
}

b32
Renderer_RebuildSharedGeometryBuffers(RendererState*s)
{
	// Finalize vertex buffer
	{
		D3D11_BUFFER_DESC vertBuffDesc = {};
		vertBuffDesc.ByteWidth           = List_SizeOf(s->vertexBuffer);
		vertBuffDesc.Usage               = D3D11_USAGE_DYNAMIC;
		vertBuffDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
		vertBuffDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
		vertBuffDesc.MiscFlags           = 0;
		vertBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vertBuffInitData = {};
		vertBuffInitData.pSysMem          = s->vertexBuffer.data;
		vertBuffInitData.SysMemPitch      = 0;
		vertBuffInitData.SysMemSlicePitch = 0;

		s->d3dVertexBuffer.Reset();
		HRESULT hr = s->d3dDevice->CreateBuffer(&vertBuffDesc, &vertBuffInitData, &s->d3dVertexBuffer);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create shared vertex buffer");
		SetDebugObjectName(s->d3dVertexBuffer, "Shared Vertex Buffer");
	}

	// Finalize index buffer
	{
		D3D11_BUFFER_DESC indexBuffDesc = {};
		indexBuffDesc.ByteWidth           = List_SizeOf(s->indexBuffer);
		indexBuffDesc.Usage               = D3D11_USAGE_IMMUTABLE;
		indexBuffDesc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
		indexBuffDesc.CPUAccessFlags      = 0;
		indexBuffDesc.MiscFlags           = 0;
		indexBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexBuffInitData = {};
		indexBuffInitData.pSysMem          = s->indexBuffer.data;
		indexBuffInitData.SysMemPitch      = 0;
		indexBuffInitData.SysMemSlicePitch = 0;

		s->d3dIndexBuffer.Reset();
		HRESULT hr = s->d3dDevice->CreateBuffer(&indexBuffDesc, &indexBuffInitData, &s->d3dIndexBuffer);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create shared index buffer");
		SetDebugObjectName(s->d3dIndexBuffer, "Shared Index Buffer");
	}

	return true;
}

static void
UpdateRasterizerState(RendererState* s)
{
	if (s->isWireframeEnabled)
	{
		s->d3dContext->RSSetState(s->d3dRasterizerStateWireframe.Get());
	}
	else
	{
		s->d3dContext->RSSetState(s->d3dRasterizerStateSolid.Get());
	}
}

void
Renderer_Teardown(RendererState* s)
{
	List_Free(s->commandList);
	List_Free(s->indexBuffer);
	List_Free(s->vertexBuffer);

	for (u32 i = 0; i < s->meshes.length; i++)
	{
		MeshData* mesh = &s->meshes[i];
		List_Free(mesh->name);
	}
	List_Free(s->meshes);

	for (u32 i = 0; i < s->pixelShaders.length; i++)
	{
		PixelShaderData* ps = &s->pixelShaders[i];

		ps->d3dPixelShader.Reset();
		for (u32 j = 0; j < ps->constantBuffers.length; j++)
			ps->constantBuffers[j].d3dConstantBuffer.Reset();
		List_Free(ps->constantBuffers);
		List_Free(ps->name);
	}
	List_Free(s->pixelShaders);

	for (u32 i = 0; i < s->vertexShaders.length; i++)
	{
		VertexShaderData* vs = &s->vertexShaders[i];

		vs->d3dVertexShader.Reset();
		vs->d3dInputLayout.Reset();
		for (u32 j = 0; j < vs->constantBuffers.length; j++)
			vs->constantBuffers[j].d3dConstantBuffer.Reset();
		List_Free(vs->constantBuffers);
		List_Free(vs->name);
	}
	List_Free(s->vertexShaders);

	s->d3dIndexBuffer             .Reset();
	s->d3dVertexBuffer            .Reset();
	s->d3dRasterizerStateWireframe.Reset();
	s->d3dRasterizerStateSolid    .Reset();
	s->d3dDepthBufferView         .Reset();
	s->d3dRenderTargetView        .Reset();
	s->d3dRenderTexture           .Reset();
	s->dxgiFactory                .Reset();
	s->d3dContext                 .Reset();
	s->d3dDevice                  .Reset();

	// Log live objects
	{
		#ifdef DEBUG
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

// TODO: Standardize asset function naming convention
Mesh
Renderer_CreateMesh(RendererState* s, Slice<c8> name, Slice<Vertex> vertices, Slice<Index> indices)
{
	MeshData mesh = {};
	defer {
		// TODO: Can end up with orphaned vertices/indices allocated
		List_Free(mesh.name);
	};

	// Copy Name
	{
		b32 success = List_Duplicate(name, mesh.name);
		LOG_IF(!success, IGNORE,
			Severity::Warning, "Failed to copy mesh name '%s'", name.data);
	}

	// Copy Data
	{
		b32 success;

		mesh.vOffset = s->vertexBuffer.length;
		mesh.iOffset = s->indexBuffer.length;
		mesh.iCount  = indices.length;
		mesh.iFormat = DXGI_FORMAT_R32_UINT;

		// TODO: Passing a Slice<c8> to a log format WILL NOT work! (pointer is
		// not at the beginning of the struct, and more importantly slices are
		// not guaranteed to be null terminated!)
		success = List_AppendRange(s->vertexBuffer, vertices);
		LOG_IF(!success, return Mesh::Null,
			Severity::Error, "Failed to allocate space for mesh vertices '%s'", name.data);

		success = List_AppendRange(s->indexBuffer, indices);
		LOG_IF(!success, return Mesh::Null,
			Severity::Error, "Failed to allocate space for mesh indices '%s'", name.data);
	}

	// Commit
	{
		MeshData* mesh2 = List_Append(s->meshes, mesh);
		LOG_IF(!mesh2, return Mesh::Null,
			Severity::Error, "Failed to allocate space for mesh '%s'", name.data);
		mesh = {};

		// TODO: Eventually lists will reuse slots
		mesh2->ref = List_GetLast(s->meshes);
	}

	return List_GetLast(s->meshes);
}

static b32
Renderer_CreateConstantBuffer(RendererState* s, ConstantBufferData* cBuf)
{
	// TODO: Add more cbuf info to logging
	// TODO: Decide where validation should be handled: simulation or renderer

	LOG_IF(!IsMultipleOf(cBuf->desc.size, 16), return false,
		Severity::Error, "Constant buffer size '%u' is not a multiple of 16", cBuf->desc.size);

	D3D11_BUFFER_DESC d3dDesc = {};
	d3dDesc.ByteWidth           = cBuf->desc.size;
	d3dDesc.Usage               = D3D11_USAGE_DYNAMIC;
	d3dDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
	d3dDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	d3dDesc.MiscFlags           = 0;
	d3dDesc.StructureByteStride = 0;

	HRESULT hr = s->d3dDevice->CreateBuffer(&d3dDesc, nullptr, &cBuf->d3dConstantBuffer);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create constant buffer");
	// TODO: Pass in shader type and buffer name for better errors?
	SetDebugObjectName(cBuf->d3dConstantBuffer, "Constant Buffer");

	return true;
}

// TODO: Should a failed load mark the file as bad?
VertexShader
Renderer_LoadVertexShader(RendererState* s, Slice<c8> name, c8* path, Slice<VertexAttribute> attributes, Slice<ConstantBufferDesc> cBufDescs)
{
	// Vertex Shader
	VertexShaderData vs = {};
	defer {
		vs.d3dVertexShader.Reset();
		vs.d3dInputLayout.Reset();

		for (u32 i = 0; i < vs.constantBuffers.length; i++)
			vs.constantBuffers[i].d3dConstantBuffer.Reset();
		List_Free(vs.constantBuffers);
		List_Free(vs.name);
	};

	// Copy Name
	{
		b32 success = List_Duplicate(name, vs.name);
		LOG_IF(!success, IGNORE,
			Severity::Warning, "Failed to copy vertex shader name '%s'", path);
	}

	// Load
	Bytes vsBytes = {};
	defer { List_Free(vsBytes); };
	{
		vsBytes = Platform_LoadFileBytes(path);
		if (!vsBytes.data) return VertexShader::Null;
	}

	// Create
	{
		HRESULT hr = s->d3dDevice->CreateVertexShader(vsBytes.data, vsBytes.length, nullptr, &vs.d3dVertexShader);
		LOG_HRESULT_IF_FAILED(hr, return VertexShader::Null,
			Severity::Error, "Failed to create vertex shader '%s'", path);
		SetDebugObjectName(vs.d3dVertexShader, "Vertex Shader: %s", name.data);
	}

	// Constant Buffers
	if (cBufDescs.length)
	{
		b32 success;

		success = List_Reserve(vs.constantBuffers, cBufDescs.length);
		LOG_IF(!success, return VertexShader::Null,
			Severity::Error, "Failed to allocate space for VS constant buffers '%s'", path);

		for (u32 i = 0; i < cBufDescs.length; i++)
		{
			ConstantBufferData* cBuf = List_Append(vs.constantBuffers);
			cBuf->desc = cBufDescs[i];

			success = Renderer_CreateConstantBuffer(s, cBuf);
			LOG_IF(!success, return VertexShader::Null,
				Severity::Error, "Failed to create VS constant buffer %u for '%s'", i, path);
		}
	}

	// Input layout
	{
		List<D3D11_INPUT_ELEMENT_DESC> vsInputDescs = {};
		b32 success = List_Reserve(vsInputDescs, attributes.length);
		LOG_IF(!success, return VertexShader::Null,
			Severity::Error, "Failed to allocate VS input descriptions '%s'", path);
		for (u32 i = 0; i < attributes.length; i++)
		{
			const c8* semantic;
			switch (attributes[i].semantic)
			{
				case VertexAttributeSemantic::Position: semantic = HLSLSemantic::Position; break;
				case VertexAttributeSemantic::Color:    semantic = HLSLSemantic::Color;    break;
				case VertexAttributeSemantic::TexCoord: semantic = HLSLSemantic::TexCoord; break;

				default:
					LOG(Severity::Error, "Unrecognized VS attribute semantic %i '%s'", attributes[i].semantic, path);
					return VertexShader::Null;
			}

			DXGI_FORMAT format;
			switch (attributes[i].format)
			{
				case VertexAttributeFormat::v2: format = DXGI_FORMAT_R32G32_FLOAT;       break;
				case VertexAttributeFormat::v3: format = DXGI_FORMAT_R32G32B32_FLOAT;    break;
				case VertexAttributeFormat::v4: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;

				default:
					LOG(Severity::Error, "Unrecognized VS attribute format %i '%s'", attributes[i].format, path);
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

		HRESULT hr = s->d3dDevice->CreateInputLayout(vsInputDescs.data, vsInputDescs.length, vsBytes.data, vsBytes.length, &vs.d3dInputLayout);
		LOG_HRESULT_IF_FAILED(hr, return VertexShader::Null,
			Severity::Error, "Failed to create VS input layout '%s'", path);
		SetDebugObjectName(vs.d3dInputLayout, "Input Layout: %s", name.data);

		vs.d3dPrimitveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	// Commit
	// TODO: Maybe it'd be better to add and defer remove instead?
	VertexShaderData* vs2 = nullptr;
	{
		vs2 = List_Append(s->vertexShaders, vs);
		LOG_IF(!vs2, return VertexShader::Null,
			Severity::Error, "Failed to allocate space for vertex shader '%s'", path);
		vs = {};

		// TODO: Eventually lists will reuse slots
		vs2->ref = List_GetLast(s->vertexShaders);
	}

	return vs2->ref;
}

// TODO: Unload functions
PixelShader
Renderer_LoadPixelShader(RendererState* s, Slice<c8> name, c8* path, Slice<ConstantBufferDesc> cBufDescs)
{
	PixelShaderData ps = {};
	defer {
		ps.d3dPixelShader.Reset();

		for (u32 i = 0; i < ps.constantBuffers.length; i++)
			ps.constantBuffers[i].d3dConstantBuffer.Reset();
		List_Free(ps.constantBuffers);
		List_Free(ps.name);
	};

	// Copy Name
	{
		b32 success = List_Duplicate(name, ps.name);
		LOG_IF(!success, IGNORE,
			Severity::Warning, "Failed to copy pixel shader name '%s'", path);
	}

	// Load
	Bytes psBytes = {};
	defer { List_Free(psBytes); };
	{
		psBytes = Platform_LoadFileBytes(path);
		if (!psBytes.data) return PixelShader::Null;
	}

	// Create
	{
		HRESULT hr = s->d3dDevice->CreatePixelShader(psBytes.data, (u64) psBytes.length, nullptr, &ps.d3dPixelShader);
		LOG_HRESULT_IF_FAILED(hr, return PixelShader::Null,
			Severity::Error, "Failed to create pixel shader '%s'", path);
		SetDebugObjectName(ps.d3dPixelShader, "Pixel Shader: %s", name.data);
	}

	// Constant Buffers
	if (cBufDescs.length)
	{
		b32 success;

		success = List_Reserve(ps.constantBuffers, cBufDescs.length);
		LOG_IF(!success, return PixelShader::Null,
			Severity::Error, "Failed to allocate space for PS constant buffers '%s'", path);

		for (u32 i = 0; i < cBufDescs.length; i++)
		{
			ConstantBufferData* cBuf = List_Append(ps.constantBuffers);
			cBuf->desc = cBufDescs[i];

			success = Renderer_CreateConstantBuffer(s, cBuf);
			LOG_IF(!success, return PixelShader::Null,
				Severity::Error, "Failed to create PS constant buffer %u for '%s'", i, path);
		}
	}

	// Commit
	PixelShaderData* ps2 = nullptr;
	{
		ps2 = List_Append(s->pixelShaders, ps);
		LOG_IF(!ps2, return PixelShader::Null,
			Severity::Error, "Failed to allocate space for pixel shader '%s'", path);
		ps = {};

		// TODO: Eventually lists will reuse slots
		ps2->ref = List_GetLast(s->pixelShaders);
	}

	return ps2->ref;
}

Material
Renderer_CreateMaterial(RendererState* s, Mesh mesh, VertexShader vs, PixelShader ps)
{
	UNUSED(s);

	Material material = {};
	material.mesh = mesh;
	material.vs   = vs;
	material.ps   = ps;
	return material;
}

ConstantBufferUpdate*
Renderer_PushConstantBufferUpdate(RendererState* s)
{
	RenderCommand* renderCommand = List_Append(s->commandList);
	renderCommand->type = RenderCommandType::ConstantBufferUpdate;
	return &renderCommand->cBufUpdate;
}

DrawCall*
Renderer_PushDrawCall(RendererState* s)
{
	RenderCommand* renderCommand = List_Append(s->commandList);
	renderCommand->type = RenderCommandType::DrawCall;
	return &renderCommand->drawCall;
}

static b32
Renderer_UpdateConstantBuffer(RendererState* s, ConstantBufferData* cBuf, void* data)
{
	D3D11_MAPPED_SUBRESOURCE map = {};
	HRESULT hr = s->d3dContext->Map(cBuf->d3dConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to map constant buffer");

	memcpy(map.pData, data, cBuf->desc.size);
	s->d3dContext->Unmap(cBuf->d3dConstantBuffer.Get(), 0);

	return true;
}

b32
Renderer_Render(RendererState* s)
{
	s->d3dContext->ClearRenderTargetView(s->d3dRenderTargetView.Get(), DirectX::Colors::Black);
	s->d3dContext->ClearDepthStencilView(s->d3dDepthBufferView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	VertexShader lastVS = VertexShader::Null;
	 PixelShader lastPS =  PixelShader::Null;

	// OPTIMIZE: Sort draws?
	// OPTIMIZE: Adding a "SetMaterial" command would remove some redundant shader lookups.

	for (u32 i = 0; i < s->commandList.length; i++)
	{
		RenderCommand* renderCommand = &s->commandList[i];
		switch (renderCommand->type)
		{
			default: Assert(false); continue;

			case RenderCommandType::ConstantBufferUpdate:
			{
				ConstantBufferUpdate* cBufUpdate = &renderCommand->cBufUpdate;

				// TODO: Do validation in API call (ps & vs refs, shader stage, etc)
				// TODO: If we fail to update a constant buffer do we skip drawing?
				ConstantBufferData* cBuf = nullptr;
				switch (cBufUpdate->shaderStage)
				{
					default: Assert(false); continue;

					case ShaderStage::Vertex:
					{
						VertexShaderData* vs = &s->vertexShaders[cBufUpdate->material.vs];
						cBuf = &vs->constantBuffers[cBufUpdate->index];
						break;
					}

					case ShaderStage::Pixel:
					{
						PixelShaderData* ps = &s->pixelShaders[cBufUpdate->material.ps];
						cBuf = &ps->constantBuffers[cBufUpdate->index];
						break;
					}
				}

				b32 success = Renderer_UpdateConstantBuffer(s, cBuf, cBufUpdate->data);
				if (!success) break;
				break;
			}

			case RenderCommandType::DrawCall:
			{
				DrawCall*         dc   = &renderCommand->drawCall;
				MeshData*         mesh = &s->meshes[dc->material.mesh];
				VertexShaderData* vs   = &s->vertexShaders[dc->material.vs];
				PixelShaderData*  ps   = &s->pixelShaders[dc->material.ps];

				// Vertex Shader
				if (vs->ref != lastVS)
				{
					lastVS = vs->ref;

					size vStride = sizeof(Vertex);
					size vOffset = 0;

					s->d3dContext->VSSetShader(vs->d3dVertexShader.Get(), nullptr, 0);
					s->d3dContext->IASetInputLayout(vs->d3dInputLayout.Get());
					s->d3dContext->IASetVertexBuffers(0, 1, s->d3dVertexBuffer.GetAddressOf(), &vStride, &vOffset);
					s->d3dContext->IASetIndexBuffer(s->d3dIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
					s->d3dContext->IASetPrimitiveTopology(vs->d3dPrimitveTopology);

					for (u32 j = 0; j < vs->constantBuffers.length; j++)
					{
						ConstantBufferData* cBuf = &vs->constantBuffers[j];
						s->d3dContext->VSSetConstantBuffers(j, 1, cBuf->d3dConstantBuffer.GetAddressOf());
					}
				}

				// Pixel Shader
				if (ps->ref != lastPS)
				{
					lastPS = ps->ref;

					s->d3dContext->PSSetShader(ps->d3dPixelShader.Get(), nullptr, 0);

					for (u32 j = 0; j < ps->constantBuffers.length; j++)
					{
						ConstantBufferData* cBuf = &ps->constantBuffers[j];
						s->d3dContext->PSSetConstantBuffers(j, 1, cBuf->d3dConstantBuffer.GetAddressOf());
					}
				}

				// TODO: Leave constant buffers bound?

				Assert(mesh->vOffset < i32Max);
				s->d3dContext->DrawIndexed(mesh->iCount, mesh->iOffset, (i32) mesh->vOffset);
				break;
			}
		}
	}
	List_Clear(s->commandList);

	// TODO: Why is this here again? I think it's for the WPF app
	//s->d3dContext->Flush();

	return true;
}

static b32
Renderer_CreateSharedD3D9RenderTexture(RendererState* s)
{
	HRESULT hr;

	// Device
	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &s->d3d9);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to initialize D3D9");

	D3DPRESENT_PARAMETERS presentParams = {};
	presentParams.Windowed             = true;
	presentParams.SwapEffect           = D3DSWAPEFFECT_DISCARD;
	presentParams.hDeviceWindow        = nullptr;
	presentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	hr = s->d3d9->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		GetDesktopWindow(), // TODO: Ensure this is ok
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
		&presentParams,
		nullptr,
		&s->d3d9Device
	);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create D3D9 device");


	// Shared surface
	ComPtr<IDXGIResource> dxgiRenderTexture;
	hr = s->d3dRenderTexture.As(&dxgiRenderTexture);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to get DXGI render texture");

	HANDLE renderTextureSharedHandle;
	hr = dxgiRenderTexture->GetSharedHandle(&renderTextureSharedHandle);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to get DXGI render texture handle");

	Assert(s->renderFormat == DXGI_FORMAT_B8G8R8A8_UNORM);
	hr = s->d3d9Device->CreateTexture(
		s->renderSize.x,
		s->renderSize.y,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&s->d3d9RenderTexture,
		&renderTextureSharedHandle
	);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create D3D9 render texture");

	hr = s->d3d9RenderTexture->GetSurfaceLevel(0, &s->d3d9RenderSurface0);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to get D3D9 render surface");

	//SetBackBuffer(D3DResourceType::IDirect3DSurface9, IntPtr(pSurface));

	return true;
}

static void
Renderer_DestroySharedD3D9RenderTarget(RendererState* s)
{
	s->d3d9              .Reset();
	s->d3d9Device        .Reset();
	s->d3d9RenderTexture .Reset();
	s->d3d9RenderSurface0.Reset();
}
