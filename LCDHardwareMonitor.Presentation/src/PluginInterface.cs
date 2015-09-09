namespace LCDHardwareMonitor.Presentation
{
	using System;
	using LCDHardwareMonitor.Presentation.Properties;
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
			string message = string.Format(Resources.Plugin_FwdViewReg, name, uri);
			logger.Log(message, Category.Info, Priority.Low);

			shell.RegisterSourceView(name, uri);
		}

		public void RegisterDrawable ( IDrawable drawable )
		{
			string message = string.Format(Resources.Plugin_FwdDrawableReg, drawable.GetType());
			logger.Log(message, Category.Info, Priority.Low);

			shell.LCDModel.Drawables.Add(drawable);
		}
	}
}