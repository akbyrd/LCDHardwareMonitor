//TODO: Change unfocused border color to gray matching window color (i.e. invisible)
//TODO: Change highlight color to match VS (darker blue)
//TODO: Property binding for the Sources link group

namespace LCDHardwareMonitor.Presentation.Views
{
	using System;
	using System.Windows.Input;
	using FirstFloor.ModernUI.Presentation;
	using FirstFloor.ModernUI.Windows.Controls;
	using Microsoft.Practices.Prism.Logging;

	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class Shell : ModernWindow
	{
		private readonly ILoggerFacade logger;

		internal Shell ( ILoggerFacade logger )
		{
			this.logger = logger;

			LCDModel = new LCDModel();
			StaticLCDModel = LCDModel;

			InitializeComponent();
		}

		private void ModernWindow_KeyDown ( object sender, KeyEventArgs e )
		{
			if ( e.Key == Key.Escape )
				Close();
		}

		#region Public Interface

		//HACK: Filthy hack until I implement proper ViewModel-first patterns
		public static LCDModel StaticLCDModel { get; private set; }

		public LCDModel LCDModel { get; private set; }

		public void RegisterSourceView ( string name, Uri uri )
		{
			string message = string.Format("Registering view. Name: {0}, URI: {1}", name, uri);
			logger.Log(message, Category.Info, Priority.Low);

			SourcesLinkGroup.Links.Add(new Link() {
				DisplayName = name,
				Source = uri
			});
		}

		#endregion

		#region Debugging

		/// <summary>
		/// Purely for providing a place for a breakpoint for easy debugging.
		/// </summary>
		private void MainWindow_MouseRightButtonDown ( object sender, MouseButtonEventArgs e ) { }

		#endregion
	}
}