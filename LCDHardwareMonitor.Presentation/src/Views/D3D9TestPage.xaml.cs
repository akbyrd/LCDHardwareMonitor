namespace LCDHardwareMonitor.Presentation.Views
{
	using System;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Interop;

	/// <summary>
	/// Interaction logic for D3D9TestPage.xaml
	/// </summary>
	public partial class D3D9TestPage : UserControl
	{
		public D3D9TestPage()
		{
			InitializeComponent();

			IntPtr pSurface = Shell.StaticLCDModel.RenderTexture;
			if (pSurface != IntPtr.Zero)
			{
				d3dimg.Lock();
				// Repeatedly calling SetBackBuffer with the same IntPtr is 
				// a no-op. There is no performance penalty.
				d3dimg.SetBackBuffer(D3DResourceType.IDirect3DSurface9, pSurface);
				d3dimg.AddDirtyRect(new Int32Rect(0, 0, d3dimg.PixelWidth, d3dimg.PixelHeight));
				d3dimg.Unlock();
			}
		}
	}
}