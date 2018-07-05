// TODO: What happens if we call random renderer api and the device is
// disconnected? Presumably it can happen at anytime, not just during an update?

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib,  "D3D9.lib")
#pragma comment(lib,  "DXGI.lib")

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
#include <DirectXColors.h>
using namespace DirectX;

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#pragma warning(pop)

namespace HLSLSemantic
{
	const c8* Position = "POSITION";
	const c8* Color    = "COLOR";
}

// NOTE:
// - Assume there's only one set of shaders and buffers so we don't need to
//   know which ones to use.
// - Shaders and buffers can be shoved in a list in RendererState, referenced
//   here as raw pointers.
struct MeshData
{
	UINT        vStride = 0;
	UINT        vOffset = 0;
	UINT        iBase   = 0;
	UINT        iCount  = 0;
	DXGI_FORMAT iFormat = DXGI_FORMAT_UNKNOWN;
	XMFLOAT4X4  world   = {};
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

struct InputLayout
{
	List<VertexAttribute>     attibutes      = {};
	ComPtr<ID3D11InputLayout> d3dInputLayout = nullptr;
};

struct VertexShader
{
	String                     name;
	// TODO: Needs to be ref counted
	InputLayout*               inputLayout;
	// TODO: Better names?
	ComPtr<ID3D11VertexShader> d3dVertexShader;
	ComPtr<ID3D11Buffer>       d3dConstantBuffer;
};

struct RendererState
{
	ComPtr<ID3D11Device>           d3dDevice                   = nullptr;
	ComPtr<ID3D11DeviceContext>    d3dContext                  = nullptr;
	ComPtr<IDXGIFactory1>          dxgiFactory                 = nullptr;

	ComPtr<ID3D11Texture2D>        d3dRenderTexture            = nullptr;
	ComPtr<ID3D11RenderTargetView> d3dRenderTargetView         = nullptr;
	ComPtr<ID3D11DepthStencilView> d3dDepthBufferView          = nullptr;

	ComPtr<ID3D11PixelShader>      d3dPixelShader              = nullptr;
	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateSolid     = nullptr;
	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateWireframe = nullptr;

	ComPtr<IDirect3D9Ex>           d3d9                        = nullptr;
	ComPtr<IDirect3DDevice9Ex>     d3d9Device                  = nullptr;
	ComPtr<IDirect3DTexture9>      d3d9RenderTexture           = nullptr;
	ComPtr<IDirect3DSurface9>      d3d9RenderSurface0          = nullptr;

	List<VertexShader> vertexShaders       = {};
	List<InputLayout>  inputLayouts        = {};

	XMFLOAT4X4         proj                = {};
	V2i                renderSize          = {}; // TODO : Change this to unsigned
	DXGI_FORMAT        renderFormat        = DXGI_FORMAT_UNKNOWN;
	u32                multisampleCount    = 1;
	u32                qualityLevelCount   = 0;

	MeshData           meshes[Mesh::Count] = {};
	b32                isWireframeEnabled  = false;

