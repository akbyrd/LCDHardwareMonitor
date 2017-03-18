#pragma comment(lib, "D3D11.lib")
#pragma comment(lib,  "D3D9.lib")
#pragma comment(lib,  "DXGI.lib")

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

namespace HLSLSemantic
{
	const c8* Position = "POSITION";
	const c8* Color    = "COLOR";
}

template<UINT TNameLength>
inline void
SetDebugObjectName(const ComPtr<ID3D11Device> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

template<UINT TNameLength>
inline void
SetDebugObjectName(const ComPtr<ID3D11DeviceChild> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

template<UINT TNameLength>
inline void
SetDebugObjectName(const ComPtr<IDXGIObject> &resource, const c8 (&name)[TNameLength])
{
	#if defined(DEBUG)
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
	#endif
}

// NOTES:
// - Assume there's only one set of shaders and buffers so we don't need to
//   know which ones to use.
// - Shaders and buffers can be shoved in a list in D3DRendererState, referenced
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

struct DrawCall
{
	Mesh mesh         = Mesh::Null;
	r32  worldM[4][4] = {};
};

struct D3DRendererState
{
	ComPtr<ID3D11Device>           d3dDevice                   = nullptr;
	ComPtr<ID3D11DeviceContext>    d3dContext                  = nullptr;
	ComPtr<IDXGIFactory1>          dxgiFactory                 = nullptr;

	ComPtr<ID3D11Texture2D>        d3dRenderTexture            = nullptr;
	ComPtr<ID3D11RenderTargetView> d3dRenderTargetView         = nullptr;
	//TODO: Do we need a depth buffer?
	ComPtr<ID3D11DepthStencilView> d3dDepthBufferView          = nullptr;

	ComPtr<ID3D11VertexShader>     d3dVertexShader             = nullptr;
	ComPtr<ID3D11Buffer>           d3dVSConstBuffer            = nullptr;
	ComPtr<ID3D11InputLayout>      d3dVSInputLayout            = nullptr;

	ComPtr<ID3D11PixelShader>      d3dPixelShader              = nullptr;
	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateSolid     = nullptr;
	ComPtr<ID3D11RasterizerState>  d3dRasterizerStateWireframe = nullptr;

	ComPtr<IDirect3D9Ex>           d3d9                        = nullptr;
	ComPtr<IDirect3DDevice9Ex>     d3d9Device                  = nullptr;
	ComPtr<IDirect3DTexture9>      d3d9RenderTexture           = nullptr;
	ComPtr<IDirect3DSurface9>      d3d9RenderSurface0          = nullptr;

	XMFLOAT4X4    proj                = {};
	V2i           renderSize          = {};
	u32           multisampleCount    = 1;
	u32           qualityLevelCount   = 0;

	MeshData      meshes[Mesh::Count] = {};
	b32           isWireframeEnabled  = false;

	DrawCall      drawCalls[32]       = {};
	u32           drawCallCount       = 0;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

#pragma region Foward Declarations
void UpdateRasterizeState(D3DRendererState*);
DrawCall* PushDrawCall(D3DRendererState*);
#pragma endregion

b32
InitializeRenderer(D3DRendererState* s, V2i renderSize)
{
	//TODO: Maybe asserts go with the usage site?
	Assert(s->d3dDevice                   == nullptr);
	Assert(s->d3dContext                  == nullptr);
	Assert(s->dxgiFactory                 == nullptr);
	Assert(s->d3dRenderTexture            == nullptr);
	Assert(s->d3dRenderTargetView         == nullptr);
	Assert(s->d3dDepthBufferView          == nullptr);
	Assert(s->d3dVertexShader             == nullptr);
	Assert(s->d3dVSConstBuffer            == nullptr);
	Assert(s->d3dVSInputLayout            == nullptr);
	Assert(s->d3dPixelShader              == nullptr);
	Assert(s->d3dRasterizerStateSolid     == nullptr);
	Assert(s->d3dRasterizerStateWireframe == nullptr);
	Assert(s->d3d9                        == nullptr);
	Assert(s->d3d9Device                  == nullptr);
	Assert(s->d3d9RenderTexture           == nullptr);
	Assert(s->d3d9RenderSurface0          == nullptr);


	HRESULT hr;
	b32 success;

	//Create device
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
			nullptr, 0, //NOTE: 11_1 will never be created by default
			D3D11_SDK_VERSION,
			&s->d3dDevice,
			&featureLevel,
			&s->d3dContext
		);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dDevice,  "Device");
		SetDebugObjectName(s->d3dContext, "Device Context");

		//Check feature level
		if ((featureLevel & D3D_FEATURE_LEVEL_11_0) != D3D_FEATURE_LEVEL_11_0)
		{
			LOG(L"Created device does not support D3D 11", Severity::Error);
			return false;
		}

		//Obtain the DXGI factory used to create the current device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		hr = s->d3dDevice.As(&dxgiDevice);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		// NOTE: It looks like the IDXGIDevice is actually the same object as
		// the ID3D11Device. Using SetPrivateData to set its name clobbers the
		// D3D device name and outputs a warning.

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(dxgiAdapter, "DXGI Adapter");

		//TODO: Only needed for the swap chain
		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&s->dxgiFactory));
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->dxgiFactory, "DXGI Factory");

		//Check for the WARP driver
		DXGI_ADAPTER_DESC desc = {};
		hr = dxgiAdapter->GetDesc(&desc);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		if ( (desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c) )
		{
			// WARNING: Microsoft Basic Render Driver is active. Performance of this
			// application may be unsatisfactory. Please ensure that your video card is
			// Direct3D10/11 capable and has the appropriate driver installed.
			LOG(L"WARP driver in use.", Severity::Warning);
		}
	}


	//Configure debugging
	{
		//TODO: Maybe replace DEBUG with something app specific
		#ifdef DEBUG
		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		dxgiDebug->EnableLeakTrackingForThread();

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue));
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR,      true);
		LOG_IF(FAILED(hr), L"", Severity::Warning);
		hr = dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		LOG_IF(FAILED(hr), L"", Severity::Warning);
		#endif
	}


	//Create render texture
	D3D11_TEXTURE2D_DESC renderTextureDesc = {};
	{
		//Create texture
		{
			//Query and set MSAA quality levels
			hr = s->d3dDevice->CheckMultisampleQualityLevels(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				s->multisampleCount,
				&s->qualityLevelCount
			);
			LOG_IF(FAILED(hr), L"", Severity::Error, return false);

			renderTextureDesc.Width              = renderSize.x;
			renderTextureDesc.Height             = renderSize.y;
			renderTextureDesc.MipLevels          = 1;
			renderTextureDesc.ArraySize          = 1;
			/* TODO: Switch to DXGI_FORMAT_B8G8R8A8_UNORM_SRGB for linear rendering
			 * DirectXMath colors are defined in sRGB colorspace. Update the clear:
			 * color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
			 * context->ClearRenderTargetView(renderTarget, color);
			 * https://blog.molecular-matters.com/2011/11/21/gamma-correct-rendering/
			 * http://http.developer.nvidia.com/GPUGems3/gpugems3_ch24.html
			 * http://filmicgames.com/archives/299
			 */
			renderTextureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
			renderTextureDesc.SampleDesc.Count   = s->multisampleCount;
			renderTextureDesc.SampleDesc.Quality = s->qualityLevelCount - 1;
			renderTextureDesc.Usage              = D3D11_USAGE_DEFAULT;
			renderTextureDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			renderTextureDesc.CPUAccessFlags     = 0;
			renderTextureDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

			hr = s->d3dDevice->CreateTexture2D(&renderTextureDesc, nullptr, &s->d3dRenderTexture);
			LOG_IF(FAILED(hr), L"", Severity::Error, return false);
			SetDebugObjectName(s->d3dRenderTexture, "Render Texture");

			hr = s->d3dDevice->CreateRenderTargetView(s->d3dRenderTexture.Get(), nullptr, &s->d3dRenderTargetView);
			LOG_IF(FAILED(hr), L"", Severity::Error, return false);
			SetDebugObjectName(s->d3dRenderTargetView, "Render Target View");

			s->renderSize = renderSize;
		}


		//Create depth buffer
		{
			//TODO: Use renderTextureDesc?
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
			LOG_IF(FAILED(hr), L"", Severity::Error, return false);
			SetDebugObjectName(d3dDepthBuffer, "Depth Buffer");

			hr = s->d3dDevice->CreateDepthStencilView(d3dDepthBuffer.Get(), nullptr, &s->d3dDepthBufferView);
			LOG_IF(FAILED(hr), L"", Severity::Error, return false);
			SetDebugObjectName(s->d3dDepthBufferView, "Depth Buffer View");
		}


		//Initialize output merger
		s->d3dContext->OMSetRenderTargets(1, s->d3dRenderTargetView.GetAddressOf(), s->d3dDepthBufferView.Get());


		//Initialize viewport
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


		//Initialize projection matrix
		{
			XMMATRIX P = XMMatrixOrthographicLH((r32) s->renderSize.x, (r32) s->renderSize.y, 0, 1000);
			XMStoreFloat4x4(&s->proj, P);
		}
	}


	//Create vertex shader
	{
		//Load
		unique_ptr<c8[]> vsBytes;
		size_t vsBytesLength;
		success = LoadFile(L"Basic Vertex Shader.cso", vsBytes, vsBytesLength);
		if (!success) return false;

		//Create
		hr = s->d3dDevice->CreateVertexShader(vsBytes.get(), vsBytesLength, nullptr, &s->d3dVertexShader);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dVertexShader, "Vertex Shader");

		//Set
		s->d3dContext->VSSetShader(s->d3dVertexShader.Get(), nullptr, 0);

		//Per-object constant buffer
		D3D11_BUFFER_DESC vsConstBuffDes = {};
		vsConstBuffDes.ByteWidth           = sizeof(XMFLOAT4X4);
		vsConstBuffDes.Usage               = D3D11_USAGE_DYNAMIC;
		vsConstBuffDes.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
		vsConstBuffDes.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
		vsConstBuffDes.MiscFlags           = 0;
		vsConstBuffDes.StructureByteStride = 0;

		hr = s->d3dDevice->CreateBuffer(&vsConstBuffDes, nullptr, &s->d3dVSConstBuffer);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dVSConstBuffer, "VS Constant Buffer (Per-Object)");

		//Input layout
		D3D11_INPUT_ELEMENT_DESC vsInputDescs[] = {
			{ HLSLSemantic::Position, 0, DXGI_FORMAT_R32G32B32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ HLSLSemantic::Color   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		hr = s->d3dDevice->CreateInputLayout(vsInputDescs, ArrayCount(vsInputDescs), vsBytes.get(), vsBytesLength, &s->d3dVSInputLayout);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dVSInputLayout, "Input Layout");

		s->d3dContext->IASetInputLayout(s->d3dVSInputLayout.Get());
		s->d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}


	//Create pixel shader
	{
		//Load
		unique_ptr<c8[]> psBytes;
		size_t psBytesLength;
		success = LoadFile(L"Basic Pixel Shader.cso", psBytes, psBytesLength);
		if (!success) return false;

		//Create
		hr = s->d3dDevice->CreatePixelShader(psBytes.get(), psBytesLength, nullptr, &s->d3dPixelShader);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dPixelShader, "Pixel Shader");

		//Set
		s->d3dContext->PSSetShader(s->d3dPixelShader.Get(), nullptr, 0);
	}


	//Initialize rasterizer state
	{
		// NOTE: MultisampleEnable toggles between quadrilateral AA (true) and alpha AA (false).
		// Alpha AA has a massive performance impact while quadrilateral is much smaller
		// (negligible for the mesh drawn here). Visually, it's hard to tell the difference between
		// quadrilateral AA on and off in this demo. Alpha AA on the other hand is more obvious. It
		// causes the wireframe to draw lines 2 px wide instead of 1.
		// 
		// See remarks: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
		const b32 useQuadrilateralLineAA = true;

		//Solid
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
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dRasterizerStateSolid, "Rasterizer State (Solid)");

		//Wireframe
		rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;

		hr = s->d3dDevice->CreateRasterizerState(&rasterizerDesc, &s->d3dRasterizerStateWireframe);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(s->d3dRasterizerStateWireframe, "Rasterizer State (Wireframe)");

		//Start off in correct state
		UpdateRasterizeState(s);
	}


	//DEBUG: Create a draw call
	{
		//Create vertices
		Vertex vertices[4];

		vertices[0].position = XMFLOAT3(-.5f * 160, -.5f * 160, 0);
		vertices[1].position = XMFLOAT3(-.5f * 160,  .5f * 160, 0);
		vertices[2].position = XMFLOAT3( .5f * 160,  .5f * 160, 0);
		vertices[3].position = XMFLOAT3( .5f * 160, -.5f * 160, 0);

		XMStoreFloat4(&vertices[0].color, Colors::Red);
		XMStoreFloat4(&vertices[1].color, Colors::Green);
		XMStoreFloat4(&vertices[2].color, Colors::Blue);
		XMStoreFloat4(&vertices[3].color, Colors::White);

		//Create vertex buffer
		D3D11_BUFFER_DESC vertBuffDesc = {};
		vertBuffDesc.ByteWidth           = ArraySize(vertices);
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
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(vBuffer, "Quad Vertices");

		s->d3dContext->IASetVertexBuffers(0, 1, vBuffer.GetAddressOf(), &mesh->vStride, &mesh->vOffset);
		s->d3dContext->VSSetConstantBuffers(0, 1, s->d3dVSConstBuffer.GetAddressOf());

		//Create indices
		UINT indices[] = {
			0, 1, 2,
			2, 3, 0
		};

		//Create index buffer
		mesh->iBase   = 0;
		mesh->iCount  = ArrayCount(indices);
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
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);
		SetDebugObjectName(iBuffer, "Quad Indices");

		s->d3dContext->IASetIndexBuffer(iBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		//TODO: Remove
		//Draw call
		DrawCall* dc;
		dc = PushDrawCall(s);
		if (dc == nullptr) return false;

		dc->mesh = Mesh::Quad;
		XMStoreFloat4x4((XMFLOAT4X4*) &dc->worldM, XMMatrixIdentity());
	}


	//Create D3D9 device and shared surface
	{
		//Device
		hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &s->d3d9);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		D3DPRESENT_PARAMETERS presentParams = {};
		presentParams.Windowed             = true;
		presentParams.SwapEffect           = D3DSWAPEFFECT_DISCARD;
		presentParams.hDeviceWindow        = nullptr;
		presentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		hr = s->d3d9->CreateDeviceEx(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			GetDesktopWindow(), //TODO: Ensure this is ok
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
			&presentParams,
			nullptr,
			&s->d3d9Device
		);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);


		//Shared surface
		ComPtr<IDXGIResource> dxgiRenderTexture;
		hr = s->d3dRenderTexture.As(&dxgiRenderTexture);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		HANDLE renderTextureSharedHandle;
		hr = dxgiRenderTexture->GetSharedHandle(&renderTextureSharedHandle);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		Assert(renderTextureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM);
		hr = s->d3d9Device->CreateTexture(
			renderTextureDesc.Width,
			renderTextureDesc.Height,
			renderTextureDesc.MipLevels,
			D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&s->d3d9RenderTexture,
			&renderTextureSharedHandle
		);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		hr = s->d3d9RenderTexture->GetSurfaceLevel(0, &s->d3d9RenderSurface0);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		//SetBackBuffer(D3DResourceType::IDirect3DSurface9, IntPtr(pSurface));
	}

	return true;
}

