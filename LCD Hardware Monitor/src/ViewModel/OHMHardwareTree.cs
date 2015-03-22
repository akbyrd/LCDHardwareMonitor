using System.Collections.ObjectModel;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.ViewModel
{
	public class OHMHardwareTree
	{
		#region Constructor

		public OHMHardwareTree ( Computer computer )
		{
			HardwareNodes = new ReadOnlyObservableCollection<HardwareNode>(hardwareNodes);

			IHardware[] hardware = computer.Hardware;
			for ( int i = 0; i < hardware.Length; ++i )
				hardwareNodes.Add(new HardwareNode(hardware[i]));

			computer.HardwareAdded   += OnHardwareAdded;
			computer.HardwareRemoved += OnHardwareRemoved;
		}

		#endregion

		#region Public Properties

		public ReadOnlyObservableCollection<HardwareNode> HardwareNodes { get; private set; }
		private        ObservableCollection<HardwareNode> hardwareNodes = new ObservableCollection<HardwareNode>();

		#endregion

		#region Adding & Removing Hardware

		private void OnHardwareAdded ( IHardware hardware )
		{
			hardwareNodes.Add(new HardwareNode(hardware));
		}

		private void OnHardwareRemoved ( IHardware hardware )
		{
			for ( int i = 0; i < hardwareNodes.Count; ++i )
			{
				if ( hardwareNodes[i].Hardware == hardware )
				{
					hardwareNodes[i].RemovedFromTree();
					hardwareNodes.RemoveAt(i);
					break;
				}
			}
		}

		#endregion
	}
}