namespace LCDHardwareMonitor.Presentation.Views
{
	using System;
	using System.ComponentModel;
	using System.Diagnostics;
	using System.Runtime.CompilerServices;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Input;
	using System.Windows.Interop;
	using System.Windows.Media;

	/// <summary>
	/// Interaction logic for LCDPreview.xaml
	/// </summary>
	public partial class LCDPreviewView : UserControl, INotifyPropertyChanged
	{
		#region Constructor

		public LCDPreviewView ()
		{
			InitializeComponent();

			CompositionTarget.Rendering += CompositionTarget_Rendering;
		}

		private void CompositionTarget_Rendering(object sender, EventArgs e)
		{
			//TODO: Do something about this
			bool success;
			success = Renderers.D3D11Renderer.Render();
			if (!success)
				throw new Exception("Uh oh");

			LCDPreviewTexture.Lock();
			LCDPreviewTexture.SetBackBuffer(D3DResourceType.IDirect3DSurface9, Shell.StaticLCDModel.RenderSurface, true);
			LCDPreviewTexture.AddDirtyRect(new Int32Rect(0, 0, LCDPreviewTexture.PixelWidth, LCDPreviewTexture.PixelHeight));
			LCDPreviewTexture.Unlock();
		}

		#endregion

		#region Public Properties

		public  float HighlightOpacity
		{
			get { return highlightOpacity; }
			private set
			{
				if ( highlightOpacity != value )
				{
					highlightOpacity = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private float highlightOpacity;

		/* NOTE: This needs to be outside the widgets themselves. Widgets would
		 * only be notified that a drag is happening once something is already
		 * dragged over them. If they disable hit testing, how would they know
		 * when to turn it back on?
		 */
		public  bool EnableWidgetHitTest
		{
			get { return enableWidgetHitTest; }
			private set
			{
				if ( enableWidgetHitTest != value )
				{
					enableWidgetHitTest = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private bool enableWidgetHitTest;

		/* NOTE: This drop point shit is going away once widgets provide their
		 * own preview.
		 */
		public  Visibility DropPointVisibility
		{
			get { return dropPointVisibility; }
			private set
			{
				if ( dropPointVisibility != value )
				{
					dropPointVisibility = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private Visibility dropPointVisibility = Visibility.Hidden;

		public  Point DropPoint
		{
			get { return dropPoint; }
			private set
			{
				if ( dropPoint != value )
				{
					dropPoint = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private Point dropPoint;

		#endregion

		//TODO: Find a better way to do this
		#region Obtaining Panel Reference

		private Panel lcdPanel;

		/// <summary>
		/// We need a reference to the actual panel the LCD widgets are part of
		/// so we can transform the mouse position into the panel's local space
		/// and place new widgets at the point where they were dropped.
		/// </summary>
		private void LCDPreviewContainer_Loaded ( object sender, RoutedEventArgs e )
		{
			lcdPanel = Utility.FindChild<Panel>(LCDPreviewContainer);
		}

		#endregion

		#region Drag & Drop

		private void Canvas_DragEnter ( object sender, DragEventArgs e )
		{
			//Attempt to grab an IDrawable Type from the DataObject.
			object data = e.Data.GetData(DrawablesView.DrawableFormat);
			draggedDataType = data as Type;

			//Ensure a type is received and that it implements IDrawable.
			if ( draggedDataType != null && typeof(IDrawable).IsAssignableFrom(draggedDataType) )
			{
				StartDragAndDrop();
			}
		}

		private void Canvas_DragOver  ( object sender, DragEventArgs e )
		{
			//Let the source know that this is a valid drop target.
			e.Effects = dropEffect;

			/* NOTE: Mouse.GetPosition does not work during drag & drop
			 * operations.
			 */
			Point snapPoint = e.GetPosition(lcdPanel);

			//TODO: Notify the user of the snapping functionality? Maybe text that fades in when dragging
			int snapAmount = 0;
			if ( Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl) )
				snapAmount = 10;

			if ( Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift) )
				snapAmount = 40;

			if ( snapAmount > 0 )
			{
				snapPoint.X = SnapValueToClosest(snapPoint.X, snapAmount);
				snapPoint.Y = SnapValueToClosest(snapPoint.Y, snapAmount);
			}

			DropPoint = snapPoint;
		}

		private void Canvas_DragLeave ( object sender, DragEventArgs e )
		{
			/* NOTE: Without a handler, DragEventArgs.Effects will default back
			 * to none, so we don't need to do it manually.
			 */
			CancelDragAndDrop();
		}

		//TODO: The logic here should be in the ViewModel and triggered through a Command
		private void Canvas_Drop      ( object sender, DragEventArgs e )
		{
			//Create a new widget from the dropped drawable.
			var newDrawable = (IDrawable) Activator.CreateInstance(draggedDataType);
			var newWidget = new Widget(newDrawable);

			//Position the new widget where it was dropped.
			newWidget.Position = new Vector2I((int) DropPoint.X, (int) DropPoint.Y);

			//Add it to the list.
			Shell.StaticLCDModel.Widgets.Add(newWidget);

			CancelDragAndDrop();
		}

		private Type draggedDataType;
		private DragDropEffects dropEffect;

		/// <summary>
		/// Allow the drag and drop operation to proceed by allowing the Copy
		/// effect and enabling a visual cue.
		/// </summary>
		private void StartDragAndDrop ()
		{
			dropEffect = DragDropEffects.Copy;

			HighlightOpacity = 1;
			DropPointVisibility = Visibility.Visible;
		}

		/// <summary>
		/// Cancel any current drag and drop operation, disable the visual cue,
		/// and clear cached data.
		/// </summary>
		private void CancelDragAndDrop ()
		{
			dropEffect = DragDropEffects.None;

			HighlightOpacity = 0;
			DropPointVisibility = Visibility.Hidden;

			draggedDataType = null;
		}

		#endregion

		#region Utility Methods

		/// <summary>
		/// Snaps a float value to the closest multiple of a provided
		/// increment. e.g snapping 7 to an increment of 5 should yield 5.
		/// 8 would yield 10.
		/// </summary>
		/// <param name="value">The number to snap.</param>
		/// <param name="increment">Snap to a multiple of this value.</param>
		/// <returns></returns>
		private double SnapValueToClosest ( double value, double increment )
		{
			increment = Math.Abs(increment) * Math.Sign(value);

			double remainder = value % increment;

			if ( remainder < .5*increment )
				return value - remainder;
			else
				return value - remainder + increment;
		}

		#endregion

		#region INotifyPropertyChanged Implementation

		public event PropertyChangedEventHandler PropertyChanged;
	
		private void RaisePropertyChangedEvent ( [CallerMemberName] string propertyName = "" )
		{
			if ( PropertyChanged != null )
			{
				var args = new PropertyChangedEventArgs(propertyName);
				PropertyChanged(this, args);
			}
		}

		#endregion
	}
}