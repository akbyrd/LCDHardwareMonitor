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
using sMouseButton = System.Windows.Input.MouseButton;

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

		private Point GetMousePosition()
		{
			Point previewPos = preview.PointToScreen(new Point());
			Point cursorPos = Win32.GetCursorPos();
			Point mousePos = (Point) (cursorPos - previewPos);
			mousePos.Y = preview.RenderSize.Height - 1 - mousePos.Y;
			return mousePos;
		}

		// TODO: Proper logging
		// TODO: Clear mouse pos when minimized / not visible?
		internal void OnMouseMove()
		{
			Point mousePos = GetMousePosition();
			if (mousePos != simState.MousePos)
			{
				simState.Messages.Add(new Message(MessageType.MouseMove, mousePos));
				simState.MousePos = mousePos;
				simState.NotifyPropertyChanged("");
			}
		}

		private new void MouseDown(object sender, MouseButtonEventArgs e)
		{
			switch (e.ChangedButton)
			{
				default: return;

				case sMouseButton.Left:
				case sMouseButton.Middle:
				case sMouseButton.Right:
				{
					var element = sender as UIElement;
					e.Handled = !element.IsMouseCaptured;
					bool success = Mouse.Capture(element);
					if (!success) Debug.Print("Warning: Failed to capture mouse");

					var mbChange = new MouseButtonChange();
					mbChange.pos = GetMousePosition();
					mbChange.button = e.ChangedButton;
					mbChange.state = e.ButtonState;
					simState.Messages.Add(new Message(MessageType.MouseButtonChange, mbChange));
					break;
				}
			}
		}

		private new void MouseUp(object sender, MouseButtonEventArgs e)
		{
			switch (e.ChangedButton)
			{
				default: return;

				case sMouseButton.Left:
				case sMouseButton.Middle:
				case sMouseButton.Right:
					var element = sender as UIElement;
					e.Handled = element.IsMouseCaptured;

					bool success = Mouse.Capture(null);
					if (!success) Debug.Print("Warning: Failed to release mouse capture");

					var mbChange = new MouseButtonChange();
					mbChange.pos = GetMousePosition();
					mbChange.button = e.ChangedButton;
					mbChange.state = e.ButtonState;
					simState.Messages.Add(new Message(MessageType.MouseButtonChange, mbChange));
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
