namespace LCDHardwareMonitor.Views
{
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Controls.Primitives;
	using System.Windows.Input;
	using System.Windows.Media;
	using LCDHardwareMonitor.ViewModels;

	/// <summary>
	/// Interaction logic for OHMSourceView.xaml
	/// </summary>
	public partial class OHMSourceView : UserControl
	{
		public OHMSourceView ()
		{
			InitializeComponent();
		}

		private int lastHandledTimestamp;
		/// <summary>
		/// Tap into left mouse clicking and enable single-click expanding.
		/// </summary>
		private void TreeViewItem_PreviewMouseLeftButtonDown ( object sender, MouseButtonEventArgs e )
		{
			/* Since Preview clicks aren't Handled, we need to avoid processing
			 * the event more than once in the tree.
			 */
			if ( e.Timestamp != lastHandledTimestamp )
			{
				lastHandledTimestamp = e.Timestamp;

				/* TreeViewItem inherently handles left click and expands on ever
				 * double click. If we're not careful we'll end up fighting the
				 * built-in click handling.
				 */
				if ( e.ClickCount % 2 != 0 )
				{
					//Find the enclosing TreeViewItem
					var parent = (DependencyObject) e.OriginalSource;
					do
					{
						/* Abort if a ToggleButton was clicked. They already
						 * have single click toggling.
						 */
						if ( parent is ToggleButton ) { return; }

						parent = VisualTreeHelper.GetParent(parent);
					} while ( !(parent is TreeViewItem) );

					TreeViewItem treeViewItem = (TreeViewItem) parent;

					treeViewItem.IsExpanded = !treeViewItem.IsExpanded;
				}
			}
		}

		/// <summary>
		/// Hack to set the intial selection on the list of hardware.
		/// </summary>
		private void HardwareList_Loaded ( object sender, RoutedEventArgs e )
		{
			//if ( HardwareList.SelectedItem != null ) { return; }

			//var firstItem = HardwareList.ItemContainerGenerator.ContainerFromIndex(0) as TreeViewItem;
			//if ( firstItem != null )
			//	firstItem.IsSelected = true;
		}
	}
}