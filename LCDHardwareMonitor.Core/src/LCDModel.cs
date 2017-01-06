namespace LCDHardwareMonitor
{
	using System;
	using System.Collections.ObjectModel;

	public class LCDModel
	{
		#region Public Interface

		public ObservableCollection<IWidget>   Widgets       { get; private set; }
		public ObservableCollection<IDrawable> Drawables     { get; private set; }
		public ObservableCollection<object>    Sources       { get; private set; }
		public IntPtr                          RenderTexture { get; private set; }

		#endregion

		#region Constructor

		public LCDModel ()
		{
			Widgets   = new ObservableCollection<IWidget>();
			Drawables = new ObservableCollection<IDrawable>();
			Sources   = new ObservableCollection<object>();

			bool success;
			success = Renderers.D3D11Renderer.Initialize();
			RenderTexture = Renderers.D3D11Renderer.GetD3D9RenderTexture();
			success = Renderers.D3D11Renderer.Render();
			//Renderers.D3D11Renderer.Teardown();
		}

		#endregion
	}
}