	DrawCall           drawCalls[32]       = {};
	u32                drawCallCount       = 0;
};

// TODO: Asset loading system
// TODO: Initialize lists
// TODO: Set shaders before use
// TODO: Debug shader!
// TODO: We're not really handling errors cases. Going to end up partially
// initialized. Reference counting might be a robust solution?
// TODO: Handle unloading assets (will require reference counting, I think)
// TODO: This pointer will move
// TODO: Start passing Strings instead of c8*?

// TODO: Merge these?
template<u32 TNameLength>
static inline void
SetDebugObjectName(const ComPtr<ID3D11Device> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

template<u32 TNameLength>
static inline void
SetDebugObjectName(const ComPtr<ID3D11DeviceChild> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

template<u32 TNameLength>
static inline void
SetDebugObjectName(const ComPtr<IDXGIObject> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

// TODO: Should a failed load mark the file as bad?
static VertexShader*
LoadVertexShader(RendererState* s, c8* path, List<VertexAttribute> attributes, ConstantBufferDesc cBufDesc)
{
	HRESULT hr;

	// TODO: Hash names?
	// Use Existing
	for (u32 i = 0; i < s->vertexShaders.length; i++)
	{
		if (strcmp(s->vertexShaders[i].name, path) == 0)
			return &s->vertexShaders[i];
	}


	// TODO: Copy name
	// TODO: Find a way to include shader name in errors
	// Vertex Shader
	VertexShader vs = {};
	defer {
		vs.d3dConstantBuffer.Reset();
		vs.d3dVertexShader.Reset();
	};

	Bytes vsBytes = {};
	defer { List_Free(vsBytes); };
	{
		// Load
		vsBytes = Platform_LoadFileBytes(path);
		LOG_IF(!vsBytes, "Failed to load vertex shader file", Severity::Warning, return nullptr);

		// Create
		hr = s->d3dDevice->CreateVertexShader(vsBytes.data, vsBytes.length, nullptr, &vs.d3dVertexShader);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create vertex shader", Severity::Error, return nullptr);
		SetDebugObjectName(vs.d3dVertexShader, "Vertex Shader");

		// Per-object constant buffer
		D3D11_BUFFER_DESC vsConstBuffDes = {};
		vsConstBuffDes.ByteWidth           = cBufDesc.size;
		vsConstBuffDes.Usage               = D3D11_USAGE_DYNAMIC;
		vsConstBuffDes.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
		vsConstBuffDes.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
		vsConstBuffDes.MiscFlags           = 0;
		vsConstBuffDes.StructureByteStride = 0;

		hr = s->d3dDevice->CreateBuffer(&vsConstBuffDes, nullptr, &vs.d3dConstantBuffer);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create vertex buffer", Severity::Error, return nullptr);
		SetDebugObjectName(vs.d3dConstantBuffer, "VS Constant Buffer (Per-Object)");
	}


	// Input layout
	InputLayout il = {};
	defer {
		il.d3dInputLayout.Reset();
	};
	{
		// TODO: Why are we duplicating these?
		il.attibutes = List_Duplicate(attributes);
		LOG_IF(!il.attibutes, "Failed to allocate space for input layout attributes", Severity::Warning, return nullptr);

		List<D3D11_INPUT_ELEMENT_DESC> vsInputDescs = {};
		List_Reserve(vsInputDescs, attributes.length);
		LOG_IF(!vsInputDescs, "Input description allocation failed", Severity::Warning, return nullptr);
		for (u32 i = 0; i < attributes.length; i++)
		{
			const c8* semantic;
			switch (attributes[i].semantic)
			{
				case VertexAttributeSemantic::Position: semantic = HLSLSemantic::Position; break;
				case VertexAttributeSemantic::Color:    semantic = HLSLSemantic::Color;    break;

				default:
					// TODO: Include semantic value
					LOG_IF(true, "Unrecognized VS attribute semantic", Severity::Warning, return nullptr);
			}

			DXGI_FORMAT format;
			switch (attributes[i].format)
			{
				case VertexAttributeFormat::Float3: format = DXGI_FORMAT_R32G32B32_FLOAT;    break;
				case VertexAttributeFormat::Float4: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;

				default:
					// TODO: Include format value
					LOG_IF(true, "Unrecognized VS attribute format", Severity::Warning, return nullptr);
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

		hr = s->d3dDevice->CreateInputLayout(vsInputDescs.data, vsInputDescs.length, vsBytes.data, vsBytes.length, &il.d3dInputLayout);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create input layout", Severity::Error, return nullptr);
		SetDebugObjectName(il.d3dInputLayout, "Input Layout");
	}


	// Commit
	VertexShader* vs2 = nullptr;
	{
		InputLayout* il2 = List_Append(s->inputLayouts, il);
		LOG_IF(!il2, "Failed to allocate space for input layout", Severity::Warning, return nullptr);
		il = {};

		vs.inputLayout = il2;
		vs2 = List_Append(s->vertexShaders, vs);
		LOG_IF(!vs2, "Failed to allocate space for vertex shader", Severity::Warning, return nullptr);
		vs = {};
	}

	return vs2;
}

static void UpdateRasterizerState(RendererState* s);

b32
Renderer_Initialize(RendererState* s, V2i renderSize)
{
	// TODO: This assert style probably needs to be changed
	Assert(s->d3dDevice                   == nullptr);
	Assert(s->d3dContext                  == nullptr);
	Assert(s->dxgiFactory                 == nullptr);
	Assert(s->d3dRenderTexture            == nullptr);
	Assert(s->d3dRenderTargetView         == nullptr);
	Assert(s->d3dDepthBufferView          == nullptr);
	Assert(s->d3dPixelShader              == nullptr);
	Assert(s->d3dRasterizerStateSolid     == nullptr);
	Assert(s->d3dRasterizerStateWireframe == nullptr);
	Assert(renderSize.x < i32Max && renderSize.y < i32Max);

	// TODO : Move into local blocks
	HRESULT hr;

	// Create device
	{
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
		LOG_HRESULT_IF_FAILED(hr, "D3D11CreateDevice failed", Severity::Error, return false);
		SetDebugObjectName(s->d3dDevice,  "Device");
		SetDebugObjectName(s->d3dContext, "Device Context");

		// Check feature level
		if ((featureLevel & D3D_FEATURE_LEVEL_11_0) != D3D_FEATURE_LEVEL_11_0)
		{
			LOG("Created device does not support D3D 11", Severity::Error);
			return false;
		}

		// Obtain the DXGI factory used to create the current device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		hr = s->d3dDevice.As(&dxgiDevice);
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI device", Severity::Error, return false);
		// NOTE: It looks like the IDXGIDevice is actually the same object as
		// the ID3D11Device. Using SetPrivateData to set its name clobbers the
		// D3D device name and outputs a warning.

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI adapter", Severity::Error, return false);
		SetDebugObjectName(dxgiAdapter, "DXGI Adapter");

		// TODO: Only needed for the swap chain
		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&s->dxgiFactory));
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI factory", Severity::Error, return false);
		SetDebugObjectName(s->dxgiFactory, "DXGI Factory");

		// Check for the WARP driver
		DXGI_ADAPTER_DESC desc = {};
		hr = dxgiAdapter->GetDesc(&desc);
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI adapter description", Severity::Error, return false);

		if ((desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c))
		{
			// WARNING: Microsoft Basic Render Driver is active. Performance of this
			// application may be unsatisfactory. Please ensure that your video card is
			// Direct3D10/11 capable and has the appropriate driver installed.
			LOG("WARP driver in use.", Severity::Warning);
		}
	}


	// Configure debugging
	{
		// TODO: Maybe replace DEBUG with something app specific
		#ifdef DEBUG
		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI debug", Severity::Error, return false);

		dxgiDebug->EnableLeakTrackingForThread();

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue));
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI info queue", Severity::Error, return false);

		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR,      true);
		LOG_HRESULT_IF_FAILED(hr, "Failed to set DXGI break on error", Severity::Warning);
		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		LOG_HRESULT_IF_FAILED(hr, "Failed to set DXGI break on corruption", Severity::Warning);
		#endif
	}


	// Create render texture
	D3D11_TEXTURE2D_DESC renderTextureDesc = {};
	{
		// Create texture
		{
			// Query and set MSAA quality levels
			hr = s->d3dDevice->CheckMultisampleQualityLevels(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				s->multisampleCount,
				&s->qualityLevelCount
			);
			LOG_HRESULT_IF_FAILED(hr, "CheckMultisampleQualityLevels failed", Severity::Error, return false);

			renderTextureDesc.Width              = (u32) renderSize.x;
			renderTextureDesc.Height             = (u32) renderSize.y;
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
			LOG_HRESULT_IF_FAILED(hr, "Failed to create render texture", Severity::Error, return false);
			SetDebugObjectName(s->d3dRenderTexture, "Render Texture");

			hr = s->d3dDevice->CreateRenderTargetView(s->d3dRenderTexture.Get(), nullptr, &s->d3dRenderTargetView);
			LOG_HRESULT_IF_FAILED(hr, "Failed to create render target view", Severity::Error, return false);
			SetDebugObjectName(s->d3dRenderTargetView, "Render Target View");

			s->renderSize   = renderSize;
			s->renderFormat = renderTextureDesc.Format;
		}


		// Create depth buffer
		{
			D3D11_TEXTURE2D_DESC depthDesc = {};
			depthDesc.Width              = (u32) s->renderSize.x;
			depthDesc.Height             = (u32) s->renderSize.y;
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
			LOG_HRESULT_IF_FAILED(hr, "Failed to create depth buffer", Severity::Error, return false);
			SetDebugObjectName(d3dDepthBuffer, "Depth Buffer");

			hr = s->d3dDevice->CreateDepthStencilView(d3dDepthBuffer.Get(), nullptr, &s->d3dDepthBufferView);
			LOG_HRESULT_IF_FAILED(hr, "Failed to create depth buffer view", Severity::Error, return false);
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


		// Initialize projection matrix
		{
			XMMATRIX P = XMMatrixOrthographicLH((r32) s->renderSize.x, (r32) s->renderSize.y, 0, 1000);
			XMStoreFloat4x4(&s->proj, P);
		}
	}


	// Create vertex shader
	{
		List<VertexAttribute> vsAttributes = {};
		List_Reserve(vsAttributes, 2);
		LOG_IF(!vsAttributes, "Failed to allocate fallback vertex shader attributes", Severity::Error, return false);

		VertexAttribute va1 = { VertexAttributeSemantic::Position, VertexAttributeFormat::Float3 };
		VertexAttribute va2 = { VertexAttributeSemantic::Color,    VertexAttributeFormat::Float4 };
		List_Append(vsAttributes, va1);
		List_Append(vsAttributes, va2);
		LoadVertexShader(s, "Shaders/Basic Vertex Shader.cso", vsAttributes, { sizeof(XMFLOAT4X4) });

		// TODO: Move this
		s->d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}


	// Create pixel shader
	{
		// Load
		Bytes psBytes = Platform_LoadFileBytes("Shaders/Basic Pixel Shader.cso");
		LOG_IF(!psBytes, "Failed to load fallback pixel shader file", Severity::Error, return false);

		// Create
		hr = s->d3dDevice->CreatePixelShader(psBytes.data, (u64) psBytes.length, nullptr, &s->d3dPixelShader);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create fallback pixel shader", Severity::Error, return false);
		SetDebugObjectName(s->d3dPixelShader, "Pixel Shader");

		// Set
		s->d3dContext->PSSetShader(s->d3dPixelShader.Get(), nullptr, 0);
	}


	// Initialize rasterizer state
	{
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
		LOG_HRESULT_IF_FAILED(hr, "Failed to create solid rasterizer state", Severity::Error, return false);
		SetDebugObjectName(s->d3dRasterizerStateSolid, "Rasterizer State (Solid)");

		// Wireframe
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;

		hr = s->d3dDevice->CreateRasterizerState(&rasterizerDesc, &s->d3dRasterizerStateWireframe);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create wireframe rasterizer state", Severity::Error, return false);
		SetDebugObjectName(s->d3dRasterizerStateWireframe, "Rasterizer State (Wireframe)");

		// Start off in correct state
		UpdateRasterizerState(s);
	}


	// DEBUG: Create a draw call
	// TODO: This should be done in the application, not the renderer
	{
		// Create vertices
		Vertex vertices[4];

		// TODO: Real equilateral triangle
		vertices[0].position = XMFLOAT3(-.5f, -.5f, 0);
		vertices[1].position = XMFLOAT3(-.5f,  .5f, 0);
		vertices[2].position = XMFLOAT3( .5f,  .5f, 0);
		vertices[3].position = XMFLOAT3( .5f, -.5f, 0);

		XMStoreFloat4(&vertices[0].color, Colors::Red);
		XMStoreFloat4(&vertices[1].color, Colors::Green);
		XMStoreFloat4(&vertices[2].color, Colors::Blue);
		XMStoreFloat4(&vertices[3].color, Colors::White);

		// Create vertex buffer
		D3D11_BUFFER_DESC vertBuffDesc = {};
		vertBuffDesc.ByteWidth           = (u32) ArraySize(vertices);
		vertBuffDesc.Usage               = D3D11_USAGE_DYNAMIC;
		vertBuffDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
		vertBuffDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
		vertBuffDesc.MiscFlags           = 0;
		vertBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vertBuffInitData = {};
		vertBuffInitData.pSysMem          = vertices;
		vertBuffInitData.SysMemPitch      = 0;
		vertBuffInitData.SysMemSlicePitch = 0;

		MeshData* mesh = &s->meshes[Mesh::Quad];
		mesh->vStride = sizeof(vertices[0]);
		mesh->vOffset = 0;

		ComPtr<ID3D11Buffer> vBuffer;
		hr = s->d3dDevice->CreateBuffer(&vertBuffDesc, &vertBuffInitData, &vBuffer);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create quad vertex buffer", Severity::Error, return false);
		SetDebugObjectName(vBuffer, "Quad Vertices");

		// TODO: Move
		// Set
		s->d3dContext->IASetVertexBuffers(0, 1, vBuffer.GetAddressOf(), &mesh->vStride, &mesh->vOffset);

		// Create indices
		UINT indices[] = {
			0, 1, 2,
			2, 3, 0
		};

		// Create index buffer
		mesh->iBase   = 0;
		mesh->iCount  = ArrayLength(indices);
		mesh->iFormat = DXGI_FORMAT_R32_UINT;

		D3D11_BUFFER_DESC indexBuffDesc = {};
		indexBuffDesc.ByteWidth           = ArraySize(indices);
		indexBuffDesc.Usage               = D3D11_USAGE_IMMUTABLE;
		indexBuffDesc.BindFlags           = D3D11_BIND_INDEX_BUFFER;
		indexBuffDesc.CPUAccessFlags      = 0;
		indexBuffDesc.MiscFlags           = 0;
		indexBuffDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexBuffInitData = {};
		indexBuffInitData.pSysMem          = indices;
		indexBuffInitData.SysMemPitch      = 0;
		indexBuffInitData.SysMemSlicePitch = 0;

		ComPtr<ID3D11Buffer> iBuffer;
		hr = s->d3dDevice->CreateBuffer(&indexBuffDesc, &indexBuffInitData, &iBuffer);
		LOG_HRESULT_IF_FAILED(hr, "Failed to create quad index buffer", Severity::Error, return false);
		SetDebugObjectName(iBuffer, "Quad Indices");

		s->d3dContext->IASetIndexBuffer(iBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		// TODO: Remove
		// Draw call
		DrawCall* dc;
		dc = Renderer_PushDrawCall(s);
		if (dc == nullptr) return false;

		dc->mesh = Mesh::Quad;
		dc->vs   = &s->vertexShaders[0];
		XMStoreFloat4x4((XMFLOAT4X4*) &dc->worldM, XMMatrixScaling(160, 160, 160) * XMMatrixIdentity());
	}

	return true;
}

static void
UpdateRasterizerState(RendererState* s)
{
	Assert(s->d3dContext                  != nullptr);
	Assert(s->d3dRasterizerStateSolid     != nullptr);
	Assert(s->d3dRasterizerStateWireframe != nullptr);


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
	for (u32 i = 0; i < s->vertexShaders.length; i++)
	{
		s->vertexShaders[i].d3dVertexShader.Reset();
		s->vertexShaders[i].d3dConstantBuffer.Reset();
	}

	for (u32 i = 0; i < s->inputLayouts.length; i++)
		s->inputLayouts[i].d3dInputLayout.Reset();

	s->d3dDevice                  .Reset();
	s->d3dContext                 .Reset();
	s->dxgiFactory                .Reset();
	s->d3dRenderTexture           .Reset();
	s->d3dRenderTargetView        .Reset();
	s->d3dDepthBufferView         .Reset();
	s->d3dPixelShader             .Reset();
	s->d3dRasterizerStateSolid    .Reset();
	s->d3dRasterizerStateWireframe.Reset();


	// Log live objects
	{
		#ifdef DEBUG
		HRESULT hr;

		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI debug interface", Severity::Warning, return);

		// TODO: Test the differences in the output
		// DXGI_DEBUG_RLO_ALL
		// DXGI_DEBUG_RLO_SUMMARY
		// DXGI_DEBUG_RLO_DETAIL
		// NOTE: Only available with the Windows SDK installed. That's not
		// unituitive and doesn't lead to cryptic errors or anything. Fuck you,
		// Microsoft.
		// TODO: Re-enable this
		//hr = dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
		//LOG_HRESULT_IF_FAILED(hr, "ReportLiveObjects failed", Severity::Warning, return);

		OutputDebugStringA("\n");
		#endif
	}
}

DrawCall*
Renderer_PushDrawCall(RendererState* s)
{
	Assert(s->drawCallCount < ArrayLength(s->drawCalls));

	DrawCall* drawCall = &s->drawCalls[s->drawCallCount++];
	return drawCall;
}

b32
Renderer_Render(RendererState* s)
{
	// TODO: Should we assert things that will immediately crash the program
	// anyway (e.g. dereferenced below)? It probably gives us a better error
	// message.
	Assert(s->d3dContext       != nullptr);
	Assert(s->d3dRenderTexture != nullptr);


	HRESULT hr;

	s->d3dContext->ClearRenderTargetView(s->d3dRenderTargetView.Get(), DirectX::Colors::Black);
	s->d3dContext->ClearDepthStencilView(s->d3dDepthBufferView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	// Update Camera
	XMVECTOR pos    = {0, 0, 0};
	XMVECTOR target = {0, 0, 1};
	XMVECTOR up     = {0, 1, 0};

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);

	// TODO: Sort draws
	for (u32 i = 0; i < s->drawCallCount; i++)
	{
		DrawCall*     dc   = &s->drawCalls[i];
		MeshData*     mesh = &s->meshes[dc->mesh];
		VertexShader* vs   = dc->vs;

		XMMATRIX W = XMMATRIX((r32*) &dc->worldM);
		XMMATRIX P = XMLoadFloat4x4(&s->proj);
		XMMATRIX WVP = XMMatrixTranspose(W * V * P);

		// Set
		s->d3dContext->VSSetShader(vs->d3dVertexShader.Get(), nullptr, 0);
		s->d3dContext->VSSetConstantBuffers(0, 1, vs->d3dConstantBuffer.GetAddressOf());
		s->d3dContext->IASetInputLayout(vs->inputLayout->d3dInputLayout.Get());
		//s->d3dContext->IASetVertexBuffers(0, 1, vBuffer.GetAddressOf(), &mesh->vStride, &mesh->vOffset);

		// Update VS cbuffer
		D3D11_MAPPED_SUBRESOURCE vsConstBuffMap = {};
		hr = s->d3dContext->Map(vs->d3dConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vsConstBuffMap);
		LOG_HRESULT_IF_FAILED(hr, "Failed to map constant buffer", Severity::Error, return false);

		memcpy(vsConstBuffMap.pData, &WVP, sizeof(WVP));
		s->d3dContext->Unmap(vs->d3dConstantBuffer.Get(), 0);

		Assert(mesh->vOffset < i32Max);
		s->d3dContext->DrawIndexed(mesh->iCount, 0, (i32) mesh->vOffset);
	}
	//s->drawCallCount = 0;

	// TODO: Why is this here again? I think it's for the WPF app
	//s->d3dContext->Flush();

	return true;
}

b32
Renderer_CreateSharedD3D9RenderTexture(RendererState* s)
{
	Assert(s->d3d9                        == nullptr);
	Assert(s->d3d9Device                  == nullptr);
	Assert(s->d3d9RenderTexture           == nullptr);
	Assert(s->d3d9RenderSurface0          == nullptr);

	HRESULT hr;

	// Device
	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &s->d3d9);
	LOG_HRESULT_IF_FAILED(hr, "Failed to initialize D3D9", Severity::Error, return false);

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
	LOG_HRESULT_IF_FAILED(hr, "Failed to create D3D9 device", Severity::Error, return false);


	// Shared surface
	ComPtr<IDXGIResource> dxgiRenderTexture;
	hr = s->d3dRenderTexture.As(&dxgiRenderTexture);
	LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI render texture", Severity::Error, return false);

	HANDLE renderTextureSharedHandle;
	hr = dxgiRenderTexture->GetSharedHandle(&renderTextureSharedHandle);
	LOG_HRESULT_IF_FAILED(hr, "Failed to get DXGI render texture handle", Severity::Error, return false);

	Assert(s->renderFormat == DXGI_FORMAT_B8G8R8A8_UNORM);
	hr = s->d3d9Device->CreateTexture(
		(u32) s->renderSize.x,
		(u32) s->renderSize.y,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		&s->d3d9RenderTexture,
		&renderTextureSharedHandle
	);
	LOG_HRESULT_IF_FAILED(hr, "Failed to create D3D9 render texture", Severity::Error, return false);

	hr = s->d3d9RenderTexture->GetSurfaceLevel(0, &s->d3d9RenderSurface0);
	LOG_HRESULT_IF_FAILED(hr, "Failed to get D3D9 render surface", Severity::Error, return false);

	//SetBackBuffer(D3DResourceType::IDirect3DSurface9, IntPtr(pSurface));

	return true;
}

void Renderer_DestroySharedD3D9RenderTarget(RendererState* s)
{
	s->d3d9              .Reset();
	s->d3d9Device        .Reset();
	s->d3d9RenderTexture .Reset();
	s->d3d9RenderSurface0.Reset();
}
