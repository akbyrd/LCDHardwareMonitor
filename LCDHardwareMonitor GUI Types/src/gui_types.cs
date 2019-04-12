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
	public string         Name    { get; set; }
	public PluginKind_CLR Kind    { get; set; }
	public string         Author  { get; set; }
	public uint           Version { get; set; }
}

public struct Sensor_CLR
{
	public string Name       { get; set; }
	public string Identifier { get; set; }
	public string String     { get; set; }
	public float  Value      { get; set; }
	public float  MinValue   { get; set; }
	public float  MaxValue   { get; set; }
}

public class SimulationState
{
	public uint                                 Version       { get; set; }
	public IntPtr                               RenderSurface { get; set; }
	public ObservableCollection<PluginInfo_CLR> Plugins       { get; } = new ObservableCollection<PluginInfo_CLR>();
	public ObservableCollection<Sensor_CLR>     Sensors       { get; } = new ObservableCollection<Sensor_CLR>();
	public ObservableCollection<string>         Widgets       { get; } = new ObservableCollection<string>();
}

// TODO: Is there any way to use native types directly?
