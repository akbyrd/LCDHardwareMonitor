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
                Application.Current.Shutdown();
            }

            // TODO: Is it possible to get the animation rate or max refresh rate for the interval?
            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
            timer.Tick += OnTick;
            timer.Start();

            // TODO: Launch the simulation if necessary
        }

        void OnExit(object sender, ExitEventArgs e)
        {
            GUIInterop.Teardown();
        }

        void OnTick(object sender, EventArgs e)
        {
            bool success = GUIInterop.Update(SimulationState);
            if (!success)
            {
                Debugger.Break();
                Application.Current.Shutdown();
            }
        }
    }
}
