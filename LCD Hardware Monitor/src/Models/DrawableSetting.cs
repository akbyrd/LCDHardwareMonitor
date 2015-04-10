namespace LCDHardwareMonitor.Models
{
	using System.Collections.Generic;
	using Microsoft.Practices.Prism.Mvvm;

	/// <summary>
	/// Represents an individual setting on a <see cref="IDrawable"/>. Supports
	/// 'named values' that can be shared by multiple instances.
	/// </summary>
	/// <typeparam name="T">The type of the setting to be encapsulated.</typeparam>
	public class DrawableSetting<T> : DrawableSetting
	{
		#region Constructor

		//TODO: Fill named values from settings
		public DrawableSetting ( T value )
		{
			Value = value;
			namedValues = new Dictionary<string,T>();
		}

		#endregion

		#region Public Properties

		/// <summary>
		/// The current value of this setting. If a named value is assigned,
		/// its value will be returned.
		/// </summary>
		public  T  Value
		{
			get
			{
				if ( !string.IsNullOrWhiteSpace(NamedValue) )
					return namedValues[NamedValue];
				else
					return _value;
			}

			set
			{
				if ( !string.IsNullOrWhiteSpace(NamedValue) )
					namedValues[NamedValue] = value;

				SetProperty<T>(ref _value, value);
			}
		}
		private T _value;

		/// <summary>
		/// The named value this setting should use. Null when no named value
		/// is in use.
		/// </summary>
		public  string NamedValue
		{
			get { return namedValue; }
			set
			{
				if ( !ValidateName(value) ) { return; }

				//Add new names to the collection of available names.
				if ( !string.IsNullOrWhiteSpace(value) )
					if ( !namedValues.ContainsKey(value) )
						namedValues[value] = _value;

				bool changed = SetProperty<string>(ref namedValue, value);

				//If name has changed, raise PropertyChanged for Value as well.
				if ( changed )
					OnPropertyChanged(() => Value);
			}
		}
		private string namedValue;

		/// <summary>
		/// All existing named values for this <typeparamref name="T"/>.
		/// </summary>
		public Dictionary<string, T>.KeyCollection NamedValues
		{
			get { return namedValues.Keys; }
		}

		#endregion

		#region Named Values

		//TODO: Changing a named value should cause PropertyChanged to be raised on all instances.
		//TODO: Provide a way to delete named values.

		/// <summary>
		/// Contains all named values for this <typeparamref name="T"/>.
		/// </summary>
		private static Dictionary<string, T> namedValues;

		private static bool ValidateName ( string name )
		{
			return name == null || !string.IsNullOrWhiteSpace(name);
		}

		#endregion
	}

	/// <summary>
	/// This only exists to give <see cref="DrawableSetting{T}"/> a common base type.
	/// </summary>
	public abstract class DrawableSetting : BindableBase { }
}