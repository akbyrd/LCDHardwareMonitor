namespace LCDHardwareMonitor.Renderers
{
	using System.Runtime.InteropServices;

	public static class D3D9Renderer
	{
		private const string DLLPath = @"Renderers\D3D9\LCDHardwareMonitor.Rendering.Direct3D9.dll";

		[DllImport(DLLPath)]
		public static extern int Render();
	}
}