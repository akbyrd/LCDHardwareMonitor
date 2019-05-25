using MahApps.Metro.Controls;
using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;

namespace LCDHardwareMonitor
{
	public partial class MainWindow : MetroWindow
	{
		SimulationState simState;

		public MainWindow()
		{
			simState = ((App) Application.Current).SimulationState;

			InitializeComponent();
			CompositionTarget.Rendering += CompositionTarget_Rendering;
		}

		private void CompositionTarget_Rendering(object sender, EventArgs e)
		{
			LCDPreviewTexture.Lock();
			LCDPreviewTexture.SetBackBuffer(D3DResourceType.IDirect3DSurface9, simState.RenderSurface, true);

			if (simState.RenderSurface != IntPtr.Zero)
				LCDPreviewTexture.AddDirtyRect(new Int32Rect(0, 0, LCDPreviewTexture.PixelWidth, LCDPreviewTexture.PixelHeight));

			LCDPreviewTexture.Unlock();
		}

		// TODO: Gray the button?
		private void LaunchSim_Click(object sender, RoutedEventArgs e)
		{
			simState.Messages.Add(MessageType.LaunchSim);
		}

		private void CloseSim_Click(object sender, RoutedEventArgs e)
		{
			bool kill = Keyboard.Modifiers.HasFlag(ModifierKeys.Control);
			simState.Messages.Add(kill ? MessageType.KillSim : MessageType.CloseSim);
		}

		private void LoadPlugin_Click(object sender, RoutedEventArgs e)
		{
			Button button = (Button) sender;
			PluginInfo_CLR item = (PluginInfo_CLR) button.DataContext;

			PluginLoadState_CLR requestState;
			switch (item.LoadState)
			{
				default:
				case PluginLoadState_CLR.Null:
				case PluginLoadState_CLR.Broken:
					return;

				case PluginLoadState_CLR.Loaded:
					requestState = PluginLoadState_CLR.Unloaded;
					break;

				case PluginLoadState_CLR.Unloaded:
					requestState = PluginLoadState_CLR.Loaded;
					break;
			}

			SetPluginLoadStates data = new SetPluginLoadStates();
			data.kind      = item.Kind;
			data.ref_      = item.Ref;
			data.loadState = requestState;

			Message_ request = new Message_(MessageType.SetPluginLoadState, data);
			simState.Messages.Add(request);
		}
	}

	public class VisibilityConverter : IValueConverter
	{
		public bool Invert { get; set; }

		public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
		{
			if (value is bool)
				return ((bool) value ^ Invert) ? Visibility.Visible : Visibility.Collapsed;
			return null;
		}

		public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
		{
			if (value is Visibility)
				return ((Visibility) value == Visibility.Collapsed ? false : true) ^ Invert;
			return null;
		}
	}
}
