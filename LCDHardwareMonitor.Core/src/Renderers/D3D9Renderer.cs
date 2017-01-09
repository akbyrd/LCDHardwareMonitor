namespace LCDHardwareMonitor.Renderers
{
	using System;
	using System.Runtime.InteropServices;

	public static class D3D9Renderer
	{
		private const string DLLPath = @"Renderers\D3D9\D3D9 Content.dll";
		private const CallingConvention CallConv = CallingConvention.Cdecl;

		[DllImport(DLLPath)] public static extern int Initialize();
		[DllImport(DLLPath)] public static extern int Render();
		[DllImport(DLLPath)] public static extern int GetD3D9BackBuffer(out IntPtr pSurface);
		[DllImport(DLLPath)] public static extern int GetD3D11BackBuffer(out IntPtr pSurface);
	}
}