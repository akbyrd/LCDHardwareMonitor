namespace LCDHardwareMonitor.Presentation
{
	using System;
	using LCDHardwareMonitor.Presentation.Views;
	using Microsoft.Practices.Prism.Logging;

	public class PluginInterface
	{
		private readonly Shell shell;
		private readonly ILoggerFacade logger;

		public PluginInterface ( Shell shell, ILoggerFacade logger )
		{
			this.shell  = shell;
			this.logger = logger;
		}

		public void RegisterSourceView ( string name, Uri uri )
		{
			//TODO: Resources
			const string fowardingViewRegistrationMsg = "Forwarding view registration. Name: {0}, URI: {1}";

			string message = string.Format(fowardingViewRegistrationMsg, name, uri);
			logger.Log(message, Category.Info, Priority.Low);

			shell.RegisterSourceView(name, uri);
		}

		public void RegisterDrawable ( IDrawable drawable )
		{
			//TODO: Resources
			const string fowardingDrawableRegistrationMsg = "Forwarding drawable registration. Type: {0}";

			string message = string.Format(fowardingDrawableRegistrationMsg, drawable.GetType());
			logger.Log(message, Category.Info, Priority.Low);

			shell.LCDModel.Drawables.Add(drawable);
		}
	}
}