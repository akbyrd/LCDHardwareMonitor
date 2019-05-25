using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;

namespace LCDHardwareMonitor.GUI
{
	public enum PluginKind
	{
		Null,
		Sensor,
		Widget,
	}

	public enum PluginLoadState
	{
		Null,
		Loaded,
		Unloaded,
		Broken,
	}

	public struct PluginInfo
	{
		public uint       Ref     { get; set; }
		public string     Name    { get; set; }
		public PluginKind Kind    { get; set; }
		public string     Author  { get; set; }
		public uint       Version { get; set; }

		public PluginLoadState LoadState { get; set; } // TODO: Probably belongs in another struct
	}

	public struct Sensor
	{
		public uint   PluginRef  { get; set; }
		public uint   Ref        { get; set; }
		public string Name       { get; set; }
		public string Identifier { get; set; }
		public string Format     { get; set; }
		public float  Value      { get; set; }
	}

	public struct WidgetDesc
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
		public PluginKind      kind;
		public uint            ref_;
		public PluginLoadState loadState;
	};

	public class SimulationState : INotifyPropertyChanged
	{
		public uint                             Version       { get; set; }
		public IntPtr                           RenderSurface { get; set; }
		public ObservableCollection<PluginInfo> Plugins       { get; set; } = new ObservableCollection<PluginInfo>();
		public ObservableCollection<Sensor>     Sensors       { get; set; } = new ObservableCollection<Sensor>();
		public ObservableCollection<WidgetDesc> WidgetDescs   { get; set; } = new ObservableCollection<WidgetDesc>();

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
