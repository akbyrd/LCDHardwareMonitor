namespace LCDHardwareMonitor
{
	using System;
	using System.Collections.Generic;
	using System.Collections.ObjectModel;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Threading;
	using LCDHardwareMonitor.Models;
	using OpenHardwareMonitor.Hardware;

	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application
	{
		#region Public Properties

		public static Computer Computer { get; private set; }

		public static ObservableCollection<IWidget> Widgets { get; private set; }

		//TODO: Read drawables from file
		//TODO: F5 to reload drawables from file (on a particular view?)
		public  static ReadOnlyObservableCollection<IDrawable> Drawables { get; private set; }
		private static         ObservableCollection<IDrawable> drawables = new ObservableCollection<IDrawable>();

		public static ReadOnlyDictionary<Type, Type> SettingViews;
		public static         Dictionary<Type, Type> settingViews = new Dictionary<Type, Type>();

		#endregion

		#region Constructor & Initialization

		protected override void OnStartup ( StartupEventArgs e )
		{
			base.OnStartup(e);

			ShutdownMode = ShutdownMode.OnMainWindowClose;

			Widgets = new ObservableCollection<IWidget>();
			Drawables = new ReadOnlyObservableCollection<IDrawable>(drawables);
			SettingViews = new ReadOnlyDictionary<Type, Type>(settingViews);

			Computer = InitializeComputer();
			InitializeTimer();

			Bootstrapper bootstrapper = new Bootstrapper();
			bootstrapper.Run();
		}

		#endregion

		#region Drawable Registration

		public static void RegisterDrawable<T> ( T drawable ) where T : IDrawable
		{
			drawables.Add(drawable);
		}

		#endregion

		#region View Registration

		public static void RegisterView<T, U> ()
			where T : DrawableSetting
			where U : UserControl
		{
			settingViews[typeof(T)] = typeof(U);
		}

		#endregion

		#region Sources

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

		#endregion

		#region Source Update Timer

		private void InitializeTimer ()
		{
			/* Update the OHM data at a regular interval as long as the
			 * application is running.
			 */
			var timer = new DispatcherTimer();
			//TODO: Settings
			timer.Interval = new TimeSpan(0, 0, 0, 0, 800);
			timer.Tick += OnTimerTick;
			timer.Start();
		}

		//TODO: Only update sensors that are actually in use
		private void OnTimerTick ( object sender, EventArgs e )
		{
			for ( int i = 0; i < Widgets.Count; ++i )
				Widgets[i].Update();
		}

		#endregion
	}
}