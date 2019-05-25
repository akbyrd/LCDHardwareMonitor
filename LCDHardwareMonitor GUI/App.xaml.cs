using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Threading;

namespace LCDHardwareMonitor.GUI
{
	public partial class App : Application
	{
		public SimulationState SimulationState { get; private set; }

		DispatcherTimer timer;
		bool activated;

		App()
		{
			SimulationState = new SimulationState();
			Activated += OnActivate;
			Exit += OnExit;
		}

		void OnActivate(object sender, EventArgs e)
		{
			if (activated) return;
			activated = true;

			IntPtr hwnd = new WindowInteropHelper(MainWindow).EnsureHandle();
			bool success = Interop.Initialize(hwnd);
			if (!success)
			{
				Debugger.Break();
				Shutdown();
			}

			// TODO: Is it possible to get the animation rate or max refresh rate for the interval?
			timer = new DispatcherTimer(DispatcherPriority.Input);
			timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
			timer.Tick += OnTick;
			timer.Start();
		}

		void OnExit(object sender, ExitEventArgs e)
		{
			Interop.Teardown();
		}

		void OnTick(object sender, EventArgs e)
		{
			Process[] processes = Process.GetProcessesByName("LCDHardwareMonitor");
			bool running = processes.Length > 0;
			if (running != SimulationState.IsSimulationRunning)
			{
				SimulationState.IsSimulationRunning = running;
				SimulationState.NotifyPropertyChanged("");
			}

			foreach (Message_ m in SimulationState.Messages)
			{
				switch (m.type)
				{
					case MessageType.LaunchSim:
						if (processes.Length > 0) break;
						Process.Start("LCDHardwareMonitor.exe");
						break;

					// TODO: Why doesn't this work?
					// TODO: Send a pipe message
					case MessageType.CloseSim:
						foreach (Process p in processes)
							p.Close();
						break;

					case MessageType.KillSim:
						foreach (Process p in processes)
							p.Kill();
						break;
				}
			}

			bool success = Interop.Update(SimulationState);
			if (!success)
			{
				Debugger.Break();
				Shutdown();
			}

			SimulationState.Messages.Clear();
		}
	}
}
