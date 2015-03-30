namespace LCDHardwareMonitor.Controls
{
	using System;
	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using System.Windows.Controls;
	using System.Windows.Controls.Primitives;

	/// <summary>
	/// Extends and slightly modifies TreeView.
	///  - Initial selection will be automatically set.
	///  - TreeViewItems will stretch to fill available horizontal space.
	/// </summary>
	public class BetterTreeView : TreeView, INotifyPropertyChanged
	{
		public BetterTreeView () : base()
		{
			SetsInitialSelection = true;
		}

		#region Selection Handling

		/// <summary>
		/// When enabled, SelectedItem will be automatically set to the first
		/// item in the associated collection when the control is loaded and
		/// when the collection is changed. If something is already selected,
		/// the selection will not be changed.
		/// </summary>
		public  bool SetsInitialSelection
		{
			get { return setsInitialSelection; }
			set
			{
				if ( setsInitialSelection != value )
				{
					setsInitialSelection = value;
					if ( setsInitialSelection )
					{
						ItemContainerGenerator.ItemsChanged  += OnGeneratorItemsChanged;
						ItemContainerGenerator.StatusChanged += OnGeneratorStatusChanged;
					}
					else
					{
						ItemContainerGenerator.ItemsChanged  -= OnGeneratorItemsChanged;
						ItemContainerGenerator.StatusChanged -= OnGeneratorStatusChanged;
					}
					UpdateSelection();
					RaisePropertyChangedEvent();
				}
			}
		}
		private bool setsInitialSelection;

		private void OnGeneratorItemsChanged  ( object sender, ItemsChangedEventArgs e ) { UpdateSelection(); }
		private void OnGeneratorStatusChanged ( object sender,             EventArgs e ) { UpdateSelection(); }

		/// <summary>
		/// Select the first item container if nothing is already selected.
		/// </summary>
		private void UpdateSelection ()
		{
			if ( SelectedItem == null && ItemContainerGenerator.Status == GeneratorStatus.ContainersGenerated )
			{
				var firstItem = ItemContainerGenerator.ContainerFromIndex(0) as TreeViewItem;
				if ( firstItem != null )
					firstItem.IsSelected = true;
			}
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