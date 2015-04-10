namespace LCDHardwareMonitor.ViewModels
{
	using System;
	using System.Collections;
	using System.Collections.Generic;
	using System.Collections.ObjectModel;
	using System.ComponentModel;
	using System.Linq;
	using LCDHardwareMonitor.Models;
	using Microsoft.Practices.Prism.Commands;
	using Microsoft.Practices.Prism.Mvvm;

	//TODO: Switch other ViewModels to use Prism/BindableBase
	public class LCDPreviewViewModel : BindableBase
	{
		#region Constructor

		public LCDPreviewViewModel ()
		{
			UpdateWidgetSelection = new DelegateCommand<IList>(OnSelectionChanged);

			settings = new ObservableCollection<DrawableSetting>();
			Settings = new ReadOnlyObservableCollection<DrawableSetting>(settings);
		}

		#endregion

		#region Bindable Properties

		public DelegateCommand<IList> UpdateWidgetSelection { get; private set; }

		public  ISettings WidgetSettingsProxy
		{
			get { return widgetSettingsProxy; }
			private set { SetProperty<ISettings>(ref widgetSettingsProxy, value); }
		}
		private ISettings widgetSettingsProxy;

		public  ReadOnlyObservableCollection<DrawableSetting> Settings { get; private set; }
		private         ObservableCollection<DrawableSetting> settings;

		#endregion

		#region Proxying DrawableSetting<T>s

		/* IDEA: Inheritance is not the way to implement these settings.
		 * Settings should be able to 'pick and choose' from existing settings
		 * and have them work out of the box with multi-select. If 2 settings
		 * classes have a property of type Color named Background, a combined
		 * property should be shown when both containing widgets are selected.
		 * Ways to implement:
		 *	1 - Individual settings are 'modules' (classes) that get reused
		 *	2 - Use attributes on settings to declare their purpose (these will need to enforce matching name and type)
		 *	3 - use Reflection to automatically link up public (bindable? value type?) properties with the same name and type.
		 * 
		 * Biggest question: how do we link a View to each property 'kind' such that it's automatically re-used?
		 */
		private void OnSelectionChanged ( IList selectedWidgets )
		{
			if ( selectedWidgets.Count == 0 )
			{
				WidgetSettingsProxy = null;
				settings.Clear();
			}
			else
			{
				settings.Clear();

				//TODO: Generalize this through reflection

				//DEBUG: Gather StaticText.Text settings
				var textSettings = new List<DrawableSetting<string>>(selectedWidgets.Count);
				foreach ( IWidget widget in selectedWidgets )
					textSettings.Add(((StaticText) widget.Drawable).Text);

				settings.Add(new DrawableSettingsProxy<string>("Text", textSettings));

				//TODO: Assert each selectedItem is IWIdget?
				//TODO: Implement
				//       - Get list of all DrawableSettings properties on each item
				//       - Store properties in dict<type, list<settings propinfo>
				//       - Find common types + names
				//       - Add all to list of DrawableSeetings (might need common base class)
				//       - bind View to list
				//       - View should have templates for each type; way to add user created templates
			}
		}

		private class DrawableSettingsProxy<T> : DrawableSetting
		{
			/// <summary>
			/// Create a proxy for a collection of <see cref="DrawableSetting{T}"/>.
			/// </summary>
			/// <param name="name">The displayable name of the setting being proxied.</param>
			/// <param name="proxiedItems">The settings to be proxied.</param>
			/// <exception cref="ArgumentException">Collection is null or empty.</exception>
			public DrawableSettingsProxy ( string name, IList<DrawableSetting<T>> proxiedItems )
			{
				if ( proxiedItems == null )
					throw new ArgumentException("Cannot proxy a null collection of settings.");

				if ( proxiedItems.Count == 0 )
					throw new ArgumentException("Cannot proxy a empty collection of settings.");

				Name = name;

				this.proxiedItems = proxiedItems.ToList().AsReadOnly();
				foreach ( var setting in proxiedItems )
					setting.PropertyChanged += OnProxiedPropertyChanged;

				UpdateIndeterminateState();
			}

			private readonly ReadOnlyCollection<DrawableSetting<T>> proxiedItems;

			public string Name { get; private set; }

			/// <summary>
			/// Indicates whether the proxied items all have the same value
			/// (false) or not (true).
			/// </summary>
			public  bool IsIndeterminate
			{
				get { return isIndeterminate; }
				set { SetProperty<bool>(ref isIndeterminate, value); }
			}
			private bool isIndeterminate;

			#region Mirrored DrawableSetting<T> Members

			/// <summary>
			/// The current value of the proxied settings. If a named value is
			/// assigned, its value will be returned.
			/// </summary>
			public T Value
			{
				get
				{
					if ( IsIndeterminate )
						return default(T);
					else
						return proxiedItems[0].Value; }

				set
				{
					foreach ( var setting in proxiedItems )
						setting.Value = value;
				}
			}

			/// <summary>
			/// The named value the proxied settings should use.
			/// </summary>
			public string NamedValue
			{
				get
				{
					if ( IsIndeterminate )
						return null;
					else
						return proxiedItems[0].NamedValue;
				}

				set
				{
					foreach ( var setting in proxiedItems )
						setting.NamedValue = value;
				}
			}

			/// <summary>
			/// All existing named values for this <typeparamref name="T"/>.
			/// </summary>
			public Dictionary<string, T>.KeyCollection NamedValues
			{
				get { return proxiedItems[0].NamedValues; }
			}

			#endregion

			#region Modifying Proxied Items

			/// <summary>
			/// Indicates when we are applying changes to all proxied items.
			/// </summary>
			private bool isModifyingProxiedItems;

			/// <summary>
			/// Marks when we start modifying the proxied items.
			/// </summary>
			private void BeginModifyProxiedItems ()
			{
				isModifyingProxiedItems = true;
			}

			/// <summary>
			/// Marks when we finish modifying proxied items. Raises
			/// <see cref="PropertyChanged"/> for all properties.
			/// </summary>
			private void EndModifyProxiedItems ()
			{
				isModifyingProxiedItems = false;
				OnPropertyChanged(null);
			}

			/// <summary>
			/// Check each proxied item to see if they all match or not. Set
			/// <see cref="IsIndeterminate"/> to reflect the result.
			/// </summary>
			private void UpdateIndeterminateState ()
			{
				T value = proxiedItems[0].Value;
				string namedValue = proxiedItems[0].NamedValue;
				bool isNamedValue = namedValue != null;

				for ( int i = 1; i < proxiedItems.Count; ++i )
				{
					bool equal;

					if ( isNamedValue )
						equal = proxiedItems[i].NamedValue != namedValue;
					else
						equal = EqualityComparer<T>.Default.Equals(proxiedItems[i].Value, value);

					if ( !equal )
					{
						IsIndeterminate = true;
						return;
					}
				}

				IsIndeterminate = false;
			}

			/// <summary>
			/// Raise <see cref="PropertyChanged"/> if it is raised by an
			/// proxied setting. We ignore events raised when we're the one
			/// modifying the proxied items.
			/// </summary>
			private void OnProxiedPropertyChanged ( object sender, PropertyChangedEventArgs e )
			{
				if ( !isModifyingProxiedItems )
				{
					UpdateIndeterminateState();
					OnPropertyChanged(e.PropertyName);
				}
			}

			#endregion
		}

		#endregion
	}
}