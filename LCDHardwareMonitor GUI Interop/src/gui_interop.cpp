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

#include <WinUser.h>

struct State
{
	ConnectionState     simConnection;
	IDirect3D9Ex*       d3d9;
	IDirect3DDevice9Ex* d3d9Device;
	IDirect3DTexture9*  d3d9RenderTexture;
	IDirect3DSurface9*  d3d9RenderSurface0;
};
static State state = {};

#pragma managed
namespace LCDHardwareMonitor::GUI
{
	using namespace System;
	using namespace System::Collections::ObjectModel;
	using namespace System::ComponentModel;
	using namespace System::Diagnostics;
	using namespace System::Windows;
	using namespace System::Windows::Input;
	using sString = System::String;
	using nButtonState = Message::ButtonState;
	using nMouseButton = Message::MouseButton;
	using nMouseButtonChange = Message::MouseButtonChange;

	public enum struct PluginKind
	{
		Null,
		Sensor,
		Widget,
	};

	public enum struct PluginLoadState
	{
		Null,
		Loaded,
		Unloaded,
		Broken,
	};

	public value struct PluginInfo
	{
		property UInt32     Ref;
		property sString^   Name;
		property PluginKind Kind;
		property sString^   Author;
		property UInt32     Version;

		property PluginLoadState LoadState; // TODO: Probably belongs in another struct
	};

	public value struct Sensor
	{
		property UInt32   PluginRef;
		property UInt32   Ref;
		property sString^ Name;
		property sString^ Identifier;
		property sString^ Format;
		property Single   Value;
	};

	public value struct WidgetDesc
	{
		property UInt32   PluginRef;
		property UInt32   Ref;
		property sString^ Name;
	};

	public enum struct ProcessState
	{
		Null,
		Launching,
		Launched,
		Terminating,
		Terminated,
	};

	public enum struct MessageType
	{
		Null,
		LaunchSim,
		TerminateSim,
		ForceTerminateSim,
		SetPluginLoadState,

		// Input
		MouseMove,
		MouseButtonChange,
	};

	public value struct MouseButtonChange
	{
		property Point pos;
		property MouseButton button;
		property MouseButtonState state;
	};

	public value struct Message
	{
		Message(MessageType type) { this->type = type; data = nullptr; }
		Message(MessageType type, Object^ data) { this->type = type; this->data = data; }
		static operator Message(MessageType type) { return Message(type); }

		MessageType type;
		Object^ data;
	};

	public ref class SimulationState : INotifyPropertyChanged
	{
	public:
		property UInt32                            Version;
		property IntPtr                            RenderSurface;
		property ObservableCollection<PluginInfo>^ Plugins;
		property ObservableCollection<Sensor>^     Sensors;
		property ObservableCollection<WidgetDesc>^ WidgetDescs;

		// UI Helpers
		property bool         IsSimulationConnected;
		property ProcessState ProcessState;
		property Stopwatch^   ProcessStateTimer;
		property Point        MousePos;

		// Messages
		System::Collections::Generic::List<Message>^ Messages;

		// Cruft
		SimulationState()
		{
			Plugins           = gcnew ObservableCollection<PluginInfo>();
			Sensors           = gcnew ObservableCollection<Sensor>();
			WidgetDescs       = gcnew ObservableCollection<WidgetDesc>();
			Messages          = gcnew System::Collections::Generic::List<Message>();
			ProcessStateTimer = gcnew Stopwatch();
		}

		virtual event PropertyChangedEventHandler^ PropertyChanged;
		void NotifyPropertyChanged(sString^ propertyName)
		{
			// TODO: Is this safe without the null check?
			PropertyChanged(this, gcnew PropertyChangedEventArgs(propertyName));
		}
	};

	public value struct SetPluginLoadStates
	{
		PluginKind      kind;
		UInt32          ref_;
		PluginLoadState loadState;
	};

	nMouseButton
	ToMouseButton(MouseButton button)
	{
		switch (button)
		{
			default: Assert(false); return nMouseButton::Null;
			case MouseButton::Left: return nMouseButton::Left;
			case MouseButton::Middle: return nMouseButton::Middle;
			case MouseButton::Right: return nMouseButton::Right;
		}
	}

	nButtonState
	ToButtonState(MouseButtonState buttonState)
	{
		switch (buttonState)
		{
			default: Assert(false); return nButtonState::Null;
			case MouseButtonState::Pressed: return nButtonState::Down;
			case MouseButtonState::Released: return nButtonState::Up;
		}
	}

