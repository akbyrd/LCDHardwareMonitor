//TODO: This should be destroyed when the page is no longer relevant (minized to tray?)

namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.Collections.ObjectModel;
	using System.Windows.Threading;
	using OpenHardwareMonitor.GUI;
	using OpenHardwareMonitor.Hardware;

	public class OHMSourceViewModel
	{
		#region Constructor

		public OHMSourceViewModel ()
		{
			HardwareNodes = new ReadOnlyObservableCollection<HardwareViewModel>(hardwareNodes);

			IHardware[] hardware = App.Computer.Hardware;
			for ( int i = 0; i < hardware.Length; ++i )
				hardwareNodes.Add(new HardwareViewModel(hardware[i]));

			App.Computer.HardwareAdded   += OnHardwareAdded;
			App.Computer.HardwareRemoved += OnHardwareRemoved;

			var timer = new DispatcherTimer();
			timer.Interval = new TimeSpan(0, 0, 0, 0, 800);
			timer.Tick += OnTick;
			timer.Start();
		}

		#endregion

		#region Update

		private IVisitor updateVisitor = new UpdateVisitor();

		private void OnTick ( object sender, EventArgs e )
		{
			App.Computer.Accept(updateVisitor);

			for ( int i = 0; i < hardwareNodes.Count; ++i )
				hardwareNodes[i].Update();
		}

		#endregion

		#region Public Interface

		public ReadOnlyObservableCollection<HardwareViewModel> HardwareNodes { get; private set; }
		private        ObservableCollection<HardwareViewModel> hardwareNodes = new ObservableCollection<HardwareViewModel>();

		#endregion

		#region Adding & Removing Hardware

		private void OnHardwareAdded ( IHardware hardware )
		{
			hardwareNodes.Add(new HardwareViewModel(hardware));
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