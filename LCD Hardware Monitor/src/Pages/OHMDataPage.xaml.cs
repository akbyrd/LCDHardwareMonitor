using System.Windows.Controls;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.Pages
{
	/// <summary>
	/// Interaction logic for OHMDataPage.xaml
	/// </summary>
	public partial class OHMDataPage : UserControl
	{
		private Computer computer;
		private TreeModel treeModel;

		public OHMDataPage ()
		{
			InitializeComponent();

			computer = InitializeComputer();
			treeModel = BuildSensorTree();
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

			treeModel.Root.Text = computer.ToString();

			//TODO: Stack approach
			foreach ( IHardware hardware in computer.Hardware )
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
			}

			foreach ( IHardware subHardware in hardware.SubHardware )
			{
				AddHardwareRecursively(hardwareNode, subHardware);
			}
		}
	}
}
