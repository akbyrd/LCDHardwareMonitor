using System.Collections.Generic;
using System.Windows.Data;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor
{
	public class HardwareNode : INode
	{
		public bool                              IsExpanded  { get; set; }
		public CompositeCollection               Children    { get; private set; }
		public IReadOnlyCollection<HardwareNode> SubHardware { get { return subHardware.AsReadOnly(); } }
		public IReadOnlyCollection<SensorNode>   Sensors     { get { return sensors.AsReadOnly(); } }
		public IHardware                         Hardware    { get; private set; }

		private List<HardwareNode> subHardware = new List<HardwareNode>();
		private List<SensorNode> sensors = new List<SensorNode>();

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
	}
}
