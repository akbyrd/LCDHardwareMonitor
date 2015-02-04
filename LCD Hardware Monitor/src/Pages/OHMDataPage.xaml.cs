using System.Windows.Controls;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.Pages
{
	public class StringList : System.Collections.Generic.List<string> { }

	/// <summary>
	/// Interaction logic for OHMDataPage.xaml
	/// </summary>
	public partial class OHMDataPage : UserControl
	{
		private Computer  Computer  { get; set; }
		private TreeModel TreeModel { get; set; }

		public StringList stuff { get; set; }

		public OHMDataPage ()
		{
			Computer = InitializeComputer();
			TreeModel = BuildSensorTree();

			stuff = new StringList();
			stuff.Add("New String 1");
			stuff.Add("New String 2");
			stuff.Add("New String 3");
			stuff.Add("New String 4");
			stuff.Add("New String 5");
			stuff.Add("New String 6");

			//DataContext = this;
			InitializeComponent();
		}

		private Computer InitializeComputer ()
		{
			Computer computer = new Computer();
			computer.Open();

			//ENABLE ALL THE THINGS!
			computer.CPUEnabled = true;
			computer.FanControllerEnabled = true;
			computer.GPUEnabled = true;
			computer.HDDEnabled = true;
			computer.MainboardEnabled = true;
			computer.RAMEnabled = true;

			return computer;
		}

		private TreeModel BuildSensorTree ()
		{
			TreeModel treeModel = new TreeModel();

			treeModel.Root.Text = Computer.ToString();

			//TODO: Stack approach
			foreach ( IHardware hardware in Computer.Hardware )
			{
				AddHardwareRecursively(treeModel.Root, hardware);
			}

			return treeModel;
		}

		private void AddHardwareRecursively ( Node parent, IHardware hardware, int depth = 0 )
		{
			Node hardwareNode = new Node();
			hardwareNode.Source = hardware;
			hardwareNode.Text = hardware.Name + " (" + hardware.HardwareType + ")";

			parent.Nodes.Add(hardwareNode);

			foreach ( ISensor sensor in hardware.Sensors )
			{
				Node sensorNode = new Node();
				sensorNode.Source = sensor;
				sensorNode.Text = sensor.Name + " (" + sensor.SensorType + ") = " + sensor.Value;

				hardwareNode.Nodes.Add(sensorNode);
			}

			foreach ( IHardware subHardware in hardware.SubHardware )
			{
				AddHardwareRecursively(hardwareNode, subHardware);
			}
		}
	}
}
