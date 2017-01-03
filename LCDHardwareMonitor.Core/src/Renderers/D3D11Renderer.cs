namespace LCDHardwareMonitor.Renderers
{
	using System.Runtime.InteropServices;

	public static class D3D11Renderer
	{
		private const string DLLPath = @"Renderers\D3D11\LCDHardwareMonitor.Rendering.Direct3D11.dll";

		[DllImport(DLLPath)]
		public static extern bool Initialize();
	}
}