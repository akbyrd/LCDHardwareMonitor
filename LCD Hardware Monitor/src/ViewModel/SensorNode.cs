using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor
{
	public class SensorNode : INode
	{
		public ISensor Sensor { get; private set; }

		public SensorNode ( ISensor sensor )
		{
			Sensor = sensor;
		}
	}
}