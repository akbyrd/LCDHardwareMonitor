using MahApps.Metro.Controls;
using System;
using System.Diagnostics;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;

namespace LCDHardwareMonitor.GUI
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

		private void LaunchSim_Click(object sender, RoutedEventArgs e)
		{
			switch (simState.ProcessState)
			{
				default: Debug.Assert(false); break;

				case ProcessState.Terminated:
					simState.Messages.Add(MessageType.LaunchSim);
					break;

				case ProcessState.Launched:
					simState.Messages.Add(MessageType.TerminateSim);
					break;
			}
		}

		private void ForceTerminateSim_Click(object sender, RoutedEventArgs e)
		{
			simState.Messages.Add(MessageType.ForceTerminateSim);
		}

		private void LoadPlugin_Click(object sender, RoutedEventArgs e)
		{
			Button button = (Button) sender;
			PluginInfo item = (PluginInfo) button.DataContext;

			PluginLoadState requestState;
			switch (item.LoadState)
			{
				default: Debug.Assert(false); return;

				case PluginLoadState.Broken:
					return;

				case PluginLoadState.Loaded:
					requestState = PluginLoadState.Unloaded;
					break;

				case PluginLoadState.Unloaded:
					requestState = PluginLoadState.Loaded;
					break;
			}

			SetPluginLoadStates data = new SetPluginLoadStates();
			data.kind      = item.Kind;
			data.ref_      = item.Ref;
			data.loadState = requestState;

			Message request = new Message(MessageType.SetPluginLoadState, data);
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
