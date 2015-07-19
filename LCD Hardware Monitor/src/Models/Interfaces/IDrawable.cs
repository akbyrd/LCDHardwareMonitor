namespace LCDHardwareMonitor
{
	using Microsoft.Practices.Prism.Modularity;
	using OpenHardwareMonitor.Hardware;

	public interface IDrawable : IModule
	{
		string Name { get; }

		void GetPreview ( ISensor sensor );
		void Render ( ISensor sensor );
	}
}