//TODO: Add refresh button or monitor directory for changes

namespace LCDHardwareMonitor.Presentation
{
	using System;
	using System.ComponentModel.Composition;
	using System.Diagnostics;
	using System.IO;
	using System.Windows;
	using LCDHardwareMonitor.Presentation.Properties;
	using LCDHardwareMonitor.Presentation.Views;
	using Microsoft.Practices.Prism.Logging;
	using Microsoft.Practices.Prism.MefExtensions;
	using Microsoft.Practices.Prism.Modularity;

	internal class Bootstrapper : MefBootstrapper
	{
		protected override IModuleCatalog CreateModuleCatalog ()
		{
			string pluginDir = @".\\" + Resources.Plugin_DirName;

			//Ensure the module directory exists
			try
			{
				Directory.CreateDirectory(pluginDir);
			}
			catch ( UnauthorizedAccessException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (  DirectoryNotFoundException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (       ArgumentNullException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (       NotSupportedException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (        PathTooLongException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (           ArgumentException e ) { return OnCreateModuleCatalogFiled(e); }
			catch (                 IOException e ) { return OnCreateModuleCatalogFiled(e); }

			var moduleCatalog = new NestedDirectoryModuleCatalog(Logger);
			moduleCatalog.ModulePath = pluginDir;

			return moduleCatalog;
		}

		[DebuggerHidden]
		private ModuleCatalog OnCreateModuleCatalogFiled ( Exception e )
		{
			string message = string.Format(Resources.Plugin_DirCreateFail, e);
			Logger.Log(message, Category.Exception, Priority.High);

			Debugger.Break();

			return new ModuleCatalog();
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
				string message = string.Format(Resources.Plugin_LoadAllFail, e);
				Logger.Log(message, Category.Exception, Priority.Medium);

				Debugger.Break();
			}
		}
	}
}