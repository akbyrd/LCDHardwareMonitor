using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using LCDHardwareMonitor.ViewModel;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor.Pages
{
	//TODO: Change this to something like "SensorPage", then have a subpage for each provider (e.g. OHM, AIDA, Fraps)

	/// <summary>
	/// Interaction logic for OHMDataPage.xaml
	/// </summary>
	public partial class OHMDataPage : UserControl
	{
		public OHMHardwareTree HardwareTree { get; private set; }

		public OHMDataPage ()
		{
			HardwareTree = new OHMHardwareTree();

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
		/// Purely for providing a place for a breakpoint for easy debugging.
		/// </summary>
		private void UserControl_MouseRightButtonDown ( object sender, MouseButtonEventArgs e ) { }
	}

	/// <summary>
	/// This is used to selectively bind properties on
	/// <see cref="HardareNode"/>s only.
	/// </summary>
	public class OHMTreeStyleSelector : StyleSelector
	{
		public Style HardwareStyle { get; set; }

		public override Style SelectStyle ( object item, DependencyObject container )
		{
			if ( item is HardwareNode )
				return HardwareStyle;

			return base.SelectStyle(item, container);
		}
	}
}