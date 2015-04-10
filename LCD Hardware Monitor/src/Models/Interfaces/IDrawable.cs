namespace LCDHardwareMonitor
{
	using OpenHardwareMonitor.Hardware;

	public interface IDrawable
	{
		string Name { get; }

		void GetPreview ( ISensor sensor );
		void Render ( ISensor sensor );
	}
}