namespace LCDHardwareMonitor
{
	using System;
	using LCDHardwareMonitor.Models;
	using Microsoft.Practices.Prism.Mvvm;
	using OpenHardwareMonitor.Hardware;

	public class StaticText : BindableBase, IDrawable
	{
		#region Constructor

		public StaticText ()
		{
			int rV = rnd.Next(initialNames.Length);
			Text = new DrawableSetting<string>(initialNames[rV]);
		}

		#endregion

		#region Settings

		public DrawableSetting<string> Text { get; private set; }

		private Random rnd = new Random();
		private string[] initialNames = new string[] {
			"New Static Text",
			"I club baby seals",
			"I <3 John Carmack",
			"His name was Robert Paulson",
			"Wrecked him? Damn near killed him!"
		};

		#endregion

		#region IDrawable Implementation

		public string Name { get { return "Static Text"; } }

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