struct PreviewWindowState
{
	HWND                    hwnd                  = nullptr;
	ComPtr<IDXGISwapChain>  swapChain             = nullptr;
	ComPtr<ID3D11Texture2D> backBuffer            = nullptr;
	V2i                     renderSize            = {};
	V2i                     nonClientSize         = {};
	u16                     zoomFactor            = 1;
	i16                     mouseWheelAccumulator = 0;
};

b32
AttachPreviewWindow(D3DRendererState* s, PreviewWindowState* previewWindow)
{
	/* TODO: When using fullscreen, the display mode should be chosen by enumerating supported
	* modes. If a mode is chosen that isn't supported, a performance penalty will be incurred due
	* to Present performing a blit instead of a swap (does this apply to incorrect refresh rates
	* or only incorrect resolutions?).
	*/

	/* Set swap chain properties
	* 
	* NOTE:
	* If the DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH flag is used, the display mode that most
	* closely matches the back buffer will be used when entering fullscreen. If this happens to
	* be the same size as the back buffer, no WM_SIZE event is sent to the application (I'm only
	* assuming it *does* get sent if the size changes, I haven't tested it). If the flag is not
	* used, the display mode will be changed to match that of the desktop (usually the monitors
	* native display mode). This generally results in a WM_SIZE event (again, I'm only assuming
	* one will not be sent if the window happens to already be the same size as the desktop).
	* For now, I think it makes the most sense to use the native display mode when entering
	* fullscreen, so I'm removing the flag.
	* 
	* If we use a flip presentation swap chain, explicitly force destruction of any previous
	* swap chains before creating a new one.
	* https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
	*/
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width                   = s->renderSize.x;
	swapChainDesc.BufferDesc.Height                  = s->renderSize.y;
	//TODO: Get values from system (match desktop. what happens if it changes?)
	swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count                   = s->multisampleCount;
	swapChainDesc.SampleDesc.Quality                 = s->qualityLevelCount - 1;
	swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount                        = 1;
	swapChainDesc.OutputWindow                       = previewWindow->hwnd;
	swapChainDesc.Windowed                           = true;
	swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags                              = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//NOTE: IDXGISwapChain::ResizeTarget to resize window if render target changes

	//Create the swap chain
	HRESULT hr;
	hr = s->dxgiFactory->CreateSwapChain(s->d3dDevice.Get(), &swapChainDesc, &previewWindow->swapChain);
	LOG_IF(FAILED(hr), L"", Severity::Error, return false);
	SetDebugObjectName(previewWindow->swapChain, "Swap Chain");

	//Get the back buffer
	hr = previewWindow->swapChain->GetBuffer(0, IID_PPV_ARGS(&previewWindow->backBuffer));
	LOG_IF(FAILED(hr), L"", Severity::Error, return false);
	SetDebugObjectName(previewWindow->backBuffer, "Back Buffer");

	//Create a render target view to the back buffer
	//ComPtr<ID3D11RenderTargetView> renderTargetView;
	//hr = s->d3dDevice->CreateRenderTargetView(previewWindow->backBuffer.Get(), nullptr, &renderTargetView);
	//LOG_IF(FAILED(hr), L"", Severity::Error, return false);
	//SetDebugObjectName(renderTargetView, "Render Target View");

	//Associate the window
	hr = s->dxgiFactory->MakeWindowAssociation(previewWindow->hwnd, DXGI_MWA_NO_ALT_ENTER);
	LOG_IF(FAILED(hr), L"", Severity::Error, return false);

	return true;
}

