using System;
using System.Windows;
using OpenHardwareMonitor.GUI;
using OpenHardwareMonitor.Hardware;

namespace LCD_Hardware_Monitor
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public MainWindow ()
		{
			InitializeComponent();

			Computer computer = new Computer();
			computer.Open();
			computer.Accept(new UpdateVisitor());

			//computer.GetReport();
			foreach ( var hardware in computer.Hardware )
			{
				PrintHardwareRecursively(hardware);
			}

			computer.Close();
		}

		private void PrintHardwareRecursively ( IHardware hardware, int depth = 0 )
		{
			string indent = new String('\t', depth);

			Console.WriteLine(indent + "   Name: " + hardware.Name);
			Console.WriteLine(indent + "   Type: " + hardware.HardwareType);
			Console.WriteLine(indent + "     ID: " + hardware.Identifier);

			Console.WriteLine(indent + "Sensors:");
			foreach ( var sensor in hardware.Sensors )
			{
				Console.WriteLine(indent + "\t Name: " + sensor.Name);
				Console.WriteLine(indent + "\tIndex: " + sensor.Index);
				Console.WriteLine(indent + "\t   ID: " + sensor.Identifier);
				Console.WriteLine(indent + "\tValue: " + sensor.Value);
			}

			foreach ( var subHardware in hardware.SubHardware )
			{
				PrintHardwareRecursively(subHardware, ++depth);
			}
		}
	}
}
