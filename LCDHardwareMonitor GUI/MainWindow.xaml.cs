using MahApps.Metro.Controls;
using System;
using System.Globalization;
using System.Windows;
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
			App app = (App) Application.Current;

			LCDPreviewTexture.Lock();
			LCDPreviewTexture.SetBackBuffer(D3DResourceType.IDirect3DSurface9, app.SimulationState.RenderSurface, true);

			if (app.SimulationState.RenderSurface != IntPtr.Zero)
				LCDPreviewTexture.AddDirtyRect(new Int32Rect(0, 0, LCDPreviewTexture.PixelWidth, LCDPreviewTexture.PixelHeight));

			LCDPreviewTexture.Unlock();
		}

		// TODO: Push these requests in to a queue. Gray the button.
		private void LaunchSim_Click(object sender, RoutedEventArgs e)
		{
			simState.messages.Add(GUIMessage.LaunchSim);
		}

		private void CloseSim_Click(object sender, RoutedEventArgs e)
		{
			bool kill = Keyboard.Modifiers.HasFlag(ModifierKeys.Control);
			simState.messages.Add(kill ? GUIMessage.KillSim : GUIMessage.CloseSim);
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
