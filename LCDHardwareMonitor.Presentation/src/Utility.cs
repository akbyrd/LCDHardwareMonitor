namespace LCDHardwareMonitor
{
	using System.Windows.Media;

	public static class Utility
	{
		public static T FindChild<T> ( Visual visual ) where T : Visual
		{
			int childCount = VisualTreeHelper.GetChildrenCount(visual);
			for ( int i = 0; i < childCount; ++i )
			{
				var child = VisualTreeHelper.GetChild(visual, i);
				if ( child is T ) { return (T) child; }

				if ( child is Visual )
				{
					T panel = FindChild<T>((Visual) child);
					if ( panel != null ) { return panel; }
				}
			}

			return default(T);
		}
	}
}