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
	u32                 messageIndex;

	IDirect3D9Ex*       d3d9;
	IDirect3DDevice9Ex* d3d9Device;
	IDirect3DTexture9*  d3d9RenderTexture;
	IDirect3DSurface9*  d3d9RenderSurface0;
};
static State state = {};

#pragma managed
using namespace LCDHardwareMonitor;

System::String^
ToSystemString(StringSlice cstring)
{
	LOG_IF((i32) cstring.length < 0, IGNORE,
		Severity::Warning, "Native string truncated");

	// TODO: Remove this
	u32 length = cstring.length;
	while (length > 0 && cstring[length - 1] == '\0')
		length--;

	System::String^ result = gcnew System::String(cstring.data, 0, (i32) length);
	return result;
}

PluginKind_CLR
ToPluginKind(PluginKind kind)
{
	switch (kind)
	{
		default: Assert(false);  return PluginKind_CLR::Null;
		case PluginKind::Null:   return PluginKind_CLR::Null;
		case PluginKind::Sensor: return PluginKind_CLR::Sensor;
		case PluginKind::Widget: return PluginKind_CLR::Widget;
	}
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

		return true;
	}

	static bool
	Update(SimulationState^ simState)
	{
		// DEBUG: Needed for the Watch window
		State& s = state;

		// Receive
		{
			using namespace Message;

			// TODO: Decide on the code structure for this. Feels bad right now
			// TODO: Loop
			// Receive messages
			Bytes bytes = {};
			defer { List_Free(bytes); };

			PipeResult result = Platform_ReadPipe(&s.pipe, bytes);
			if (result == PipeResult::UnexpectedFailure) return false;
			if (result == PipeResult::TransientFailure) return true;

			// Synthesize a disconnect
			{
				b32 noMessage        = bytes.length == 0;
				b32 simConnected     = simState->IsSimulationConnected;
				b32 pipeDisconnected = s.pipe.state == PipeState::Disconnecting
				                    || s.pipe.state == PipeState::Disconnected;

				if (noMessage && simConnected && pipeDisconnected)
				{
					// TODO: Should do this without allocating. Maybe with a ByteSlice?
					b32 success = List_Reserve(bytes, sizeof(Disconnect));
					LOG_IF(!success, return false,
						Severity::Fatal, "Failed to disconnect from simulation");
					bytes.length = sizeof(Disconnect);

					Disconnect* disconnect = (Disconnect*) bytes.data;
					*disconnect = {};

					disconnect->header.id    = IdOf<Disconnect>;
					disconnect->header.index = s.messageIndex;
					disconnect->header.size  = sizeof(Disconnect);
				}
			}

			if (bytes.length == 0) return true;

			// TODO: Need to handle these errors more intelligently
			LOG_IF(bytes.length < sizeof(Header), return false,
				Severity::Warning, "Corrupted message received");

			Header* header = (Header*) bytes.data;

			LOG_IF(bytes.length != header->size, return false,
				Severity::Warning, "Incorrectly sized message received");

			LOG_IF(header->index != s.messageIndex, return false,
				Severity::Warning, "Unexpected message received");
			s.messageIndex++;

			switch (header->id)
		{
			default: Assert(false); break;
			case IdOf<Null>: break;

			case IdOf<Connect>:
			{
				DeserializeMessage<Connect>(bytes);

				Connect* connect = (Connect*) bytes.data;
				simState->Version = connect->version;

				b32 success = D3D9_CreateSharedSurface(
					s.d3d9Device,
					&s.d3d9RenderTexture,
					&s.d3d9RenderSurface0,
					(HANDLE) connect->renderSurface,
					connect->renderSize
				);
				if (!success) return false;

				simState->RenderSurface = (System::IntPtr) s.d3d9RenderSurface0;
				simState->IsSimulationConnected = true;
				simState->NotifyPropertyChanged("");
				break;
			}

			case IdOf<Disconnect>:
			{
				D3D9_DestroySharedSurface(&s.d3d9RenderTexture, &s.d3d9RenderSurface0);
				simState->RenderSurface = System::IntPtr::Zero;
				simState->Plugins->Clear();
				simState->Sensors->Clear();
				simState->WidgetDescs->Clear();
				simState->IsSimulationConnected = false;
				simState->NotifyPropertyChanged("");

				s.messageIndex = 0;
				break;
			}

			case IdOf<PluginsAdded>:
			{
				DeserializeMessage<PluginsAdded>(bytes);
				PluginsAdded* pluginsAdded = (PluginsAdded*) &bytes[0];

				for (u32 i = 0; i < pluginsAdded->infos.length; i++)
				{
					PluginInfo_CLR pluginInfo_clr = {};
					pluginInfo_clr.Ref     = pluginsAdded->refs[i].index;
					pluginInfo_clr.Name    = ToSystemString(pluginsAdded->infos[i].name);
					pluginInfo_clr.Kind    = ToPluginKind(pluginsAdded->kind);
					pluginInfo_clr.Author  = ToSystemString(pluginsAdded->infos[i].author);
					pluginInfo_clr.Version = pluginsAdded->infos[i].version;
					simState->Plugins->Add(pluginInfo_clr);
				}
				break;
			}

			case IdOf<PluginStatesChanged>:
			{
				#if true
				DeserializeMessage<PluginStatesChanged>(bytes);
				PluginStatesChanged* statesChanged = (PluginStatesChanged*) &bytes[0];

				for (u32 i = 0; i < statesChanged->refs.length; i++)
				{
					for (u32 j = 0; j < (u32) simState->Plugins->Count; j++)
					{
						PluginInfo_CLR p = simState->Plugins[(i32) j];
						if ((PluginKind) p.Kind == statesChanged->kind && p.Ref == statesChanged->refs[i].index)
						{
							p.LoadState = (PluginLoadState_CLR) statesChanged->loadStates[i];
							simState->Plugins[(i32) j] = p;
						}
					}
				}
				simState->NotifyPropertyChanged("");
				#endif
				break;
			}

			case IdOf<SensorsAdded>:
			{
				DeserializeMessage<SensorsAdded>(bytes);

				SensorsAdded* sensorsAdded = (SensorsAdded*) bytes.data;
				for (u32 i = 0; i < sensorsAdded->sensors.length; i++)
				{
					List<Sensor> sensors = sensorsAdded->sensors[i];
					for (u32 j = 0; j < sensors.length; j++)
					{
						Sensor* sensor = &sensors[j];

						Sensor_CLR sensor_clr = {};
						sensor_clr.PluginRef  = sensorsAdded->pluginRefs[i].index;
						sensor_clr.Ref        = sensor->ref.index;
						sensor_clr.Name       = ToSystemString(sensor->name);
						sensor_clr.Identifier = ToSystemString(sensor->identifier);
						sensor_clr.Format     = ToSystemString(sensor->format);
						sensor_clr.Value      = sensor->value;
						simState->Sensors->Add(sensor_clr);
					}
				}
				break;
			}

			case IdOf<WidgetDescsAdded>:
			{
				DeserializeMessage<WidgetDescsAdded>(bytes);

				WidgetDescsAdded* widgetDescsAdded = (WidgetDescsAdded*) bytes.data;
				for (u32 i = 0; i < widgetDescsAdded->descs.length; i++)
				{
					Slice<WidgetDesc> widgetDescs = widgetDescsAdded->descs[i];
					for (u32 j = 0; j < widgetDescs.length; j++)
					{
						WidgetDesc* desc = &widgetDescs[j];

						WidgetDesc_CLR widgetDesc_clr = {};
						widgetDesc_clr.PluginRef = widgetDescsAdded->pluginRefs[i].index;
						widgetDesc_clr.Ref       = desc->ref.index;
						widgetDesc_clr.Name      = ToSystemString(desc->name);
						simState->WidgetDescs->Add(widgetDesc_clr);
					}
				}
				break;
			}
		}
		}

		// Send
		{
			for (u32 i = 0; i < (u32) simState->Messages->Count; i++)
			{
				// TODO: Implement
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

		PipeResult result = Platform_FlushPipe(&s.pipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, IGNORE,
			Severity::Error, "Failed to flush GUI communication pipe");
		Platform_DestroyPipe(&s.pipe);

		s = {};
	}
};
