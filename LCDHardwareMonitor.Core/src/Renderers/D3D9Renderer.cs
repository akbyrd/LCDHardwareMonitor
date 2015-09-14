namespace LCDHardwareMonitor.Renderers
{
	using System;
	using System.Runtime.InteropServices;

	public static class D3D9Renderer
	{
		private const string DLLPath = @"Renderers\D3D9\LCDHardwareMonitor.Rendering.Direct3D9.dll";

		[DllImport(DLLPath)]
		public static extern int GetBackBufferNoRef ( out IntPtr pSurface );

		[DllImport(DLLPath)]
		public static extern int SetSize ( uint width, uint height );

		[DllImport(DLLPath)]
		public static extern int SetAlpha ( bool useAlpha );

		[DllImport(DLLPath)]
		public static extern int SetNumDesiredSamples ( uint numSamples );

		[StructLayout(LayoutKind.Sequential)]
		public struct POINT
		{
			public POINT ( int x, int y )
			{
				this.x = x;
				this.y = y;
			}

			public int x;
			public int y;
		}

		[DllImport(DLLPath)]
		public static extern int SetAdapter ( POINT screenSpacePoint );

		[DllImport(DLLPath)]
		public static extern int Render ();

		[DllImport(DLLPath)]
		public static extern void Destroy ();
	}
}