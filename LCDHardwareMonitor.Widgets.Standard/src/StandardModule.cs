namespace LCDHardwareMonitor.Widgets.Standard
{
	using System.ComponentModel.Composition;
	using LCDHardwareMonitor.Presentation;
	using Microsoft.Practices.Prism.MefExtensions.Modularity;
	using Microsoft.Practices.Prism.Modularity;

	[ModuleExport(typeof(StandardModule))]
	public class StandardModule : IModule
	{
		#region Constructor

		private PluginInterface pluginInterface;

		[ImportingConstructor]
		public StandardModule ( PluginInterface pluginInterface )
		{
			this.pluginInterface = pluginInterface;
		}

		#endregion

		#region IModule Implementation

		public void Initialize ()
		{
			pluginInterface.RegisterDrawable(new StaticText());
		}

		#endregion
	}
}