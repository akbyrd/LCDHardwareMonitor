namespace LCDHardwareMonitor
{
	using System;

	public interface IWidget : IDisposable
	{
		string           Name { get; set; }
		Vector2I     Position { get; set; }
		IDrawable    Drawable { get; }
		//ISensor        Sensor { get; }
		//Identifier Identifier { get; }

		//void SetSensor ( Computer computer, ISensor sensor );
		//void SetSensor ( Computer computer, Identifier identifier );
		void Update ();
	}
}