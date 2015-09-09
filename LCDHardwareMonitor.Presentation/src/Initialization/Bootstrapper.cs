//TODO: Add refresh button or monitor directory for changes

namespace LCDHardwareMonitor.Presentation
{
	using System;
	using System.ComponentModel.Composition;
	using System.Diagnostics;
	using System.IO;
	using System.Windows;
	using LCDHardwareMonitor.Presentation.Views;
	using Microsoft.Practices.Prism.Logging;
	using Microsoft.Practices.Prism.MefExtensions;
	using Microsoft.Practices.Prism.Modularity;

	internal class Bootstrapper : MefBootstrapper
	{
		protected override IModuleCatalog CreateModuleCatalog ()
		{
			//TODO: Move to resources
			const string modulePath = @".\Plugins";
			const string errorMsg = "Warning: Failed to create plugin directory.";

			Func<Exception, ModuleCatalog> onError = (e) => {
				Console.WriteLine(errorMsg + Environment.NewLine + e);
				return new ModuleCatalog();
			};

			//Ensure the module directory exists
			try
			{
				Directory.CreateDirectory(modulePath);
			}
			catch ( UnauthorizedAccessException e ) { return onError(e); }
			catch (  DirectoryNotFoundException e ) { return onError(e); }
			catch (       ArgumentNullException e ) { return onError(e); }
			catch (       NotSupportedException e ) { return onError(e); }
			catch (        PathTooLongException e ) { return onError(e); }
			catch (           ArgumentException e ) { return onError(e); }
			catch (                 IOException e ) { return onError(e); }

			var moduleCatalog = new NestedDirectoryModuleCatalog(Logger);
			moduleCatalog.ModulePath = modulePath;

			return moduleCatalog;
		}

		protected override DependencyObject CreateShell ()
		{
			return new Shell(Logger);
		}

		protected override void InitializeShell ()
		{
			base.InitializeShell();

			Application.Current.MainWindow = (Window) Shell;
			Application.Current.MainWindow.Show();
		}

		protected override void InitializeModules ()
		{
			var pluginInterface = new PluginInterface(((Shell) Shell), Logger);
			Container.ComposeExportedValue<PluginInterface>(pluginInterface);

			try
			{
				base.InitializeModules();
			}
			catch ( Exception e )
			{
				const string loadingPluginsFailedMsg = "Failed to load plugins.";

				Logger.Log(loadingPluginsFailedMsg + Environment.NewLine + e, Category.Exception, Priority.Medium);
				Debugger.Break();
			}
		}
	}
}