b32
DetachPreviewWindow(D3DRendererState* s, PreviewWindowState* previewWindow)
{
	HRESULT hr;

	previewWindow->backBuffer.Reset();
	previewWindow->swapChain .Reset();

	hr = s->dxgiFactory->MakeWindowAssociation(previewWindow->hwnd, DXGI_MWA_NO_ALT_ENTER);
	LOG_IF(FAILED(hr), L"", Severity::Error, return false);

	return true;
}

void
UpdateRasterizeState(D3DRendererState* s)
{
	Assert(s->d3dContext                  != nullptr);
	Assert(s->d3dRasterizerStateSolid     != nullptr);
	Assert(s->d3dRasterizerStateWireframe != nullptr);


	if ( s->isWireframeEnabled )
	{
		s->d3dContext->RSSetState(s->d3dRasterizerStateWireframe.Get());
	}
	else
	{
		s->d3dContext->RSSetState(s->d3dRasterizerStateSolid.Get());
	}
}

void
TeardownRenderer(D3DRendererState* s)
{
	if (s == nullptr) return;

	s->d3dDevice                  .Reset();
	s->d3dContext                 .Reset();
	s->dxgiFactory                .Reset();
	s->d3dRenderTexture           .Reset();
	s->d3dRenderTargetView        .Reset();
	s->d3dDepthBufferView         .Reset();
	s->d3dVertexShader            .Reset();
	s->d3dVSConstBuffer           .Reset();
	s->d3dVSInputLayout           .Reset();
	s->d3dPixelShader             .Reset();
	s->d3dRasterizerStateSolid    .Reset();
	s->d3dRasterizerStateWireframe.Reset();
	s->d3d9                       .Reset();
	s->d3d9Device                 .Reset();
	s->d3d9RenderTexture          .Reset();
	s->d3d9RenderSurface0         .Reset();


	//Log live objects
	{
		#ifdef DEBUG
		HRESULT hr;

		ComPtr<IDXGIDebug1> dxgiDebug;
		hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		LOG_IF(FAILED(hr), L"", Severity::Warning, return);

		//TODO: Test the differences in the output
		//DXGI_DEBUG_RLO_ALL
		//DXGI_DEBUG_RLO_SUMMARY
		//DXGI_DEBUG_RLO_DETAIL
		hr = dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
		LOG_IF(FAILED(hr), L"", Severity::Warning, return);

		OutputDebugStringW(L"\n");
		#endif
	}
}


