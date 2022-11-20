using MahApps.Metro.Controls;
using System;
using System.Collections;
using System.Collections.Specialized;
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
	public class BindableDataGrid : DataGrid
	{
		public static readonly DependencyProperty SelectedItemsProperty = DependencyProperty.Register(
			"SelectedItems",
			typeof(IList),
			typeof(BindableDataGrid),
			new PropertyMetadata(OnPropertyChanged));

		private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
		{
			BindableDataGrid bdg = (BindableDataGrid) d;
			if (bdg.initialized) return;
			bdg.initialized = true;

			bdg.source = (IList) e.NewValue;
			bdg.target = ((DataGrid) bdg).SelectedItems;
			((INotifyCollectionChanged) e.NewValue).CollectionChanged += bdg.OnCollectionChanged;
		}

		public new IList SelectedItems
		{
			get { return (IList) GetValue(SelectedItemsProperty); }
			set { SetValue(SelectedItemsProperty, value); }
		}

		IList source;
		IList target;
		bool initialized;

		private void OnCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
		{
			target.Clear();
			foreach (var item in source)
				target.Add(item);
		}
	}

	public partial class MainWindow : MetroWindow
	{
		SimulationState simState;
		int mouseCaptureCount;

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
				default:
					Debug.Assert(false);
					break;

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
				default:
					Debug.Assert(false);
					return;

				case PluginLoadState.Loaded:
					Interop.SetPluginLoadState(simState, item.Handle, PluginLoadState.Unloaded);
					break;

				case PluginLoadState.Unloaded:
					Interop.SetPluginLoadState(simState, item.Handle, PluginLoadState.Loaded);
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
					if (simState.Interaction != Interaction.Null) break;

					e.Handled = true;
					Close();
					break;
			}
		}

		// -------------------------------------------------------------------------------------------
		// Preview Input

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

					if (e.ButtonState == MouseButtonState.Pressed)
					{
						e.Handled = true;
						element.Focus();
					}

					// TODO: I bet the errors here come from multiple mouse buttons. Maybe we should
					// only be capturing for mouse look?

					// TODO: Proper logging
					if (mouseCaptureCount++ == 0)
					{
						e.Handled = true;
						bool success = Mouse.Capture(element);
						if (!success) Debug.Print("Warning: Failed to capture mouse");
					}
					break;
				}
			}

			switch (e.ChangedButton)
			{
				case MouseButton.Left:
					Interop.SelectHovered(simState);
					Interop.BeginDragSelection(simState);
					break;

				case MouseButton.Right:
					Interop.BeginMouseLook(simState);
					break;

				case MouseButton.Middle:
					Interop.ResetCamera(simState);
					break;
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
				{
					var element = sender as UIElement;
					if (--mouseCaptureCount == 0)
					{
						e.Handled = true;
						// TODO: Proper logging
						bool success = Mouse.Capture(null);
						if (!success) Debug.Print("Warning: Failed to release mouse capture");
					}
					break;
				}
			}

			switch (e.ChangedButton)
			{
				case MouseButton.Left:
					Interop.EndDragSelection(simState);
					break;

				case MouseButton.Right:
					Interop.EndMouseLook(simState);
					break;
			}
		}

		private void Preview_KeyDown(object sender, KeyEventArgs e)
		{
			switch (e.Key)
			{
				default: return;

				case Key.Delete:
					e.Handled = true;
					Interop.RemoveSelectedWidgets(simState);
					break;
			}
		}

		// -------------------------------------------------------------------------------------------
		// Drag and Drop

		[Serializable]
		private struct DragDropData
		{
			public PluginKind pluginKind;
			public UInt32 pluginHandle;
			public UInt32 widgetRef;
		}

		// TODO: Looks like there's a bug in .NET where dragging over Firefox, Desktop, other? Crashes with DV_E_FORMATETC
		private void Widget_MouseMove(object sender, MouseEventArgs e)
		{
			if (e.LeftButton == MouseButtonState.Pressed)
			{
				e.Handled = true;

				// TODO: Test this with multiple selection.
				DataGridRow dataGridRow = (DataGridRow) sender;
				WidgetDesc widgetDesc = (WidgetDesc) dataGridRow.Item;

				DragDropData data = new DragDropData();
				data.pluginKind = PluginKind.Widget;
				data.pluginHandle = widgetDesc.PluginHandle;
				data.widgetRef = widgetDesc.Ref;
				Interop.DragDrop(simState, PluginKind.Widget, true);
				DragDropEffects effect = DragDrop.DoDragDrop(dataGridRow, data, DragDropEffects.Copy);
				Interop.DragDrop(simState, PluginKind.Widget, false);
			}
		}

		private void Preview_DragOver(object sender, DragEventArgs e)
		{
			DragDropData? data = e.Data.GetData(typeof(DragDropData)) as DragDropData?;
			if (data == null)
			{
				e.Handled = true;
				e.Effects = DragDropEffects.None;
				return;
			}
		}

		private void Preview_DragDrop(object sender, DragEventArgs e)
		{
			e.Handled = true;

			DragDropData data = (DragDropData) e.Data.GetData(typeof(DragDropData));
			switch (data.pluginKind)
			{
				default:
					Debug.Assert(false);
					break;

				case PluginKind.Widget: Interop.AddWidget(simState, data.pluginHandle, data.widgetRef, GetMousePosition()); break;
				case PluginKind.Sensor: break;
			}
		}

		private void WidgetInstances_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			if (simState.UpdatingSelection) return;

			var grid = (DataGrid) sender;
			Interop.SetWidgetSelection(simState, grid.SelectedItems);
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
