using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.Sources.OpenHardwareMonitor
{
	using System;
	using System.Windows.Threading;

	public class OHMUpdateVisitor : IVisitor
	{
		public Computer Computer { get { return computer; } }
		private static Computer computer;

		public void VisitComputer ( IComputer computer )
		{
			computer.Traverse(this);
		}

		public void VisitHardware ( IHardware hardware )
		{
			hardware.Update();
			foreach ( IHardware subHardware in hardware.SubHardware )
				subHardware.Accept(this);
		}

		public void VisitSensor ( ISensor sensor ) { }

		public void VisitParameter ( IParameter parameter ) { }

		public OHMUpdateVisitor ()
		{
			if ( computer == null )
				computer = InitializeComputer();

			InitializeTimer();
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
			VisitComputer(Computer);
		}

		#endregion
	}
}