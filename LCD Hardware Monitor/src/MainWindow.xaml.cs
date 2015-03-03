using System.Windows;
using System.Windows.Input;
using FirstFloor.ModernUI.Windows.Controls;

namespace LCDHardwareMonitor
{
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
	}
}