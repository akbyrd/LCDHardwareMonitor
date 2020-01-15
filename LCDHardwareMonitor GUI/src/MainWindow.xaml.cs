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
			if (!preview.IsVisible) return;

			// TODO BUG: When the display turns off this line appears to stall indefinitely
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
					Interop.LaunchSim(simState);
					break;

				case ProcessState.Launched:
					Interop.TerminateSim(simState);
					break;
			}
		}

		private void ForceTerminateSim_Click(object sender, RoutedEventArgs e)
		{
			Interop.ForceTerminateSim(simState);
		}

		private void LoadPlugin_Click(object sender, RoutedEventArgs e)
		{
			Button button = (Button) sender;
			Plugin item = (Plugin) button.DataContext;

			switch (item.LoadState)
			{
				default: Debug.Assert(false); return;

				case PluginLoadState.Loaded:
					Interop.SetPluginLoadState(simState, item.Ref, PluginLoadState.Unloaded);
					break;

				case PluginLoadState.Unloaded:
					Interop.SetPluginLoadState(simState, item.Ref, PluginLoadState.Loaded);
					break;

				// Button is disabled for these states
				case PluginLoadState.Loading:
				case PluginLoadState.Unloading:
				case PluginLoadState.Broken:
					Debug.Assert(false);
					return;
			}
		}

		private void Window_KeyDown(object sender, KeyEventArgs e)
		{
			switch (e.Key)
			{
				default: return;

				case Key.Escape:
					Close();
					break;
			}
		}

		internal Point GetMousePosition()
		{
			// TODO: Make this better
			if (!preview.IsVisible) return new Point();

			Point previewPos = preview.PointToScreen(new Point());
			Point cursorPos = Win32.GetCursorPos();
			Point mousePos = (Point) (cursorPos - previewPos);
			mousePos.Y = preview.RenderSize.Height - 1 - mousePos.Y;
			return mousePos;
		}

		private void Preview_MouseDown(object sender, MouseButtonEventArgs e)
		{
			switch (e.ChangedButton)
			{
				default: return;

				case MouseButton.Left:
				case MouseButton.Middle:
				case MouseButton.Right:
				{
					var element = sender as UIElement;
					e.Handled = !element.IsMouseCaptured;
					Interop.MouseButtonChange(simState, GetMousePosition(), e.ChangedButton, e.ButtonState);

					// TODO: Proper logging
					bool success = Mouse.Capture(element);
					if (!success) Debug.Print("Warning: Failed to capture mouse");
					break;
				}
			}
		}

		private void Preview_MouseUp(object sender, MouseButtonEventArgs e)
		{
			switch (e.ChangedButton)
			{
				default: return;

				case MouseButton.Left:
				case MouseButton.Middle:
				case MouseButton.Right:
					var element = sender as UIElement;
					e.Handled = element.IsMouseCaptured;
					Interop.MouseButtonChange(simState, GetMousePosition(), e.ChangedButton, e.ButtonState);

					// TODO: Proper logging
					// TODO: Failure is logged when dragging from outside the window and releasing inside it
					bool success = Mouse.Capture(null);
					if (!success) Debug.Print("Warning: Failed to release mouse capture");
					break;
			}
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
