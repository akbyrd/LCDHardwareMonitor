// TODO: Spend up to 8 ms receiving messages
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Add loops to connecting, reading, and writing (predicated on making progress each iteration)
// TODO: Be careful to ensure the gui and sim never end up doing a blocking write simultaneously
// TODO: Decide whether to support multiple instances or enforce a single instance

#pragma unmanaged
#include "LHMAPI.h"

#include "platform.h"
#include "plugin_shared.h"
#include "gui_protocol.hpp"

#include "platform_win32.hpp"
#include "renderer_d3d9.hpp"

struct State
{
	Pipe                pipe;
	u32                 activeMessageId;

	IDirect3D9Ex*       d3d9;
	IDirect3DDevice9Ex* d3d9Device;
	IDirect3DTexture9*  d3d9RenderTexture;
	IDirect3DSurface9*  d3d9RenderSurface0;
};
static State state = {};

#pragma managed

// TODO: Can this be implemented as an extension method to System::String?
System::String^
ToSystemString(StringSlice cstring)
{
	LOG_IF((i32) cstring.length < 0, IGNORE,
		Severity::Warning, "Native string truncated");

	System::String^ result = gcnew System::String(cstring.data, 0, (i32) cstring.length);
	return result;
}

public value struct GUIInterop abstract sealed
{
	static bool
	Initialize(System::IntPtr hwnd)
	{
		// DEBUG: Needed for the Watch window
		State& s = state;

		using namespace Message;

		PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, return false,
			Severity::Error, "Failed to create pipe for GUI communication");

		b32 success = D3D9_Initialize((HWND) (void*) hwnd, &s.d3d9, &s.d3d9Device);
		if (!success) return false;

		s.activeMessageId = Connect::Id;
		return true;
	}

	static bool
	Update(SimulationState^ simState)
	{
		// DEBUG: Needed for the Watch window
		State& s = state;

		using namespace Message;

		// Receive messages
		{
			b32 success;

			Bytes bytes = {};
			defer { List_Free(bytes); };

			success = ReceiveMessage(&s.pipe, bytes, &s.activeMessageId);
			if (success && bytes.length != 0)
			{
				Header* header = (Header*) bytes.data;
				switch (header->id)
				{
					default: Assert(false); break;
					case Null::Id: break;

					case Connect::Id:
					{
						Connect* connect = (Connect*) bytes.data;
						simState->Version = connect->version;

						success = D3D9_CreateSharedSurface(
							s.d3d9Device,
							&s.d3d9RenderTexture,
							&s.d3d9RenderSurface0,
							(HANDLE) connect->renderSurface,
							connect->renderSize
						);
						if (!success) return false;

						simState->RenderSurface = (System::IntPtr) s.d3d9RenderSurface0;
						break;
					}

					case Plugins::Id:
					{
						Plugins* plugins = (Plugins*) bytes.data;
						for (u32 i = 0; i < plugins->sensorPlugins.length; i++)
						{
							PluginInfo pluginInfo = plugins->sensorPlugins[i];

							PluginInfo_CLR pluginInfo_clr = {};
							pluginInfo_clr.Name    = ToSystemString(pluginInfo.name);
							pluginInfo_clr.Kind    = PluginKind_CLR::Sensor;
							pluginInfo_clr.Author  = ToSystemString(pluginInfo.author);
							pluginInfo_clr.Version = pluginInfo.version;
							simState->Plugins->Add(pluginInfo_clr);
						}
						for (u32 i = 0; i < plugins->widgetPlugins.length; i++)
						{
							PluginInfo pluginInfo = plugins->widgetPlugins[i];

							PluginInfo_CLR pluginInfo_clr = {};
							pluginInfo_clr.Name    = ToSystemString(pluginInfo.name);
							pluginInfo_clr.Kind    = PluginKind_CLR::Widget;
							pluginInfo_clr.Author  = ToSystemString(pluginInfo.author);
							pluginInfo_clr.Version = pluginInfo.version;
							simState->Plugins->Add(pluginInfo_clr);
						}
						break;
					}

					case Sensors::Id:
					{
						Sensors* sensors = (Sensors*) bytes.data;
						for (u32 i = 0; i < sensors->sensors.length; i++)
						{
							List<Sensor> sensors2 = sensors->sensors[i];
							for (u32 j = 0; j < sensors2.length; j++)
							{
								Sensor sensor = sensors2[j];

								Sensor_CLR sensor_clr = {};
								sensor_clr.Name       = ToSystemString(sensor.name);
								sensor_clr.Identifier = ToSystemString(sensor.identifier);
								sensor_clr.String     = ToSystemString(sensor.string);
								sensor_clr.Value      = sensor.value;
								sensor_clr.MinValue   = sensor.minValue;
								sensor_clr.MaxValue   = sensor.maxValue;
								simState->Sensors->Add(sensor_clr);
							}
						}
						break;
					}
				}
			}
		}

		// Reset on disconnects
		{
			if (s.pipe.state == PipeState::Disconnecting
			 || s.pipe.state == PipeState::Disconnected)
			{
				if (simState->RenderSurface != System::IntPtr::Zero)
				{
					D3D9_DestroySharedSurface(&s.d3d9RenderTexture, &s.d3d9RenderSurface0);
					simState->RenderSurface = System::IntPtr::Zero;

					s.activeMessageId = Connect::Id;
				}
			}
		}

		return true;
	}

	static void
	Teardown()
	{
		// DEBUG: Needed for the Watch window
		State& s = state;

		D3D9_DestroySharedSurface(&s.d3d9RenderTexture, &s.d3d9RenderSurface0);
		D3D9_Teardown(s.d3d9, s.d3d9Device);

		// TODO: Is there anything sane we can do with failures?
		Platform_DisconnectPipe(&s.pipe);

		s = {};
	}
};
