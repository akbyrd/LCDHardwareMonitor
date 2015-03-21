using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Data;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor
{
	public class HardwareNode : INode, INotifyPropertyChanged
	{
		#region Constructor

		public HardwareNode ( IHardware hardware )
		{
			IsExpanded = true;
			Hardware = hardware;

			for ( int i = 0; i < hardware.SubHardware.Length; ++i )
				subHardware.Add(new HardwareNode(hardware.SubHardware[i]));

			for ( int i = 0; i < hardware.Sensors.Length; ++i )
				sensors.Add(new SensorNode(hardware.Sensors[i]));

			Children = new CompositeCollection() {
				new CollectionContainer() { Collection = Sensors },
				new CollectionContainer() { Collection = SubHardware },
			};
		}

		#endregion

		#region Public Properties

		public  bool IsExpanded
		{
			get { return isExpanded; }
			set { isExpanded = value; RaisePropertyChanged(); }
		}
		private bool isExpanded;

		public IHardware Hardware { get; private set; }

		public ReadOnlyCollection<HardwareNode> SubHardware { get { return subHardware.AsReadOnly(); } }
		private              List<HardwareNode> subHardware = new List<HardwareNode>();

		public ReadOnlyCollection<SensorNode> Sensors { get { return sensors.AsReadOnly(); } }
		private              List<SensorNode> sensors = new List<SensorNode>();

		public CompositeCollection Children { get; private set; }

		#endregion

		#region INotifyPropertyChanged Implementation

		public event PropertyChangedEventHandler PropertyChanged;

		private void RaisePropertyChanged ( [CallerMemberName] string propertyName = "" )
		{
			if ( PropertyChanged != null )
				PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
		}

		#endregion
	}
}