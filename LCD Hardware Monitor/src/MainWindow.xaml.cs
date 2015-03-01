using System.Windows;
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

		private void ModernWindow_KeyDown ( object sender, System.Windows.Input.KeyEventArgs e )
		{
			Application.Current.Shutdown();
		}
	}
}