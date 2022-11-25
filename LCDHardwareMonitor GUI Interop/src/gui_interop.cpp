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

// TODO: Why is this separate? Seems pointless. Strong typing?
struct State
{
	ConnectionState     simConnection;
	IDirect3D9Ex*       d3d9;
	IDirect3DDevice9Ex* d3d9Device;
	IDirect3DTexture9*  d3d9RenderTarget;
	IDirect3DSurface9*  d3d9RenderSurface0;
};
static State state = {};

#pragma managed
#include "LHMAPICLR.h"

namespace LCDHardwareMonitor::GUI
{
	using namespace System;
	using namespace System::Collections::ObjectModel;
	using namespace System::ComponentModel;
	using namespace System::Diagnostics;
	using namespace System::Windows;
	using namespace System::Windows::Input;

	// TODO: Should this file use LHMAPICLR?
	using CLRString = System::String;
	using LHMString = ::String;

	public enum struct PluginKind
	{
		Null,
		Sensor,
		Widget,
	};

	public enum struct PluginLanguage
	{
		Null,
		Builtin,
		Native,
		Managed,
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
		property UInt32          Handle;
		property PluginLoadState LoadState;
		property PluginInfo      Info;
		property PluginKind      Kind;
		property PluginLanguage  Language;
	};

	public value struct Sensor
	{
		property UInt32     Handle;
		property CLRString^ Name;
		property CLRString^ Identifier;
		property CLRString^ Format;
		property Single     Value;
	};

	public value struct WidgetDesc
	{
		property UInt32     Handle;
		property CLRString^ Name;
	};

	public value struct Widget
	{
		property UInt32 Handle;
	};

	public enum struct ProcessState
	{
		Null,
		Launching,
		Launched,
		Terminating,
		Terminated,
	};

	public enum struct Interaction
	{
		Null,
		MouseLook,
		Dragging,
	};

	public ref class SimulationState : INotifyPropertyChanged
	{
	public:
		property UInt32                            Version;
		property IntPtr                            RenderSurface;
		property Vector                            RenderSize;
		property ObservableCollection<Plugin>^     Plugins;
		property ObservableCollection<Sensor>^     Sensors;
		property ObservableCollection<WidgetDesc>^ WidgetDescs;
		property ObservableCollection<Widget>^     Widgets;
		property ObservableCollection<Widget>^     SelectedWidgets;
		property Interaction                       Interaction;

		// UI Helpers
		property bool         IsSimulationConnected;
		property ProcessState ProcessState;
		property Stopwatch^   ProcessStateTimer;
		property Point        MousePos;
		property bool         UpdatingSelection;

		// Cruft
		SimulationState()
		{
			Plugins           = gcnew ObservableCollection<Plugin>();
			Sensors           = gcnew ObservableCollection<Sensor>();
			WidgetDescs       = gcnew ObservableCollection<WidgetDesc>();
			Widgets           = gcnew ObservableCollection<Widget>();
			SelectedWidgets   = gcnew ObservableCollection<Widget>();
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

		// TODO: LHMString?
		static CLRString^
		ToManagedString(StringView lhmString)
		{
			LOG_IF((i32) lhmString.length < 0, IGNORE,
				Severity::Warning, "Native string truncated");

			CLRString^ result = gcnew CLRString(lhmString.data, 0, (i32) lhmString.length);
			return result;
		}

		static Vector
		ToManagedVector(v2u lhmVector)
		{
			Vector result = {};
			result.X = lhmVector.x;
			result.Y = lhmVector.y;
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
				// TODO: Got an Access Is Denied exception here once
				p->Kill();
		}

		// -------------------------------------------------------------------------------------------
		// Messages to Simulation

		static void
		TerminateSim(SimulationState^ simState)
		{
			SetProcessState(simState, ProcessState::Terminating);
			FromGUI::TerminateSimulation terminate = {};
			SerializeAndQueueMessage(state.simConnection, terminate);
		}

		static void
		SetPluginLoadState(SimulationState^ simState, Plugin% plugin, PluginLoadState loadState)
		{
			FromGUI::SetPluginLoadStates setLoadStates = {};
			setLoadStates.handles    = Handle<::Plugin> { plugin.Handle };
			setLoadStates.loadStates = (::PluginLoadState) loadState;
			SerializeAndQueueMessage(state.simConnection, setLoadStates);

			// TODO: Plugin isn't actually being passed by reference
			plugin.LoadState = loadState == PluginLoadState::Unloaded ? PluginLoadState::Unloading : PluginLoadState::Loading;
			simState->NotifyPropertyChanged("");
		}

		static void
		MouseMove(SimulationState^ simState, Point pos)
		{
			if (pos == simState->MousePos) return;
			simState->MousePos = pos;
			simState->NotifyPropertyChanged("");

			FromGUI::MouseMove move = {};
			move.pos = { (int) pos.X, (int) pos.Y };
			SerializeAndQueueMessage(state.simConnection, move);
		}

		static void
		SelectHovered(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Null) return;

			FromGUI::SelectHovered selectHovered = {};
			SerializeAndQueueMessage(state.simConnection, selectHovered);
		}

		static void
		BeginMouseLook(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Null) return;
			simState->Interaction = Interaction::MouseLook;
			simState->NotifyPropertyChanged("");

			FromGUI::BeginMouseLook beginMouseLook = {};
			SerializeAndQueueMessage(state.simConnection, beginMouseLook);
		}

