using System;
using System.Collections.ObjectModel;

public enum PluginKind
{
    Null,
    Sensor,
    Widget,
}

public struct PluginInfo_CLR
{
    public String     Name    { get; set; }
    public PluginKind Kind    { get; set; }
    public String     Author  { get; set; }
    public UInt32     Version { get; set; }
}

public class SimulationState
{
    public UInt32                                Version         { get; set; }
    public ObservableCollection<PluginInfo_CLR>  Plugins         { get; } = new ObservableCollection<PluginInfo_CLR>();
    public ObservableCollection<string>          Sensors         { get; } = new ObservableCollection<string>();
    public ObservableCollection<string>          Widgets         { get; } = new ObservableCollection<string>();
}

// TODO: Is there any way to use native types directly?