DrawCall*
PushDrawCall(D3DRendererState* s)
{
	Assert(s->drawCallCount < ArrayCount(s->drawCalls));

	DrawCall* drawCall = &s->drawCalls[s->drawCallCount++];
	return drawCall;
}

b32
Render(D3DRendererState* s, PreviewWindowState* previewWindow)
{
	// TODO: Should we assert things that will immediately crash the program
	// anyway (e.g. dereferenced below)? It probably gives us a better error
	// message.
	Assert(s->d3dContext       != nullptr);
	Assert(s->d3dRenderTexture != nullptr);


	HRESULT hr;

	s->d3dContext->ClearRenderTargetView(s->d3dRenderTargetView.Get(), DirectX::Colors::Black);
	s->d3dContext->ClearDepthStencilView(s->d3dDepthBufferView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	//DEBUG: Spin
	{
		i64 ticks;
		QueryPerformanceCounter((LARGE_INTEGER*) &ticks);

		i64 frequency;
		QueryPerformanceFrequency((LARGE_INTEGER*) &frequency);

		r32 t = r32 ((r64) ticks / (r64) frequency);

		static r32 lastT = 0;
		r32 deltaT = t - lastT;
		lastT = t;

		XMMATRIX world    = XMLoadFloat4x4((XMFLOAT4X4*) &s->drawCalls[0].worldM);
		XMMATRIX rotation = XMMatrixRotationZ(deltaT);

		XMStoreFloat4x4((XMFLOAT4X4*) &s->drawCalls[0].worldM, rotation * world);
	}

	//Update Camera
	XMVECTOR pos    = {0, 0, 0};
	XMVECTOR target = {0, 0, 1};
	XMVECTOR up     = {0, 1, 0};

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);

	for (size_t i = 0; i < s->drawCallCount; i++)
	{
		DrawCall* dc   = &s->drawCalls[i];
		MeshData* mesh = &s->meshes[dc->mesh];

		XMMATRIX W = XMMATRIX((r32*) &dc->worldM);
		XMMATRIX P = XMLoadFloat4x4(&s->proj);
		XMMATRIX WVP = XMMatrixTranspose(W * V * P);

		//Update VS cbuffer
		D3D11_MAPPED_SUBRESOURCE vsConstBuffMap = {};
		hr = s->d3dContext->Map(s->d3dVSConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vsConstBuffMap);
		LOG_IF(FAILED(hr), L"", Severity::Error, return false);

		memcpy(vsConstBuffMap.pData, &WVP, sizeof(WVP));
		s->d3dContext->Unmap(s->d3dVSConstBuffer.Get(), 0);

		s->d3dContext->DrawIndexed(mesh->iCount, 0, mesh->vOffset);
	}
	//s->drawCallCount = 0;

	//TODO: Why is this here again? I think it's for the WPF app
	//s->d3dContext->Flush();

	if (previewWindow->hwnd)
	{
		/* TODO: Handle DXGI_ERROR_DEVICE_RESET and DXGI_ERROR_DEVICE_REMOVED
		 * Developer Command Prompt for Visual Studio as an administrator, and
		 * typing dxcap -forcetdr which will immediately cause all currently running
		 * Direct3D apps to get a DXGI_ERROR_DEVICE_REMOVED event.
		 */
		s->d3dContext->CopyResource(previewWindow->backBuffer.Get(), s->d3dRenderTexture.Get());
		hr = previewWindow->swapChain->Present(0, 0);
	}

	return true;
}