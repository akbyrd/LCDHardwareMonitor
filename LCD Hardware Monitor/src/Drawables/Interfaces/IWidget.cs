namespace LCDHardwareMonitor.Drawables
{
	using System;
	using System.Windows;
	using OpenHardwareMonitor.Hardware;

	public interface IWidget : IDisposable
	{
		string           Name { get; set; }
		Point        Position { get; set; }
		IDrawable    Drawable { get; }
		ISensor        Sensor { get; }
		Identifier Identifier { get; }

		void SetSensor ( Computer computer , ISensor sensor );
		void SetSensor ( Computer computer , Identifier identifier );
	}
}