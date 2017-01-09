namespace LCDHardwareMonitor.Presentation.Views
{
	using System;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Interop;
	using System.Windows.Media;

	/// <summary>
	/// Interaction logic for D3D11TestPage.xaml
	/// </summary>
	public partial class D3D11TestPage : UserControl
	{
		public D3D11TestPage()
		{
			InitializeComponent();

			CompositionTarget.Rendering += CompositionTarget_Rendering;
		}

		private void CompositionTarget_Rendering(object sender, EventArgs e)
		{
			bool success;

			success = Renderers.D3D11Renderer.Render();

			IntPtr pSurface = Renderers.D3D11Renderer.GetD3D9RenderSurface();
			d3dimg.Lock();
			d3dimg.SetBackBuffer(D3DResourceType.IDirect3DSurface9, pSurface, true);
			d3dimg.AddDirtyRect(new Int32Rect(0, 0, d3dimg.PixelWidth, d3dimg.PixelHeight));
			d3dimg.Unlock();
		}
	}
}