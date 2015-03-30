namespace LCDHardwareMonitor
{
	using System.Windows.Media;
	using OpenHardwareMonitor.Hardware;

	public static class Utility
	{
		public static ISensor FindSensorInComputer ( Computer computer, Identifier identifier )
		{
			if ( computer == null )
			{
				var s = string.Format("Attempting to find {0} in null {1}", typeof(Identifier), typeof(Computer));
				System.Console.WriteLine(s);
				return null;
			}

			if ( identifier == null )
			{
				var s = string.Format("Attempting to find null {0} in {1}", typeof(Identifier), typeof(Computer));
				System.Console.WriteLine(s);
				return null;
			}

			//TODO: Implement
			throw new System.NotImplementedException();
		}

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