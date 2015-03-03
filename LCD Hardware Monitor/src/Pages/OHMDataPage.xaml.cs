using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
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

		/// <summary>
		/// Tap into left mouse clicking and enable single-click expanding.
		/// </summary>
		private void TreeViewItem_MouseLeftButtonDown ( object sender, MouseButtonEventArgs e )
		{
			/* TreeViewItem inherently handles left click and expands on ever
			 * double click. If we're not careful we'll end up fighting the
			 * built-in click handling.
			 */
			if ( e.ClickCount % 2 != 0 )
			{
				TreeViewItem treeViewItem = (TreeViewItem) sender;
				treeViewItem.IsExpanded = !treeViewItem.IsExpanded;
			}
		}

		/// <summary>
		/// Purely for providing a place for a breakpoint for easy debugging.
		/// </summary>
		private void UserControl_MouseRightButtonDown ( object sender, MouseButtonEventArgs e ) { }
	}

	public class OHMTreeTemplateSelector : DataTemplateSelector
	{
		public HierarchicalDataTemplate HardwareTemplate { get; set; }
		public             DataTemplate   SensorTemplate { get; set; }

		public override DataTemplate SelectTemplate ( object item, DependencyObject container )
		{
			     if ( item is IHardware ) return HardwareTemplate;
			else if ( item is ISensor   ) return   SensorTemplate;

			return base.SelectTemplate(item, container);
		}
	}
}