	sString^
	ToManagedString(StringSlice cstring)
	{
		LOG_IF((i32) cstring.length < 0, IGNORE,
			Severity::Warning, "Native string truncated");

		// TODO: Remove this
		u32 length = cstring.length;
		while (length > 0 && cstring[length - 1] == '\0')
			length--;

		sString^ result = gcnew sString(cstring.data, 0, (i32) length);
		return result;
	}

	public value struct Interop abstract sealed
	{
		static bool
		Initialize(IntPtr hwnd)
		{
			// DEBUG: Needed for the Watch window
			State& s = state;

			using namespace Message;

			PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", s.simConnection.pipe);
			LOG_IF(result == PipeResult::UnexpectedFailure, return false,
				Severity::Error, "Failed to create pipe for sim communication");

			b32 success = D3D9_Initialize((HWND) (void*) hwnd, s.d3d9, s.d3d9Device);
			if (!success) return false;

			return true;
		}

		static void
		OnDisconnect(SimulationState% simState)
		{
			// DEBUG: Needed for the Watch window
			State& s = state;

			D3D9_DestroySharedSurface(s.d3d9RenderTexture, s.d3d9RenderSurface0);
			simState.RenderSurface = IntPtr::Zero;
			simState.Plugins->Clear();
			simState.Sensors->Clear();
			simState.WidgetDescs->Clear();
			simState.IsSimulationConnected = false;
			simState.NotifyPropertyChanged("");

			ConnectionState& simCon = s.simConnection;
			simCon.sendIndex = 0;
			simCon.recvIndex = 0;
			// TODO: Will need to handle 'pending' states.
			simState.Messages->Clear();
		}

		static bool
		Update(SimulationState^ _simState)
		{
			// DEBUG: Needed for the Watch window
			State& s = state;
			SimulationState% simState = *_simState;

			ConnectionState& simCon = s.simConnection;
			Assert(!simCon.failure);

			while (true)
			{
				Bytes bytes = {};
				defer { List_Free(bytes); };

				// Connection handling
				{
					b32 wasConnected = simCon.pipe.state == PipeState::Connected;
					PipeResult result = Platform_UpdatePipeConnection(simCon.pipe);
					HandleMessageResult(simCon, result);

					b32 isConnected = simCon.pipe.state == PipeState::Connected;
					if (!isConnected && wasConnected) OnDisconnect(simState);
				}
				if (simCon.pipe.state != PipeState::Connected) break;

				// Receive
				i64 startTicks = Platform_GetTicks();
				while (MessageTimeLeft(startTicks))
				{
					b32 success = ReceiveMessage(simCon, bytes);
					if (!success) break;

					using namespace Message;
					Header& header = (Header&) bytes[0];
					switch (header.id)
					{
						default: Assert(false); break;

						case IdOf<Connect>:
						{
							DeserializeMessage<Connect>(bytes);

							Connect& connect = (Connect&) bytes[0];
							simState.Version = connect.version;

							success = D3D9_CreateSharedSurface(
								*s.d3d9Device,
								s.d3d9RenderTexture,
								s.d3d9RenderSurface0,
								(HANDLE) connect.renderSurface,
								connect.renderSize
							);
							if (!success) return false;

							simState.RenderSurface = (IntPtr) s.d3d9RenderSurface0;
							simState.IsSimulationConnected = true;
							simState.NotifyPropertyChanged("");
							break;
						}

						case IdOf<Disconnect>:
						{
							OnDisconnect(simState);
							break;
						}

						case IdOf<PluginsAdded>:
						{
							DeserializeMessage<PluginsAdded>(bytes);
							PluginsAdded& pluginsAdded = (PluginsAdded&) bytes[0];

							for (u32 i = 0; i < pluginsAdded.infos.length; i++)
							{
								PluginInfo mPluginInfo = {};
								mPluginInfo.Ref     = pluginsAdded.refs[i].index;
								mPluginInfo.Name    = ToManagedString(pluginsAdded.infos[i].name);
								mPluginInfo.Kind    = (PluginKind) pluginsAdded.kind;
								mPluginInfo.Author  = ToManagedString(pluginsAdded.infos[i].author);
								mPluginInfo.Version = pluginsAdded.infos[i].version;
								simState.Plugins->Add(mPluginInfo);
							}
							break;
						}

						case IdOf<PluginStatesChanged>:
						{
							DeserializeMessage<PluginStatesChanged>(bytes);
							PluginStatesChanged& statesChanged = (PluginStatesChanged&) bytes[0];

							for (u32 i = 0; i < statesChanged.refs.length; i++)
							{
								for (u32 j = 0; j < (u32) simState.Plugins->Count; j++)
								{
									PluginInfo p = simState.Plugins[(i32) j];
									if ((::PluginKind) p.Kind == statesChanged.kind && p.Ref == statesChanged.refs[i].index)
									{
										p.LoadState = (PluginLoadState) statesChanged.loadStates[i];
										simState.Plugins[(i32) j] = p;
									}
								}
							}
							simState.NotifyPropertyChanged("");
							break;
						}

						case IdOf<SensorsAdded>:
						{
							DeserializeMessage<SensorsAdded>(bytes);

							SensorsAdded& sensorsAdded = (SensorsAdded&) bytes[0];
							for (u32 i = 0; i < sensorsAdded.sensors.length; i++)
							{
								Slice<::Sensor> sensors = sensorsAdded.sensors[i];
								for (u32 j = 0; j < sensors.length; j++)
								{
									::Sensor& sensor = sensors[j];

									Sensor mSensor = {};
									mSensor.PluginRef  = sensorsAdded.pluginRefs[i].index;
									mSensor.Ref        = sensor.ref.index;
									mSensor.Name       = ToManagedString(sensor.name);
									mSensor.Identifier = ToManagedString(sensor.identifier);
									mSensor.Format     = ToManagedString(sensor.format);
									mSensor.Value      = sensor.value;
									simState.Sensors->Add(mSensor);
								}
							}
							break;
						}

						case IdOf<WidgetDescsAdded>:
						{
							DeserializeMessage<WidgetDescsAdded>(bytes);

							WidgetDescsAdded& widgetDescsAdded = (WidgetDescsAdded&) bytes[0];
							for (u32 i = 0; i < widgetDescsAdded.descs.length; i++)
							{
								Slice<::WidgetDesc> widgetDescs = widgetDescsAdded.descs[i];
								for (u32 j = 0; j < widgetDescs.length; j++)
								{
									::WidgetDesc& desc = widgetDescs[j];

									WidgetDesc mWidgetDesc = {};
									mWidgetDesc.PluginRef = widgetDescsAdded.pluginRefs[i].index;
									mWidgetDesc.Ref       = desc.ref.index;
									mWidgetDesc.Name      = ToManagedString(desc.name);
									simState.WidgetDescs->Add(mWidgetDesc);
								}
							}
							break;
						}
					}
				}

				// TODO: Change this to On*() functions called directly from C#, similar to the sim side
				// Construct messages
				for (i32 i = 0; i < simState.Messages->Count; i++)
				{
					using namespace Message;
					Message^ message = simState.Messages[i];
					switch (message->type)
					{
						// TODO: I think I'd rather have the warnings
						default: Assert(false); break;

						case MessageType::LaunchSim:
						case MessageType::ForceTerminateSim:
							// Handled in App
							break;

						case MessageType::TerminateSim:
						{
							TerminateSimulation nMessage = {};
							b32 success = SerializeAndQueueMessage(simCon, nMessage);
							if (!success) return false;
							break;
						}

						case MessageType::SetPluginLoadState:
							// TODO: Implement
							break;

						case MessageType::MouseMove:
						{
							Point pos = (Point) message->data;

							MouseMove nMessage = {};
							nMessage.pos = { (int) pos.X, (int) pos.Y };
							b32 success = SerializeAndQueueMessage(simCon, nMessage);
							if (!success) return false;
							break;
						}

						case MessageType::MouseButtonChange:
						{
							MouseButtonChange mouseButtonChange = (MouseButtonChange) message->data;

							nMouseButtonChange nMessage = {};
							nMessage.pos = { (int) mouseButtonChange.pos.X, (int) mouseButtonChange.pos.Y };
							nMessage.button = ToMouseButton(mouseButtonChange.button);
							nMessage.state = ToButtonState(mouseButtonChange.state);
							b32 success = SerializeAndQueueMessage(simCon, nMessage);
							if (!success) return false;
							break;
						}
					}
				}

				// Send
				while (MessageTimeLeft(startTicks))
				{
					b32 success = SendMessage(simCon);
					if (!success) break;
				}

				break;
			}

			return !simCon.failure;
		}

		static void
		Teardown()
		{
			// DEBUG: Needed for the Watch window
			State& s = state;

			D3D9_DestroySharedSurface(s.d3d9RenderTexture, s.d3d9RenderSurface0);
			D3D9_Teardown(s.d3d9, s.d3d9Device);

			ConnectionState& simCon = s.simConnection;
			Connection_Teardown(simCon);

			s = {};
		}
	};


	public value struct Win32 abstract sealed
	{
		static Point
		GetCursorPos()
		{
			POINT point = {};
			bool result = ::GetCursorPos(&point);
			// TODO: Logging
			Assert(result);
			return Point(point.x, point.y);
		}
	};
}
