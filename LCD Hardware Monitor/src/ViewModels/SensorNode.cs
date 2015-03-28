namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.ComponentModel;
	using OpenHardwareMonitor.Hardware;

	/// <summary>
	/// Exposes an <see cref="OpenHardwareMonitor.Hardware.ISensor"/> for
	/// display in XAML.
	/// </summary>
	public class SensorNode : INode, INotifyPropertyChanged
	{
		#region Constructor

		public SensorNode ( ISensor sensor )
		{
			Sensor = sensor;
		}

		#endregion

		#region Public Interface

		public ISensor Sensor { get; private set; }

		//OPTIMIZE: This is lazy. Not sure if it's a performance issue.
		/// <summary>
		/// Just flag the entire sensor as changed every update.
		/// </summary>
		public void Update ()
		{
			if ( PropertyChanged != null )
			{
				var args = new PropertyChangedEventArgs(null);
				PropertyChanged(this, args);
			}
		}

		#endregion

		#region INotifyPropertyChanged Implementation

		public event PropertyChangedEventHandler PropertyChanged;

		#endregion
	}
}