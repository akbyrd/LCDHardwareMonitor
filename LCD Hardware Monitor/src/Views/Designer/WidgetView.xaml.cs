namespace LCDHardwareMonitor.Views
{
	//TODO: Implement a 'create widget' command
	//TODO: LCDPrevew should call the command
	//TODO: WidgetView spawns a DrawableView

	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using System.Windows;
	using System.Windows.Controls;

	/// <summary>
	/// Interaction logic for WidgetViewModel.xaml
	/// </summary>
	public partial class WidgetView : UserControl, INotifyPropertyChanged
	{
		public WidgetView ()
		{
			InitializeComponent();
		}

		#region Drag & Drop

		public  float HighlightOpacity
		{
			get { return highlightOpacity; }
			private set { highlightOpacity = value; RaisePropertyChangedEvent(); }
		}
		private float highlightOpacity;

		public  bool EnableHitTest
		{
			get { return enableHitTest; }
			private set { enableHitTest = value; RaisePropertyChangedEvent(); }
		}
		private bool enableHitTest = true;

		private void OnDragEnter ( object sender, DragEventArgs e )
		{

		}

		private void OnDragOver  ( object sender, DragEventArgs e )
		{

		}

		private void OnDragLeave ( object sender, DragEventArgs e )
		{

		}

		private void OnDrop      ( object sender, DragEventArgs e )
		{

		}

		private void StartDragAndDrop ()
		{
			HighlightOpacity = 1;
		}

		private void CancelDragAndDrop ()
		{
			HighlightOpacity = 0;
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