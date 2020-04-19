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
	using namespace Message;
	using namespace System;
	using namespace System::Collections::ObjectModel;
	using namespace System::ComponentModel;
	using namespace System::Diagnostics;
	using namespace System::Windows;
	using namespace System::Windows::Input;

	// TODO: Should this file use LHMAPICLR?
	using CLRString = System::String;
	using CLRMouseButton = System::Windows::Input::MouseButton;
	using LHMString = ::String;

	public enum struct PluginKind
	{
		Null,
		Sensor,
		Widget,
	};

	public enum struct PluginLoadState
	{
		Null,
		Loading,
		Loaded,
		Unloading,
		Unloaded,
		Broken,
	};

	public value struct PluginInfo
	{
		property CLRString^ Name;
		property CLRString^ Author;
		property UInt32     Version;

	};

	public value struct Plugin
	{
		property UInt32          Ref;
		property PluginLoadState LoadState;
		property PluginInfo      Info;
		property PluginKind      Kind;
	};

	public value struct Sensor
	{
		property UInt32     PluginRef;
		property UInt32     Ref;
		property CLRString^ Name;
		property CLRString^ Identifier;
		property CLRString^ Format;
		property Single     Value;
	};

	public value struct WidgetDesc
	{
		property UInt32     PluginRef;
		property UInt32     Ref;
		property CLRString^ Name;
	};

	public enum struct ProcessState
	{
		Null,
		Launching,
		Launched,
		Terminating,
		Terminated,
	};

	public ref class SimulationState : INotifyPropertyChanged
	{
	public:
		property UInt32                            Version;
		property IntPtr                            RenderSurface;
		property ObservableCollection<Plugin>^     Plugins;
		property ObservableCollection<Sensor>^     Sensors;
		property ObservableCollection<WidgetDesc>^ WidgetDescs;

		// UI Helpers
		property bool         IsSimulationConnected;
		property ProcessState ProcessState;
		property Stopwatch^   ProcessStateTimer;
		property Point        MousePos;

		// Cruft
		SimulationState()
		{
			Plugins           = gcnew ObservableCollection<Plugin>();
			Sensors           = gcnew ObservableCollection<Sensor>();
			WidgetDescs       = gcnew ObservableCollection<WidgetDesc>();
			ProcessStateTimer = gcnew Stopwatch();
		}

		virtual event PropertyChangedEventHandler^ PropertyChanged;
		void NotifyPropertyChanged(CLRString^ propertyName)
		{
			// TODO: Is this safe without the null check?
			PropertyChanged(this, gcnew PropertyChangedEventArgs(propertyName));
		}
	};

	public value struct Interop abstract sealed
	{
		// -------------------------------------------------------------------------------------------
		// Helper Functions

		static Message::MouseButton
		ToMouseButton(CLRMouseButton button)
		{
			switch (button)
			{
				default: Assert(false);      return Message::MouseButton::Null;
				case CLRMouseButton::Left:   return Message::MouseButton::Left;
				case CLRMouseButton::Middle: return Message::MouseButton::Middle;
				case CLRMouseButton::Right:  return Message::MouseButton::Right;
			}
		}

		static Message::ButtonState
		ToButtonState(MouseButtonState buttonState)
		{
			switch (buttonState)
			{
				default: Assert(false);          return Message::ButtonState::Null;
				case MouseButtonState::Pressed:  return Message::ButtonState::Down;
				case MouseButtonState::Released: return Message::ButtonState::Up;
			}
		}

		// TODO: LHMString?
		static CLRString^
		ToManagedString(StringView cstring)
		{
			LOG_IF((i32) cstring.length < 0, IGNORE,
				Severity::Warning, "Native string truncated");

			CLRString^ result = gcnew CLRString(cstring.data, 0, (i32) cstring.length);
			return result;
		}

		// -------------------------------------------------------------------------------------------

		static void
		SetProcessState(SimulationState^ simState, ProcessState processState)
		{
			if (simState->ProcessState != processState)
			{
				simState->ProcessState = processState;
				simState->ProcessStateTimer->Restart();
				simState->NotifyPropertyChanged("");
			}
		}

		static void
		LaunchSim(SimulationState^ simState)
		{
			SetProcessState(simState, ProcessState::Launching);
			Process::Start("LCDHardwareMonitor.exe");
		}

		static void
		ForceTerminateSim(SimulationState^ simState)
		{
			SetProcessState(simState, ProcessState::Terminating);
			array<Process^>^ processes = Process::GetProcessesByName("LCDHardwareMonitor");
			for each (Process^ p in processes)
				p->Kill();
		}

		// -------------------------------------------------------------------------------------------
		// Messages to Simulation

		static void
		TerminateSim(SimulationState^ simState)
		{
			SetProcessState(simState, ProcessState::Terminating);
			Message::TerminateSimulation terminate = {};
			SerializeAndQueueMessage(state.simConnection, terminate);
		}

		static void
		MouseMove(SimulationState^ simState, Point pos)
		{
			if (pos == simState->MousePos) return;
			simState->MousePos = pos;
			simState->NotifyPropertyChanged("");

			Message::MouseMove move = {};
			move.pos = { (int) pos.X, (int) pos.Y };
			SerializeAndQueueMessage(state.simConnection, move);
		}

		static void
		MouseButtonChange(SimulationState^, Point pos, CLRMouseButton button, MouseButtonState buttonState)
		{
			Message::MouseButtonChange buttonChange = {};
			buttonChange.pos = { (int) pos.X, (int) pos.Y };
			buttonChange.button = ToMouseButton(button);
			buttonChange.state = ToButtonState(buttonState);
			SerializeAndQueueMessage(state.simConnection, buttonChange);
		}

		static void
		SetPluginLoadState(SimulationState^ simState, UInt32 ref, PluginLoadState loadState)
		{
			Plugin plugin = simState->Plugins[ref - 1];
			Assert(plugin.Ref == ref);

			PluginRef nRef = { plugin.Ref };
			::PluginLoadState nLoadState = (::PluginLoadState) loadState;

			Message::SetPluginLoadStates setLoadStates = {};
			setLoadStates.refs = nRef;
			setLoadStates.loadStates = nLoadState;
			SerializeAndQueueMessage(state.simConnection, setLoadStates);

			plugin.LoadState = loadState == PluginLoadState::Unloaded ? PluginLoadState::Unloading : PluginLoadState::Loading;
			simState->Plugins[ref - 1] = plugin;
			simState->NotifyPropertyChanged("");
		}

		static void
		CreateWidget(SimulationState^, UInt32 pluginRef, UInt32 descRef, Point pos)
		{
			FullWidgetDataRef ref = {};
			ref.pluginRef = { pluginRef };
			ref.dataRef = { descRef };
			v2 position = { (float) pos.X, (float) pos.Y };

			Message::CreateWidgets createWidget = {};
			createWidget.ref = ref;
			createWidget.positions = position;
			SerializeAndQueueMessage(state.simConnection, createWidget);
		}

		static void
		DragDrop(SimulationState^, PluginKind pluginKind, bool inProgress)
		{
			Message::DragDrop dragDrop = {};
			dragDrop.pluginKind = (::PluginKind) pluginKind;
			dragDrop.inProgress = inProgress;
			SerializeAndQueueMessage(state.simConnection, dragDrop);
		}

		// -------------------------------------------------------------------------------------------
		// Incoming Messages

		static bool
		FromGUI_Connect(SimulationState% simState, Message::Connect& connect)
		{
			simState.Version = connect.version;

			b8 success = D3D9_CreateSharedSurface(
				*state.d3d9Device,
				state.d3d9RenderTexture,
				state.d3d9RenderSurface0,
				(HANDLE) connect.renderSurface,
				connect.renderSize
			);
			if (!success) return false;

			simState.RenderSurface = (IntPtr) state.d3d9RenderSurface0;
			simState.IsSimulationConnected = true;
			simState.NotifyPropertyChanged("");
			return true;
		}

		static void
		FromGUI_Disconnect(SimulationState% simState)
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
		}

		static void
		FromGUI_PluginsAdded(SimulationState% simState, Message::PluginsAdded& pluginsAdded)
		{
			for (u32 i = 0; i < pluginsAdded.infos.length; i++)
			{
				Plugin mPlugin = {};
				mPlugin.Ref  = pluginsAdded.refs[i].index;
				mPlugin.Kind = (PluginKind) pluginsAdded.kinds[i];

				PluginInfo mPluginInfo = {};
				mPluginInfo.Name    = ToManagedString(pluginsAdded.infos[i].name);
				mPluginInfo.Author  = ToManagedString(pluginsAdded.infos[i].author);
				mPluginInfo.Version = pluginsAdded.infos[i].version;
				mPlugin.Info = mPluginInfo;

				// TODO: Re-use empty plugin slots
				simState.Plugins->Add(mPlugin);
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromGUI_PluginStatesChanged(SimulationState% simState, Message::PluginStatesChanged& statesChanged)
		{
			for (u32 i = 0; i < statesChanged.refs.length; i++)
			{
				for (u32 j = 0; j < (u32) simState.Plugins->Count; j++)
				{
					Plugin p = simState.Plugins[(i32) j];
					if ((::PluginKind) p.Kind == statesChanged.kinds[i] && p.Ref == statesChanged.refs[i].index)
					{
						p.LoadState = (PluginLoadState) statesChanged.loadStates[i];
						simState.Plugins[(i32) j] = p;
					}
				}
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromGUI_SensorsAdded(SimulationState% simState, Message::SensorsAdded& sensorsAdded)
		{
			for (u32 i = 0; i < sensorsAdded.sensors.length; i++)
			{
				Slice<::Sensor> sensors = sensorsAdded.sensors[i];
				for (u32 j = 0; j < sensors.length; j++)
				{
					::Sensor& sensor = sensors[j];

					Sensor mSensor = {};
					mSensor.PluginRef  = sensorsAdded.sensorPluginRefs[i].index;
					mSensor.Ref        = sensor.ref.index;
					mSensor.Name       = ToManagedString(sensor.name);
					mSensor.Identifier = ToManagedString(sensor.identifier);
					mSensor.Format     = ToManagedString(sensor.format);
					mSensor.Value      = sensor.value;
					simState.Sensors->Add(mSensor);
				}
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromGUI_WidgetDescsAdded(SimulationState% simState, Message::WidgetDescsAdded& widgetDescsAdded)
		{
			for (u32 i = 0; i < widgetDescsAdded.descs.length; i++)
			{
				Slice<::WidgetDesc> widgetDescs = widgetDescsAdded.descs[i];
				for (u32 j = 0; j < widgetDescs.length; j++)
				{
					::WidgetDesc& desc = widgetDescs[j];

					WidgetDesc mWidgetDesc = {};
					mWidgetDesc.PluginRef = widgetDescsAdded.widgetPluginRefs[i].index;
					mWidgetDesc.Ref       = desc.ref.index;
					mWidgetDesc.Name      = ToManagedString(desc.name);
					simState.WidgetDescs->Add(mWidgetDesc);
				}
			}
			simState.NotifyPropertyChanged("");
		}

		// -------------------------------------------------------------------------------------------
		// Main Functions

		static bool
		Initialize(IntPtr hwnd)
		{
			// DEBUG: Needed for the Watch window
			State& s = state;

			PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", s.simConnection.pipe);
			LOG_IF(result == PipeResult::UnexpectedFailure, return false,
				Severity::Error, "Failed to create pipe for sim communication");

			b8 success = D3D9_Initialize((HWND) (void*) hwnd, s.d3d9, s.d3d9Device);
			if (!success) return false;

			return true;
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
					bool wasConnected = simCon.pipe.state == PipeState::Connected;
					PipeResult result = Platform_UpdatePipeConnection(simCon.pipe);
					HandleMessageResult(simCon, result);

					bool isConnected = simCon.pipe.state == PipeState::Connected;
					if (!isConnected && wasConnected) FromGUI_Disconnect(simState);
				}
				if (simCon.pipe.state != PipeState::Connected) break;

				// Receive
				i64 startTicks = Platform_GetTicks();
				while (MessageTimeLeft(startTicks))
				{
					b8 success = ReceiveMessage(simCon, bytes);
					if (!success) break;

					Header& header = (Header&) bytes[0];
					switch (header.id)
					{
						default: Assert(false); break;

						case IdOf<Connect>:
						{
							DeserializeMessage<Connect>(bytes);
							Connect& connect = (Connect&) bytes[0];
							success = FromGUI_Connect(simState, connect);
							if (!success) return false;
							break;
						}

						case IdOf<Disconnect>:
						{
							DeserializeMessage<Disconnect>(bytes);
							FromGUI_Disconnect(simState);
							break;
						}

						case IdOf<PluginsAdded>:
						{
							DeserializeMessage<PluginsAdded>(bytes);
							PluginsAdded& pluginsAdded = (PluginsAdded&) bytes[0];
							FromGUI_PluginsAdded(simState, pluginsAdded);
							break;
						}

						case IdOf<PluginStatesChanged>:
						{
							DeserializeMessage<PluginStatesChanged>(bytes);
							PluginStatesChanged& statesChanged = (PluginStatesChanged&) bytes[0];
							FromGUI_PluginStatesChanged(simState, statesChanged);
							break;
						}

						case IdOf<SensorsAdded>:
						{
							DeserializeMessage<SensorsAdded>(bytes);
							SensorsAdded& sensorsAdded = (SensorsAdded&) bytes[0];
							FromGUI_SensorsAdded(simState, sensorsAdded);
							break;
						}

						case IdOf<WidgetDescsAdded>:
						{
							DeserializeMessage<WidgetDescsAdded>(bytes);
							WidgetDescsAdded& widgetDescsAdded = (WidgetDescsAdded&) bytes[0];
							FromGUI_WidgetDescsAdded(simState, widgetDescsAdded);
							break;
						}
					}
				}

				// Send
				while (MessageTimeLeft(startTicks))
				{
					b8 success = SendMessage(simCon);
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
