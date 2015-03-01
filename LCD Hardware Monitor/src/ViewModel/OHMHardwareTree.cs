using System.Collections.Generic;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.ViewModel
{
	public class OHMHardwareTree
	{
		public IReadOnlyCollection<HardwareNode> HardwareNodes { get { return hardwareNodes.AsReadOnly(); } }

		private List<HardwareNode> hardwareNodes = new List<HardwareNode>();

		public OHMHardwareTree ()
		{
			IHardware[] hardware = InitializeComputer().Hardware;

			for ( int i = 0; i < hardware.Length; ++i )
				hardwareNodes.Add(new HardwareNode(hardware[i]));
		}

		private Computer InitializeComputer ()
		{
			Computer computer = new Computer();
			computer.Open();

			//TODO: Saved settings fodder
			//ENABLE ALL THE THINGS!
			computer.CPUEnabled = true;
			computer.FanControllerEnabled = true;
			computer.GPUEnabled = true;
			computer.HDDEnabled = true;
			computer.MainboardEnabled = true;
			computer.RAMEnabled = true;

			return computer;
		}
	}
}