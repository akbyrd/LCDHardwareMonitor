using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Threading;

namespace LCDHardwareMonitor
{
	public partial class App : Application
	{
		public SimulationState SimulationState { get; private set; } = new SimulationState();

		DispatcherTimer timer;
		bool activated;

		App()
		{
			Activated += OnActivate;
			Exit += OnExit;
		}

		void OnActivate(object sender, EventArgs e)
		{
			if (activated) return;
			activated = true;

			IntPtr hwnd = new WindowInteropHelper(MainWindow).EnsureHandle();
			bool success = GUIInterop.Initialize(hwnd);
			if (!success)
			{
				Debugger.Break();
				Shutdown();
			}

			// TODO: Is it possible to get the animation rate or max refresh rate for the interval?
			timer = new DispatcherTimer();
			timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
			timer.Tick += OnTick;
			timer.Start();
		}

		void OnExit(object sender, ExitEventArgs e)
		{
			GUIInterop.Teardown();
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

			foreach (GUIMessage m in SimulationState.messages)
			{
				switch (m)
				{
					case GUIMessage.LaunchSim:
						if (processes.Length > 0) break;
						Process.Start("LCDHardwareMonitor.exe");
						break;

					// TODO: Why doesn't this work?
					// TODO: Send a pipe message
					case GUIMessage.CloseSim:
						foreach (Process p in processes)
							p.Close();
						break;

					case GUIMessage.KillSim:
						foreach (Process p in processes)
							p.Kill();
						break;
				}
			}
			SimulationState.messages.Clear();

			bool success = GUIInterop.Update(SimulationState);
			if (!success)
			{
				Debugger.Break();
				Shutdown();
			}
		}
	}
}
