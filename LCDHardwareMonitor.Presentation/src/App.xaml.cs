namespace LCDHardwareMonitor.Presentation
{
	using System.Windows;

	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		#region Constructor & Initialization

		protected override void OnStartup ( StartupEventArgs e )
		{
			base.OnStartup(e);

			ShutdownMode = ShutdownMode.OnMainWindowClose;

			var bootstrapper = new Bootstrapper();
			bootstrapper.Run();
		}

		#endregion
	}
}