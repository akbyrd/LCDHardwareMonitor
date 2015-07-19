namespace LCDHardwareMonitor
{
	using System;
	using System.IO;
	using System.Windows;
	using LCDHardwareMonitor.Views;
	using Microsoft.Practices.Prism.Modularity;
	using Microsoft.Practices.Prism.UnityExtensions;
	using Microsoft.Practices.ServiceLocation;

	internal class Bootstrapper : UnityBootstrapper
	{
		protected override DependencyObject CreateShell ()
		{
			return ServiceLocator.Current.GetInstance<MainWindow>();
		}

		protected override void InitializeShell ()
		{
			base.InitializeShell();

			Application.Current.MainWindow = (Window) Shell;
			Application.Current.MainWindow.Show();
		}

		protected override IModuleCatalog CreateModuleCatalog ()
		{
			const string modulePath = @".\Plugins";
			//TODO: Move to resources
			const string errorMsg = "Warning: Failed to create plugin directory";

			//Ensure the module directory exists
			try
			{
				Directory.CreateDirectory(modulePath);
			}
			catch ( UnauthorizedAccessException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (  DirectoryNotFoundException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (       ArgumentNullException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (       NotSupportedException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (        PathTooLongException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (           ArgumentException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }
			catch (                 IOException e ) { Console.WriteLine(errorMsg + Environment.NewLine + e); return new ModuleCatalog(); }

			var moduleCatalog = new DirectoryModuleCatalog();
			moduleCatalog.ModulePath = modulePath;

			return moduleCatalog;
		}

		protected override void ConfigureModuleCatalog ()
		{
			base.ConfigureModuleCatalog();

			//TODO: Automatically add these?
			ModuleCatalog.AddModule(new ModuleInfo(
				typeof(StaticText).Name,
				typeof(StaticText).AssemblyQualifiedName
			));

			//TODO: Add refresh button or monitor directory for changes
		}
	}
}