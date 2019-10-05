using System;
using System.Diagnostics;
using System.Reflection;
using System.Windows;
using System.Windows.Input;
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
			DisableTabletSupport();
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

			SimulationState.ProcessStateTimer.Start();
			OnTick(null, null);

			// TODO: Setting this to 60 yields 30 FPS. 70 yields 60 FPS and that seems to be the cap.
			timer = new DispatcherTimer(DispatcherPriority.Send);
			timer.Interval = TimeSpan.FromSeconds(1.0 / 70.0);
			timer.Tick += OnTick;
			timer.Start();
		}

		void OnExit(object sender, ExitEventArgs e)
		{
			// DEBUG: Only want this during development
			Interop.TerminateSim(SimulationState);
			OnTick(null, null);
			Interop.Teardown();
		}

		void OnTick(object sender, EventArgs e)
		{
			Process[] processes = Process.GetProcessesByName("LCDHardwareMonitor");
			bool running = processes.Length > 0;

			// TODO: Maybe generalize this to reuse for other requests
			long elapsed = SimulationState.ProcessStateTimer.ElapsedMilliseconds;
			switch (SimulationState.ProcessState)
			{
				case ProcessState.Null:
					// NOTE: Slightly faster to use "Multiple startup projects" when developing
					if (running)
						Interop.SetProcessState(SimulationState, ProcessState.Launched);
					else
						Interop.LaunchSim(SimulationState);
					break;

				case ProcessState.Launching:
					if (running)
						Interop.SetProcessState(SimulationState, ProcessState.Launched);
					else if (elapsed >= 1000)
						Interop.SetProcessState(SimulationState, ProcessState.Terminated);
					break;

				case ProcessState.Launched:
					if (!running)
						Interop.SetProcessState(SimulationState, ProcessState.Terminated);
					break;

				case ProcessState.Terminating:
					if (!running)
						Interop.SetProcessState(SimulationState, ProcessState.Terminated);
					else if (elapsed >= 1000)
						Interop.SetProcessState(SimulationState, ProcessState.Launched);
					break;

				case ProcessState.Terminated:
					if (running)
						Interop.SetProcessState(SimulationState, ProcessState.Launched);
					break;
			}

			// TODO: Clear mouse pos when minimized / not visible?
			var mainWindow = (MainWindow) MainWindow;
			if (mainWindow != null)
			{
				Point mousePos = mainWindow.GetMousePosition();
				Interop.MouseMove(SimulationState, mousePos);
			}

			bool success = Interop.Update(SimulationState);
			if (!success)
			{
				Debugger.Break();
				Shutdown();
			}
		}

		void DisableTabletSupport()
		{
			// NOTE: This is a filthy hack to disable tablet support because it breaks mouse capture.
			// This is a problem with *all* WPF applications in Windows 10. Nice work Microsoft.
			// https://github.com/dotnet/wpf/issues/1323
			// https://developercommunity.visualstudio.com/content/problem/573655/wpf-mouse-capture-issue-windows-10-1903.html
			// https://docs.microsoft.com/en-us/dotnet/framework/wpf/advanced/disable-the-realtimestylus-for-wpf-applications

			TabletDeviceCollection devices = Tablet.TabletDevices;
			if (devices.Count > 0)
			{
				object stylusLogic = typeof(InputManager).InvokeMember("StylusLogic",
					BindingFlags.GetProperty | BindingFlags.Instance | BindingFlags.NonPublic,
					null, InputManager.Current, null);

				if (stylusLogic != null)
				{
					Type stylusLogicType = stylusLogic.GetType();
					var args = new object[] { (uint) 0 };

					// NOTE: The loop below may not work on some systems, which could cause an infinite
					// loop. Also, it appears the remove call sometimes needs to happen several times
					// for a device to be removed.
					int maxTries = 4 * devices.Count;
					while (devices.Count > 0 && maxTries-- > 0)
					{
						stylusLogicType.InvokeMember("OnTabletRemoved",
							BindingFlags.InvokeMethod | BindingFlags.Instance | BindingFlags.NonPublic,
							null, stylusLogic, args);
					}
				}
			}
		}
	}
}
