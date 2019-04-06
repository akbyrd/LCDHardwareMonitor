using System;
using System.Windows;
using System.Windows.Threading;

namespace LCDHardwareMonitor
{
    public partial class App : Application
    {
        public SimulationState SimulationState { get; private set; } = new SimulationState();

        DispatcherTimer timer;

        public App()
        {
            // TODO: Handle failure
            GUIInterop.Initialize();

            // TODO: Is it possible to get the animation rate or max refresh rate for the interval?
            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
            timer.Tick += Tick;
            timer.Start();

            // TODO: Launch the simulation if necessary
        }

        ~App()
        {
            GUIInterop.Teardown();
        }

        public void Tick(object sender, EventArgs e)
        {
            // TODO: Handle failure
            GUIInterop.Update(SimulationState);
        }
    }
}
