using System;
using System.IO.Pipes;
using System.Windows;
using System.Windows.Threading;

namespace LCDHardwareMonitor
{
    public partial class App : Application
    {
        DispatcherTimer       timer;
        //NamedPipeClientStream pipe;
        //IAsyncResult          pendingRead;

        public App()
        {
            GUIInterop.Initialize();

            // TODO: Is it possible to get the animation rate or max refresh rate for the interval?
            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
            timer.Tick += Tick;
            timer.Start();

            // TODO: Launch the simulation if necessary
            //pipe = new NamedPipeClientStream(".", "LCDHardwareMonitor GUI Pipe", PipeDirection.InOut);
            //pipe.ConnectAsync();
        }

        ~App()
        {
            GUIInterop.Teardown();
        }

        // NOTE: The 'runtime tools' overlay causes massive frame rate hitching
        public void Tick(object sender, EventArgs e)
        {
            //GUIInterop.Tick(pipe, ref pendingRead);
            GUIInterop.Update();
        }
    }
}
