namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.ComponentModel;
	using OpenHardwareMonitor.Hardware;

	/// <summary>
	/// Exposes an <see cref="OpenHardwareMonitor.Hardware.ISensor"/> for
	/// display in XAML.
	/// </summary>
	public class SensorNode : INode, INotifyPropertyChanged, IDisposable
	{
		#region Constructor

		public SensorNode ( ISensor sensor )
		{
			Sensor = sensor;

			App.Tick += OnTick;
		}

		#endregion

		#region Public Interface

		public ISensor Sensor { get; private set; }

		#endregion

		#region INotifyPropertyChanged Implementation

		public event PropertyChangedEventHandler PropertyChanged;

		//OPTIMIZE: This is lazy. Not sure if it's a performance issue.
		/// <summary>
		/// Just flag the entire sensor as changed every tick.
		/// </summary>
		private void OnTick ()
		{
			if ( PropertyChanged != null )
			{
				var args = new PropertyChangedEventArgs(null);
				PropertyChanged(this, args);
			}
		}

		#endregion

		#region IDisposable Implementation

		/// <summary>
		/// Cleanup when this node is removed from the tree. Namely, unregister
		/// from events to prevent memory leaks.
		/// </summary>
		void IDisposable.Dispose ()
		{
			App.Tick -= OnTick;
		}

		#endregion
	}
}