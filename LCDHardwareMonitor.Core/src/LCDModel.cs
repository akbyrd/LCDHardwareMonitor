namespace LCDHardwareMonitor
{
	using System.Collections.Generic;

	//TODO: Make this a standalone application. The WPF application should be separate and simply talk to this application.
	public class LCDModel
	{
		#region Public Interface

		public Vector2I        DisplaySize = new Vector2I(320, 240);
		public List<IWidget>   Widgets     = new List<IWidget>();
		public List<IDrawable> Drawables   = new List<IDrawable>();
		public List<object>    Sources     = new List<object>();

		#endregion
	}
}