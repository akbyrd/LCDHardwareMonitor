namespace LCDHardwareMonitor
{
	using System;
	using System.ComponentModel;
	using System.Runtime.CompilerServices;
	using OpenHardwareMonitor.Hardware;

	public class StaticText : IDrawable
	{
		#region Constructor

		public StaticText ()
		{
			settings = new SettingsContainer();
		}

		#endregion

		#region Settings

		public class SettingsContainer : ISettings
		{
			public  string Text
			{
				get { return text; }
				set { text = value; RaisePropertyChangedEvent(); }
			}
			private string text;

			/// <summary>
			/// Set default values.
			/// </summary>
			public SettingsContainer ()
			{
				int rV = rnd.Next(initialNames.Length);
				Text = initialNames[rV];
			}

			private Random rnd = new Random();
			private string[] initialNames = new string[] {
				"New Static Text",
				"Club baby seals",
				"I <3 John Carmack",
				"His name was Robert Paulson",
				"Wrecked him? Damn near killed him!"
			};

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

		#endregion

		#region IDrawable Implementation

		public string Name { get { return "Static Text"; } }

		public ISettings Settings { get { return settings; } }
		private SettingsContainer settings;

		public void GetPreview ( ISensor sensor )
		{
			throw new System.NotImplementedException();
		}

		public void Render ( ISensor sensor )
		{
			throw new System.NotImplementedException();
		}

		#endregion
	}
}