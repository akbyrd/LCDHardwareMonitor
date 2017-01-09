namespace LCDHardwareMonitor
{
	using System.Collections.Generic;
	using LCDHardwareMonitor.Renderers;
	using System;

	//TODO: Make this a standalone application. The WPF application should be separate and simply talk to this application.
	public class LCDModel
	{
		#region Public Interface

		public Vector2I        RenderSize    = new Vector2I(320, 240);
		public IntPtr          RenderSurface = IntPtr.Zero;
		public List<IWidget>   Widgets       = new List<IWidget>();
		public List<IDrawable> Drawables     = new List<IDrawable>();
		public List<object>    Sources       = new List<object>();

		#endregion

#if true
		public LCDModel()
		{
			bool success;

			//TODO: Error checking
			//TODO: Render loop
			//success = D3D11Renderer.Initialize((UInt16) RenderSize.x, (UInt16) RenderSize.y, out RenderSurface);
			success = D3D11Renderer.Initialize();
			if (!success)
				throw new Exception("Uh-oh");

			RenderSurface = D3D11Renderer.GetD3D9RenderSurface();
		}

		~LCDModel()
		{
			D3D11Renderer.Teardown();
		}
#endif
	}
}