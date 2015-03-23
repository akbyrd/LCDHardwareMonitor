namespace LCDHardwareMonitor.Drawables
{
	using System;
	using System.Windows;
	using OpenHardwareMonitor.Hardware;

	//TODO: INotifyPropertyChanged for Name, position, sensor?

	public class Widget : IWidget
	{
		#region Constructor

		public Widget ( IDrawable drawable )
		{
			//TODO: Better name
			Name = "New Widget";
			Drawable = drawable;
		}

		#endregion

		#region Functionality

		private Computer  computer;
		private IHardware hardware;

		private void HardwareRemoved ( IHardware removedHardware )
		{
			if ( hardware != null && hardware == removedHardware )
			{
				RemoveSensor();
			}
		}

		private void SensorRemoved ( ISensor removedSensor )
		{
			if ( Sensor != null && Sensor == removedSensor )
			{
				RemoveSensor();
			}
		}

		private void RemoveSensor ()
		{
			if ( computer != null )
			{
				computer.HardwareRemoved -= HardwareRemoved;
				computer = null;
			}

			if ( hardware != null )
			{
				hardware.SensorRemoved   += SensorRemoved;
				hardware = null;
			}

			Sensor = null;
		}

		#endregion

		#region IWidget Implementation

		public string     Name       { get; set; }
		public Vector     Position   { get; set; }
		public IDrawable  Drawable   { get; private set; }
		public ISensor    Sensor     { get; private set; }
		public Identifier Identifier { get; private set; }

		public void SetSensor ( Computer computer, ISensor sensor )
		{
			if ( computer == null )
			{
				var s = string.Format("Attempting to assign {0} with null {1} to {2}", typeof(ISensor), typeof(Computer), typeof(Widget));
				Console.WriteLine(s);
				return;
			}

			if ( sensor == null )
			{
				var s = string.Format("Attempting to assign null {0} to {2}", typeof(ISensor), typeof(Widget));
				Console.WriteLine(s);
				return;
			}

			RemoveSensor();

			Sensor = sensor;
			hardware = sensor.Hardware;
			this.computer = computer;
			Identifier = sensor.Identifier;

			computer.HardwareRemoved += HardwareRemoved;
			hardware.SensorRemoved   += SensorRemoved;
		}

		public void SetSensor ( Computer computer, Identifier identifier )
		{
			var sensor = Utility.FindSensorInComputer(computer, identifier);

			if ( sensor != null )
				SetSensor(computer, sensor);
		}

		#endregion

		#region IDisposable Implementation

		public void Dispose ()
		{
			//TODO: Save identifier, position, name, to settings.

			RemoveSensor();
			Identifier = null;
		}

		#endregion
	}
}