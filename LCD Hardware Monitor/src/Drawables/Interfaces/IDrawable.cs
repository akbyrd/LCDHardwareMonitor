namespace LCDHardwareMonitor.Drawables
{
	using OpenHardwareMonitor.Hardware;

	public interface IDrawable
	{
		string    Name     { get; }
		ISettings Settings { get; }

		void GetPreview ( ISensor sensor );
		void Render ( ISensor sensor );
	}
}