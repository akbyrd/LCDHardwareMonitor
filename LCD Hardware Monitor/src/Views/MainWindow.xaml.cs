namespace LCDHardwareMonitor.Views
{
	using System.Windows;
	using System.Windows.Input;
	using FirstFloor.ModernUI.Windows.Controls;

	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : ModernWindow
	{
		public MainWindow ()
		{
			InitializeComponent();
		}

		private void ModernWindow_KeyDown ( object sender, KeyEventArgs e )
		{
			if ( e.Key == Key.Escape )
				Application.Current.Shutdown();
		}

		#region Debugging

		/// <summary>
		/// Purely for providing a place for a breakpoint for easy debugging.
		/// </summary>
		private void MainWindow_MouseRightButtonDown ( object sender, MouseButtonEventArgs e ) { }

		#endregion
	}
}