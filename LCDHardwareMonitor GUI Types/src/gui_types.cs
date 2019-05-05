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
	public uint           Ref      { get; set; }
	public string         Name     { get; set; }
	public PluginKind_CLR Kind     { get; set; }
	public string         Author   { get; set; }
	public uint           Version  { get; set; }
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

public class SimulationState
{
	public uint                                 Version       { get; set; }
	public IntPtr                               RenderSurface { get; set; }
	public ObservableCollection<PluginInfo_CLR> Plugins       { get; } = new ObservableCollection<PluginInfo_CLR>();
	public ObservableCollection<Sensor_CLR>     Sensors       { get; } = new ObservableCollection<Sensor_CLR>();
	public ObservableCollection<WidgetDesc_CLR> WidgetDescs   { get; } = new ObservableCollection<WidgetDesc_CLR>();
}

// TODO: Is there any way to use native types directly?
