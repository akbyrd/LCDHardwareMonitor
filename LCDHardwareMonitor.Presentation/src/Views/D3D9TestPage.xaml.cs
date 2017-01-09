namespace LCDHardwareMonitor.Presentation.Views
{
	using System;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Interop;
	using System.Windows.Media;

	/// <summary>
	/// Interaction logic for D3D9TestPage.xaml
	/// </summary>
	public partial class D3D9TestPage : UserControl
	{
		public D3D9TestPage()
		{
			InitializeComponent();

			CompositionTarget.Rendering += new EventHandler(CompositionTarget_Rendering);
		}

		void CompositionTarget_Rendering(object sender, EventArgs e)
		{
			RenderingEventArgs args = (RenderingEventArgs)e;

			Renderers.D3D9Renderer.Render();

			IntPtr pSurface = IntPtr.Zero;
#if false
			Renderers.D3D9Renderer.GetD3D9BackBuffer(out pSurface);
#else
			Renderers.D3D9Renderer.GetD3D11BackBuffer(out pSurface);
#endif
			d3dimg.Lock();
			d3dimg.SetBackBuffer(D3DResourceType.IDirect3DSurface9, pSurface);
			d3dimg.AddDirtyRect(new Int32Rect(0, 0, d3dimg.PixelWidth, d3dimg.PixelHeight));
			d3dimg.Unlock();
		}
	}
}