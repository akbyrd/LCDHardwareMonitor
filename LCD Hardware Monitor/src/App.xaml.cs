using System.Windows;

namespace LCD_Hardware_Monitor
{
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		WMIReader wmiReader = new WMIReader();
	}
}