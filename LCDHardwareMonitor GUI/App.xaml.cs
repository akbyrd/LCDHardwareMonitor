using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Threading;

namespace LCDHardwareMonitor
{
    public partial class App : Application
    {
        DispatcherTimer timer;
        public ObservableCollection<Source> Sources { get; set; }

        public App()
        {
            Sources = new ObservableCollection<Source>();
            //for (int i = 0; i < 10; i++)
            //    Sources.Add(new Source() { name = "Some Source " + i, author = "Some Author " + i, version = 10 + (uint) i });

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

        // NOTE: The 'runtime tools' overlay causes massive frame rate hitching
        public void Tick(object sender, EventArgs e)
        {
            // TODO: Handle failure
            GUIInterop.Update(Sources);
        }
    }
}
