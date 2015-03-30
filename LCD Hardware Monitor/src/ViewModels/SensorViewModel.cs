namespace LCDHardwareMonitor.ViewModels
{
	using System.Collections.Generic;
	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using OpenHardwareMonitor.Hardware;

	/// <summary>
	/// Exposes an <see cref="OpenHardwareMonitor.Hardware.ISensor"/> for
	/// display in XAML.
	/// </summary>
	public class SensorViewModel : INode, INotifyPropertyChanged
	{
		#region Constructor

		public SensorViewModel ( ISensor sensor )
		{
			Sensor = sensor;
			Update();
		}

		#endregion

		#region Public Interface

		//TODO: Completely wrap in ViewModel and hide?
		public ISensor Sensor { get; private set; }

		public  float? Value
		{
			get { return sensorValue; }
			private set
			{
				if ( sensorValue != value )
				{
					sensorValue = value;
					RaisePropertyChangedEvent();
					UpdateValueString();
				}
			}
		}
		private float? sensorValue;

		public  string ValueString
		{
			get { return valueString; }
			private set
			{
				if ( valueString != value )
				{
					valueString = value;
					RaisePropertyChangedEvent();
				}
			}
		}
		private string valueString;

		public void Update ()
		{
			Value = Sensor.Value;
		}

		#endregion

		#region Private Stuff

		private static Dictionary<SensorType, string> sensorValueFormats = new Dictionary<SensorType,string>() {
			{ SensorType.Voltage    , "{0:F3} V"   },
			{ SensorType.Clock      , "{0:F0} MHz" },
			{ SensorType.Load       , "{0:F1} %"   },
			{ SensorType.Temperature, "{0:F1} °C"  },
			{ SensorType.Fan        , "{0:F0} RPM" },
			{ SensorType.Flow       , "{0:F0} L/h" },
			{ SensorType.Control    , "{0:F1} %"   },
			{ SensorType.Level      , "{0:F1} %"   },
			{ SensorType.Power      , "{0:F1} W"   },
			{ SensorType.Data       , "{0:F1} GB"  },
			{ SensorType.Factor     , "{0:F3}"     }
		};

		private void UpdateValueString ()
		{
			if ( Value.HasValue )
				ValueString = string.Format(sensorValueFormats[Sensor.SensorType], Value.Value);
			else
				ValueString = "null";
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