//TODO: This should be destroyed when the page is no longer relevant (minized to tray?)

namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.Collections.ObjectModel;
	using OpenHardwareMonitor.Hardware;

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
					var disposable = (IDisposable) hardwareNodes[i];
					disposable.Dispose();

					hardwareNodes.RemoveAt(i);
					break;
				}
			}
		}

		#endregion
	}
}