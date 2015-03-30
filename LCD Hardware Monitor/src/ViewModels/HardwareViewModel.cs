namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.Collections.ObjectModel;
	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using System.Windows.Data;
	using OpenHardwareMonitor.Hardware;

	/// <summary>
	/// Exposes an <see cref="OpenHardwareMonitor.Hardware.IHardware"/> for
	/// display in XAML.
	/// </summary>
	public class HardwareViewModel : INode, INotifyPropertyChanged, IDisposable
	{
		#region Constructor

		public HardwareViewModel ( IHardware hardware )
		{
			Hardware = hardware;

			for ( int i = 0; i < hardware.SubHardware.Length; ++i )
				subHardware.Add(new HardwareViewModel(hardware.SubHardware[i]));
			SubHardware = new ReadOnlyObservableCollection<HardwareViewModel>(subHardware);

			for ( int i = 0; i < hardware.Sensors.Length; ++i )
				sensors.Add(new SensorViewModel(hardware.Sensors[i]));
			 Sensors = new ReadOnlyObservableCollection<SensorViewModel>(sensors);

			Hardware.SensorAdded   += OnSensorAdded;
			Hardware.SensorRemoved += OnSensorRemoved;

			Children = new CompositeCollection() {
				new CollectionContainer() { Collection = Sensors },
				new CollectionContainer() { Collection = SubHardware },
			};
		}

		#endregion

		#region Public Interface

		public  bool IsExpanded
		{
			get { return isExpanded; }
			set
			{
				if ( isExpanded != value )
				{
					isExpanded = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private bool isExpanded = true;

		public IHardware Hardware { get; private set; }

		public ReadOnlyObservableCollection<HardwareViewModel> SubHardware { get; private set; }
		private        ObservableCollection<HardwareViewModel> subHardware = new ObservableCollection<HardwareViewModel>();

		public ReadOnlyObservableCollection<SensorViewModel> Sensors { get; private set; }
		private        ObservableCollection<SensorViewModel> sensors = new ObservableCollection<SensorViewModel>();

		public CompositeCollection Children { get; private set; }

		public void Update ()
		{
			for ( int i = 0; i < subHardware.Count; ++i )
				subHardware[i].Update();

			for ( int i = 0; i < sensors.Count; ++i )
				sensors[i].Update();
		}

		#endregion

		#region Adding & Removing Sensors

		private void OnSensorAdded ( ISensor sensor )
		{
			sensors.Add(new SensorViewModel(sensor));
		}

		private void OnSensorRemoved ( ISensor sensor )
		{
			for ( int i = 0; i < sensors.Count; ++i )
			{
				if ( sensors[i].Sensor == sensor )
				{
					var disposable = (IDisposable) sensors[i];
					disposable.Dispose();

					sensors.RemoveAt(i);
					break;
				}
			}
		}

		#endregion

		#region INotifyPropertyChanged Implementation

		public event PropertyChangedEventHandler PropertyChanged;

		private void RaisePropertyChangedEvent ( [CallerMemberName] string propertyName = "" )
		{
			if ( PropertyChanged != null )
			{
				var args = new PropertyChangedEventArgs(propertyName);
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
			Hardware.SensorAdded   -= OnSensorAdded;
			Hardware.SensorRemoved -= OnSensorRemoved;
		}

		#endregion
	}
}