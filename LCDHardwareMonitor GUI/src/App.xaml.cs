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

		// TODO: Probably want to run this once from OnActivate to put things in the correct state
		void OnTick(object sender, EventArgs e)
		{
			Process[] processes = Process.GetProcessesByName("LCDHardwareMonitor");
			bool running = processes.Length > 0;

			// TODO: Maybe generalize this to reuse for other requests
			long elapsed = SimulationState.ProcessStateTimer.ElapsedMilliseconds;
			ProcessState prevState = SimulationState.ProcessState;
			switch (SimulationState.ProcessState)
			{
				case ProcessState.Null:
					SimulationState.ProcessState = running ? ProcessState.Launched : ProcessState.Terminated;
					break;

				case ProcessState.Launching:
					if (running)
						SimulationState.ProcessState = ProcessState.Launched;
					else if (elapsed >= 1000)
						SimulationState.ProcessState = ProcessState.Terminated;
					break;

				case ProcessState.Launched:
					if (!running)
						SimulationState.ProcessState = ProcessState.Terminated;
					break;

				case ProcessState.Terminating:
					if (!running)
						SimulationState.ProcessState = ProcessState.Terminated;
					else if (elapsed >= 1000)
						SimulationState.ProcessState = ProcessState.Launched;
					break;

				case ProcessState.Terminated:
					if (running)
						SimulationState.ProcessState = ProcessState.Launched;
					break;
			}

			foreach (Message m in SimulationState.Messages)
			{
				switch (m.type)
				{
					case MessageType.LaunchSim:
						Process.Start("LCDHardwareMonitor.exe");
						SimulationState.ProcessState = ProcessState.Launching;
						break;

					case MessageType.TerminateSim:
						SimulationState.ProcessState = ProcessState.Terminating;
						break;

					case MessageType.ForceTerminateSim:
						foreach (Process p in processes)
							p.Kill();
						break;
				}
			}

			if (SimulationState.ProcessState != prevState)
			{
				SimulationState.ProcessStateTimer.Restart();
				SimulationState.NotifyPropertyChanged("");
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
