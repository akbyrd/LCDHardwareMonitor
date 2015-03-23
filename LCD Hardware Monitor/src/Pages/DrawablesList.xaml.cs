namespace LCDHardwareMonitor.Pages
{
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Input;

	/// <summary>
	/// Interaction logic for DrawablesList.xaml
	/// </summary>
	public partial class DrawablesList : UserControl
	{
		public DrawablesList ()
		{
			InitializeComponent();
		}

		#region Drag and Drop Functionality

		//TODO: Cleaner way to do this
		private bool isClicked;

		private void Drawable_MouseLeftButtonDown ( object sender, MouseEventArgs e )
		{
			var lvItem = sender as ListViewItem;
			if ( lvItem != null && e.LeftButton == MouseButtonState.Pressed )
			{
				isClicked = true;
			}
		}

		private void Drawable_MouseLeftButtonUp ( object sender, MouseEventArgs e )
		{
			var lvItem = sender as ListViewItem;
			if ( lvItem != null )
			{
				isClicked = false;
			}
		}

		private void Drawable_MouseMove ( object sender, MouseEventArgs e )
		{
			if ( isClicked )
			{
				var lvItem = sender as ListViewItem;
				if ( lvItem != null && e.LeftButton == MouseButtonState.Pressed )
				{
					DragDrop.DoDragDrop(lvItem, lvItem.Content.GetType(), DragDropEffects.Copy);
				}
			}
		}

		#endregion
	}
}