		static void
		EndMouseLook(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::MouseLook) return;
			simState->Interaction = Interaction::Null;
			simState->NotifyPropertyChanged("");

			FromGUI::EndMouseLook endMouseLook = {};
			SerializeAndQueueMessage(state.simConnection, endMouseLook);
		}

		static void
		ResetCamera(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Null) return;

			FromGUI::ResetCamera resetCamera = {};
			SerializeAndQueueMessage(state.simConnection, resetCamera);
		}

		static void
		BeginDragSelection(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Null) return;
			simState->Interaction = Interaction::Dragging;
			simState->NotifyPropertyChanged("");

			FromGUI::BeginDragSelection beginDrag = {};
			SerializeAndQueueMessage(state.simConnection, beginDrag);
		}

		static void
		EndDragSelection(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Dragging) return;
			simState->Interaction = Interaction::Null;
			simState->NotifyPropertyChanged("");

			FromGUI::EndDragSelection endDrag = {};
			SerializeAndQueueMessage(state.simConnection, endDrag);
		}

		static void
		DragDrop(SimulationState^ simState, PluginKind pluginKind, bool inProgress)
		{
			Unused(simState);
			Assert(simState->Interaction == Interaction::Null);

			FromGUI::DragDrop dragDrop = {};
			dragDrop.pluginKind = (::PluginKind) pluginKind;
			dragDrop.inProgress = inProgress;
			SerializeAndQueueMessage(state.simConnection, dragDrop);
		}

		static void
		AddWidget(SimulationState^ simState, UInt32 Handle, Point pos)
		{
			Unused(simState);
			Assert(simState->Interaction == Interaction::Null);

			FromGUI::AddWidget addWidget = {};
			addWidget.handle   = { Handle };
			addWidget.position = { (r32) pos.X, (r32) pos.Y };
			SerializeAndQueueMessage(state.simConnection, addWidget);
		}

		static void
		RemoveSelectedWidgets(SimulationState^ simState)
		{
			if (simState->Interaction != Interaction::Null) return;

			FromGUI::RemoveSelectedWidgets removeSelectedWidgets = {};
			SerializeAndQueueMessage(state.simConnection, removeSelectedWidgets);
		}

		static void
		SetWidgetSelection(SimulationState^, System::Collections::IList^ selectedWidgets)
		{
			List<Handle<::Widget>> selectedHandles = {};
			defer { List_Free(selectedHandles); };

			for (i32 i = 0; i < selectedWidgets->Count; i++)
			{
				Widget% selectedWidget = (Widget%) selectedWidgets[i];
				List_Append(selectedHandles, { selectedWidget.Handle });
			}

			FromGUI::SetWidgetSelection widgetSelection = {};
			widgetSelection.handles = selectedHandles;
			SerializeAndQueueMessage(state.simConnection, widgetSelection);
		}

		// -------------------------------------------------------------------------------------------
		// Incoming Messages

		static void
		FromSim_Connect(SimulationState% simState, ToGUI::Connect& connect)
		{
			simState.Version = connect.version;

			b8 success = D3D9_CreateSharedSurface(
				*state.d3d9Device,
				state.d3d9RenderTarget,
				state.d3d9RenderSurface0,
				(HANDLE) connect.renderSurface,
				connect.renderSize
			);
			LOG_IF(!success, IGNORE,
				Severity::Warning, "Failed to D3D9 shared surface");

			simState.RenderSurface = (IntPtr) state.d3d9RenderSurface0;
			simState.RenderSize = ToManagedVector(connect.renderSize);
			simState.IsSimulationConnected = true;
			simState.NotifyPropertyChanged("");
		}

		static void
		FromSim_Disconnect(SimulationState% simState, ToGUI::Disconnect&)
		{
			// DEBUG: Needed for the Watch window
			State& s = state;

			D3D9_DestroySharedSurface(s.d3d9RenderTarget, s.d3d9RenderSurface0);
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
		FromSim_PluginsAdded(SimulationState% simState, ToGUI::PluginsAdded& pluginsAdded)
		{
			for (u32 i = 0; i < pluginsAdded.infos.length; i++)
			{
				Plugin mPlugin = {};
				mPlugin.Handle   = pluginsAdded.handles[i].value;
				mPlugin.Kind     = (PluginKind) pluginsAdded.kinds[i];
				mPlugin.Language = (PluginLanguage) pluginsAdded.languages[i];

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
		FromSim_PluginStatesChanged(SimulationState% simState, ToGUI::PluginStatesChanged& statesChanged)
		{
			for (u32 i = 0; i < statesChanged.handles.length; i++)
			{
				for (u32 j = 0; j < (u32) simState.Plugins->Count; j++)
				{
					Plugin p = simState.Plugins[(i32) j];
					// TODO: I don't think we need to check the kind here. This is the untyped plugin handle
					if ((::PluginKind) p.Kind == statesChanged.kinds[i] && p.Handle == statesChanged.handles[i].value)
					{
						p.LoadState = (PluginLoadState) statesChanged.loadStates[i];
						simState.Plugins[(i32) j] = p;
					}
				}
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromSim_SensorsAdded(SimulationState% simState, ToGUI::SensorsAdded& sensorsAdded)
		{
			for (u32 i = 0; i < sensorsAdded.sensors.length; i++)
			{
				::Sensor sensor = sensorsAdded.sensors[i];

				Sensor mSensor = {};
				mSensor.Handle     = sensor.handle.value;
				mSensor.Name       = ToManagedString(sensor.name);
				mSensor.Identifier = ToManagedString(sensor.identifier);
				mSensor.Format     = ToManagedString(sensor.format);
				mSensor.Value      = sensor.value;
				simState.Sensors->Add(mSensor);
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromSim_WidgetDescsAdded(SimulationState% simState, ToGUI::WidgetDescsAdded& widgetDescsAdded)
		{
			for (u32 i = 0; i < widgetDescsAdded.handles.length; i++)
			{
				// TODO: Are we just leaking strings in messages?
				WidgetDesc mWidgetDesc = {};
				mWidgetDesc.Handle = widgetDescsAdded.handles[i].value;
				mWidgetDesc.Name   = ToManagedString(widgetDescsAdded.names[i]);
				simState.WidgetDescs->Add(mWidgetDesc);
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromSim_WidgetsAdded(SimulationState% simState, ToGUI::WidgetsAdded& widgetsAdded)
		{
			for (u32 i = 0; i < widgetsAdded.handles.length; i++)
			{
				Widget mWidget = {};
				mWidget.Handle = widgetsAdded.handles[i].value;
				simState.Widgets->Add(mWidget);
			}
			simState.NotifyPropertyChanged("");
		}

		static void
		FromSim_WidgetSelectionChanged(SimulationState% simState, ToGUI::WidgetSelectionChanged& widgetSelection)
		{
			simState.UpdatingSelection = true;
			simState.SelectedWidgets->Clear();
			for (u32 i = 0; i < widgetSelection.handles.length; i++)
			{
				Widget mWidget = {};
				mWidget.Handle = widgetSelection.handles[i].value;
				simState.SelectedWidgets->Add(mWidget);
			}
			simState.NotifyPropertyChanged("");
			simState.UpdatingSelection = false;
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
					if (!isConnected && wasConnected)
					{
						ToGUI::Disconnect disconnect = {};
						FromSim_Disconnect(simState, disconnect);
					}
				}
				if (simCon.pipe.state != PipeState::Connected) break;

				// Receive
				i64 startTicks = Platform_GetTicks();
				while (MessageTimeLeft(startTicks))
				{
					b8 success = ReceiveMessage(simCon, bytes);
					if (!success) break;

					#define HANDLE_MESSAGE(Type) \
						case IdOf<ToGUI::Type>: \
						{ \
							using Type = ToGUI::Type; \
							DeserializeMessage<Type>(bytes); \
							Type& message = (Type&) bytes[0]; \
							FromSim_##Type(simState, message); \
							break; \
						}

					Message::Header& header = (Message::Header&) bytes[0];
					switch (header.id)
					{
						default:
						case IdOf<Message::Null>:
							Assert(false);
							break;

						HANDLE_MESSAGE(Connect)
						HANDLE_MESSAGE(Disconnect)
						HANDLE_MESSAGE(PluginsAdded)
						HANDLE_MESSAGE(PluginStatesChanged)
						HANDLE_MESSAGE(SensorsAdded)
						HANDLE_MESSAGE(WidgetDescsAdded)
						HANDLE_MESSAGE(WidgetsAdded)
						HANDLE_MESSAGE(WidgetSelectionChanged)
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

			D3D9_DestroySharedSurface(s.d3d9RenderTarget, s.d3d9RenderSurface0);
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
			AssertOrUnused(result);
			return Point(point.x, point.y);
		}
	};
}
