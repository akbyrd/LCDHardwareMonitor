namespace LCDHardwareMonitor.Pages
{
	using System;
	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using System.Windows;
	using System.Windows.Controls;
	using System.Windows.Media;
	using System.Windows.Media.Effects;
	using LCDHardwareMonitor.Drawables;

	/// <summary>
	/// Interaction logic for LCDPreview.xaml
	/// </summary>
	public partial class LCDPreview : UserControl, INotifyPropertyChanged
	{
		public  Effect HighlightEffect
		{
			get { return highlightEffect; }
			private set { highlightEffect = value; RaisePropertyChangedEvent(); }
		}
		private Effect highlightEffect;

		public LCDPreview ()
		{
			InitializeComponent();
		}

		#region Drag & Drop Functionality

		private Type draggedDataType;
		private DragDropEffects dropEffect;
		private DropShadowEffect dropShadow = new DropShadowEffect() {
			Color = new Color() { B = 255 }
		};

		//TODO: Cleanup
		private void Canvas_DragEnter ( object sender, DragEventArgs e )
		{
			//var staticText = (StaticText) e.Data.GetData(typeof(StaticText));
			object data = e.Data.GetData(e.Data.GetFormats()[0]);
			draggedDataType = data as Type;

			if ( draggedDataType != null && typeof(IDrawable).IsAssignableFrom(draggedDataType) )
			{
				dropEffect = DragDropEffects.Copy;
				HighlightEffect = dropShadow;
			}
			else
			{
				dropEffect = DragDropEffects.None;
				HighlightEffect = null;
			}
		}

		private void Canvas_DragOver ( object sender, DragEventArgs e )
		{
			e.Effects = dropEffect;
		}

		private void Canvas_DragLeave ( object sender, DragEventArgs e )
		{
			dropEffect = DragDropEffects.None;
			HighlightEffect = null;
		}

		private void Canvas_Drop ( object sender, DragEventArgs e )
		{
			dropEffect = DragDropEffects.None;
			HighlightEffect = null;

			var newDrawable = (IDrawable) Activator.CreateInstance(draggedDataType);
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