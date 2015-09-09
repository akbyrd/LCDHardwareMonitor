namespace LCDHardwareMonitor.Sources.OpenHardwareMonitor
{
	using System;
	using System.ComponentModel.Composition;
	using LCDHardwareMonitor.Presentation;
	using Microsoft.Practices.Prism.MefExtensions.Modularity;
	using Microsoft.Practices.Prism.Modularity;

	/// <summary>
	/// Responsible for registering the source and any views in this plugin
	/// with the main application.
	/// </summary>
	[ModuleExport(typeof(OHMModule))]
	public class OHMModule : IModule
	{
		private readonly PluginInterface pluginInterface;

		#region Constructor

		[ImportingConstructor]
		public OHMModule ( PluginInterface pluginInterface )
		{
			this.pluginInterface = pluginInterface;
		}

		#endregion

		#region IModule Implementation

		public void Initialize ()
		{
			//TODO: Register source
			//TODO: Register designer view view
			//TODO: Register settings view

			//TODO: Register source view
			var viewURI = new Uri("/LCDHardwareMonitor.Sources.OpenHardwareMonitor;component/src/Views/OHMSourceView.xaml", UriKind.Relative);
			var viewName = "Open Hardware Monitor";
			pluginInterface.RegisterSourceView(viewName, viewURI);
		}

		#endregion
	}
}