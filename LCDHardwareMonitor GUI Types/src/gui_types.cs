using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace LCDHardwareMonitor
{
	public enum PluginKind_CLR
	{
		Null,
		Sensor,
		Widget,
	}

	public enum PluginLoadState_CLR
	{
		Null,
		Loaded,
		Unloaded,
		Broken,
	}

	public struct PluginInfo_CLR
	{
		public uint           Ref     { get; set; }
		public string         Name    { get; set; }
		public PluginKind_CLR Kind    { get; set; }
		public string         Author  { get; set; }
		public uint           Version { get; set; }

		public PluginLoadState_CLR LoadState { get; set; } // TODO: Probably belongs in another struct
	}

	public struct Sensor_CLR
	{
		public uint   PluginRef  { get; set; }
		public uint   Ref        { get; set; }
		public string Name       { get; set; }
		public string Identifier { get; set; }
		public string Format     { get; set; }
		public float  Value      { get; set; }
	}

	public struct WidgetDesc_CLR
	{
		public uint   PluginRef { get; set; }
		public uint   Ref       { get; set; }
		public string Name      { get; set; }
	}

	public struct Message_
	{
		public Message_(MessageType type) { this.type = type; data = null; }
		public Message_(MessageType type, object data) { this.type = type; this.data = data; }
		public static implicit operator Message_ (MessageType type) { return new Message_(type);  }

		public MessageType type;
		public object      data;
	}

	public enum MessageType
	{
		Null,
		LaunchSim,
		CloseSim,
		KillSim,
		SetPluginLoadState,
	}

	public struct SetPluginLoadStates
	{
		public PluginKind_CLR      kind;
		public uint                ref_;
		public PluginLoadState_CLR loadState;
	};

	public class SimulationState : INotifyPropertyChanged
	{
		public uint                                 Version       { get; set; }
		public IntPtr                               RenderSurface { get; set; }
		public ObservableCollection<PluginInfo_CLR> Plugins       { get; set; } = new ObservableCollection<PluginInfo_CLR>();
		public ObservableCollection<Sensor_CLR>     Sensors       { get; set; } = new ObservableCollection<Sensor_CLR>();
		public ObservableCollection<WidgetDesc_CLR> WidgetDescs   { get; set; } = new ObservableCollection<WidgetDesc_CLR>();

		// UI Helpers
		public bool IsSimulationRunning   { get; set; }
		public bool IsSimulationConnected { get; set; }

		public event PropertyChangedEventHandler PropertyChanged;
		public void NotifyPropertyChanged(string propertyName)
		{
			if (PropertyChanged != null)
				PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
		}

		// Messages
		public List<Message_> Messages = new List<Message_>();
	}
}
