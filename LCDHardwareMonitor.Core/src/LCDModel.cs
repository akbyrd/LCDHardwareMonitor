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
		public IntPtr                          RenderTexture { get; set; }

		#endregion

		#region Constructor

		public LCDModel ()
		{
			Widgets   = new ObservableCollection<IWidget>();
			Drawables = new ObservableCollection<IDrawable>();
			Sources   = new ObservableCollection<object>();
		}

		#endregion
	}
}