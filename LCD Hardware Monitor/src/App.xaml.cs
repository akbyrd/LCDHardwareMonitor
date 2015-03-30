namespace LCDHardwareMonitor
{
	using System;
	using System.Collections.ObjectModel;
	using System.Windows;
	using System.Windows.Threading;
	using OpenHardwareMonitor.Hardware;

	//TODO: Change unfocused border color to gray matching window color (i.e. invisible)
	//TODO: Change highlight color to match VS (darker blue)

	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		public static Computer Computer { get; private set; }

		public static ObservableCollection<IWidget> Widgets { get; private set; }

		//TODO: Read drawables from file
		//TODO: F5 to reload drawables from file
		public  static ReadOnlyObservableCollection<IDrawable> Drawables { get; private set; }
		private static         ObservableCollection<IDrawable> drawables = new ObservableCollection<IDrawable>();

		public App ()
		{
			Computer = InitializeComputer();

			//DEBUG
			drawables.Add(new StaticText());

			Widgets = new ObservableCollection<IWidget>();
			Drawables = new ReadOnlyObservableCollection<IDrawable>(drawables);

			/* Update the OHM data at a regular interval as long as the
			 * application is running.
			 */
			var timer = new DispatcherTimer();
			//TODO: Settings
			timer.Interval = new TimeSpan(0, 0, 0, 0, 800);
			timer.Tick += OnTick;
			timer.Start();
		}

		//TODO: Only update sensors that are actually in use
		private void OnTick ( object sender, EventArgs e )
		{
			for ( int i = 0; i < Widgets.Count; ++i )
				Widgets[i].Update();
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