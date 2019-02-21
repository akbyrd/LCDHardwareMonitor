using System;
using System.IO;
using System.IO.Pipes;
using System.Windows;
using System.Windows.Threading;

namespace LCDHardwareMonitor
{
    public partial class App : Application
    {
        DispatcherTimer       timer;
        NamedPipeClientStream pipe;
        //MemoryStream          pipeOutput;

        public App()
        {
            // TODO: Is it possible to get the animation rate or max refresh rate for the interval?
            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1.0 / 60.0);
            timer.Tick += Tick;
            timer.Start();

            // TODO: Launch the simulation if necessary
            pipe = new NamedPipeClientStream(".", "LCDHardwareMonitor GUI Pipe", PipeDirection.InOut);
            pipe.ConnectAsync();
        }

        // NOTE: The 'runtime tools' overlay causes massive frame rate hitching
        public void Tick(object sender, EventArgs e)
        {
            if (!pipe.IsConnected) return;

            //pipe.CopyTo(pipeOutput);
            //Byte[] bytes = pipeOutput.GetBuffer();

            Console.WriteLine("GUI Tick");
            Byte[] bytes = new Byte[128];
            int read = pipe.Read(bytes, 0, bytes.Length);
            if (read > 0)
            {
                Console.Write("Read {0} bytes", read);
                Console.Write(" ({0}", bytes[0]);
                for (int i = 1; i < read; i++)
                    Console.Write(", {0}", bytes[i]);
                Console.WriteLine(")");
            }
        }
    }
}
