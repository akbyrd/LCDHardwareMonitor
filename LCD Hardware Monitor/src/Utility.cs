namespace LCDHardwareMonitor
{
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
	}
}