namespace LCDHardwareMonitor.Views
{
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Input;

	/// <summary>
	/// Interaction logic for DrawablesList.xaml
	/// </summary>
	public partial class DrawablesList : UserControl
	{
		#region Constructor

		public DrawablesList ()
		{
			InitializeComponent();
		}

		#endregion

		#region Drag and Drop Functionality

		/// <summary>
		/// The format used when stuffing the <see cref="System.Type"/> of an
		/// <see cref="Drawables.IDrawable"/> into a <see cref="DataObject"/>.
		/// </summary>
		public const string DrawableFormat = "Drawable";

		private ListViewItem clickedItem;

		/// <summary>
		/// If a drawable in the list is clicked, keep track of it to allow
		/// subsequent dragging to initiate a drag and drop operation.
		/// </summary>
		private void Drawable_PreviewMouseLeftButtonDown ( object sender, MouseEventArgs e )
		{
			clickedItem = sender as ListViewItem;
		}

		/// <summary>
		/// If the mouse is released, clear the clicked item reference.
		/// </summary>
		private void Drawable_PreviewMouseLeftButtonUp ( object sender, MouseEventArgs e )
		{
			clickedItem = null;
		}

		/// <summary>
		/// If the mouse is being dragged on the same item the was initially
		/// clicked, initiate a drag and drop operation.
		/// </summary>
		private void Drawable_MouseMove ( object sender, MouseEventArgs e )
		{
			if ( clickedItem != null && sender == clickedItem )
			{
				var dataObject = new DataObject(DrawableFormat, clickedItem.Content.GetType());
				DragDrop.DoDragDrop(clickedItem, dataObject, DragDropEffects.Copy);

				/* DoDragDrop is blocking. This is an easy way to know when
				 * the drag operation is complete. We prevent another drag
				 * from taking place until the user actually clicks on a
				 * drawable again.
				 */
				clickedItem = null;
			}
		}

		#endregion
	}
}