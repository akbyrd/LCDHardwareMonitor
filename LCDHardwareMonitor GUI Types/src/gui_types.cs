using System;
using System.Collections.ObjectModel;

public enum PluginKind_CLR
{
	Null,
	Sensor,
	Widget,
}

public struct PluginInfo_CLR
{
	public String         Name    { get; set; }
	public PluginKind_CLR Kind    { get; set; }
	public String         Author  { get; set; }
	public UInt32         Version { get; set; }
}

public struct Sensor_CLR
{
	public String Name       { get; set; }
	public String Identifier { get; set; }
	public String String     { get; set; }
	public float  Value      { get; set; }
	public float  MinValue   { get; set; }
	public float  MaxValue   { get; set; }
}

public class SimulationState
{
	public UInt32                               Version { get; set; }
	public ObservableCollection<PluginInfo_CLR> Plugins { get; } = new ObservableCollection<PluginInfo_CLR>();
	public ObservableCollection<Sensor_CLR>     Sensors { get; } = new ObservableCollection<Sensor_CLR>();
	public ObservableCollection<string>         Widgets { get; } = new ObservableCollection<string>();
}

// TODO: Is there any way to use native types directly?
