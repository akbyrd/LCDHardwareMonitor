using MahApps.Metro.Controls;
using System;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;

namespace LCDHardwareMonitor
{
    public partial class MainWindow : MetroWindow
    {
        public MainWindow()
        {
            InitializeComponent();
            CompositionTarget.Rendering += CompositionTarget_Rendering;
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            App app = (App) Application.Current;
            if (app.SimulationState.RenderSurface == IntPtr.Zero) return;

            LCDPreviewTexture.Lock();
            LCDPreviewTexture.SetBackBuffer(D3DResourceType.IDirect3DSurface9, app.SimulationState.RenderSurface, true);
            LCDPreviewTexture.AddDirtyRect(new Int32Rect(0, 0, LCDPreviewTexture.PixelWidth, LCDPreviewTexture.PixelHeight));
            LCDPreviewTexture.Unlock();
        }
    }
}
