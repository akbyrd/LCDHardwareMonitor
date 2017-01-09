namespace LCDHardwareMonitor.Renderers
{
	using System;
	using System.Runtime.InteropServices;

	public static class D3D11Renderer
	{
		private const string DLLPath = @"Renderers\D3D11\LCDHardwareMonitor.Rendering.Direct3D11.dll";
		//TODO: Research and double check calling convention stuff
		private const CallingConvention CallConv = CallingConvention.Cdecl;

		[DllImport(DLLPath, CallingConvention = CallConv)] public static extern bool Initialize(UInt16 width, UInt16 height, out IntPtr renderSurface);
		[DllImport(DLLPath, CallingConvention = CallConv)] public static extern bool Render();
		[DllImport(DLLPath, CallingConvention = CallConv)] public static extern void Teardown();
	}
}