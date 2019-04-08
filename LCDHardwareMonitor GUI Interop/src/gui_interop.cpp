// TODO: Spend up to 8 ms receiving messages
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Add loops to connecting, reading, and writing (predicated on making progress each iteration)
// TODO: Be careful to ensure the gui and sim never end up doing a blocking write simultaneously
// TODO: Decide whether to support multiple instances or enforce a single instance

#pragma unmanaged
#include "LHMAPI.h"

#include "platform.h"
#include "plugin_shared.h"
#include "gui_protocol.h"

// Fuck you, Microsoft
#pragma warning(push, 0)
#pragma push_macro("IGNORE")
#undef IGNORE
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma pop_macro("IGNORE")
#pragma warning(pop)

#include "platform_win32.hpp"

struct State
{
	Pipe pipe;
	u32  activeMessageId;
};
static State s = {};

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
	static b32
	Initialize()
	{
		using namespace Message;

		PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, return false,
			Severity::Error, "Failed to create pipe for GUI communication");

		s.activeMessageId = Connect::Id;
		return true;
	}

	static b32
	Update(SimulationState^ simState)
	{
		using namespace Message;

		Bytes bytes = {};
		defer { List_Free(bytes); };

		PipeResult result = ReceiveMessage(&s.pipe, bytes, &s.activeMessageId);
		switch (result)
		{
			default: Assert(false); break;

			case PipeResult::Success:
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
				break;
			}

			case PipeResult::TransientFailure:
				// Ignore failures, retry next frame
				break;

			case PipeResult::UnexpectedFailure:
				// TODO: Reset pipe connection?
				LOG(Severity::Error, "Failed to receive GUI message");
				return false;
		}

		return true;
	}

	static void
	Teardown()
	{
		// TODO: Is there anything sane we can do with failures?
		Platform_DisconnectPipe(&s.pipe);

		s = {};
	}
};
