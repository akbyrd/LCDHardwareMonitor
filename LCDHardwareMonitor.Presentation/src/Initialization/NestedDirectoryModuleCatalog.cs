namespace LCDHardwareMonitor.Presentation
{
	using System;
	using System.Collections.Generic;
	using System.Diagnostics;
	using System.IO;
	using System.Linq;
	using System.Security;
	using LCDHardwareMonitor.Presentation.Properties;
	using Microsoft.Practices.Prism.Logging;
	using Microsoft.Practices.Prism.Modularity;

	/// <summary>
	/// Extends <see cref="DirectoryModuleCatalog"/> to recurse through the
	/// provided path recursively, loading from all subfolders.
	/// </summary>
	internal class NestedDirectoryModuleCatalog : DirectoryModuleCatalog
	{
		private ILoggerFacade logger;

		public NestedDirectoryModuleCatalog ( ILoggerFacade logger )
		{
			this.logger = logger;
		}

		protected override void InnerLoad ()
		{
			IEnumerable<String> subDirectories;
			string pluginDir = ".\\" + Resources.Plugin_DirName;

			try
			{
				subDirectories = Directory.EnumerateDirectories(pluginDir, "*", SearchOption.AllDirectories);
			}
			catch ( ArgumentOutOfRangeException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (       ArgumentNullException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (           ArgumentException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (  DirectoryNotFoundException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (        PathTooLongException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (                 IOException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch (           SecurityException e ) { OnEnumerateDirectoriesFailed(e); return; }
			catch ( UnauthorizedAccessException e ) { OnEnumerateDirectoriesFailed(e); return; }

			foreach ( string path in subDirectories )
			{
				ModulePath = path;

				try
				{
					base.InnerLoad();
				}
				catch ( Exception e )
				{
					string message = string.Format(Resources.Plugin_LoadSingleFail, ModulePath, e);
					logger.Log(message, Category.Exception, Priority.High);

					Debugger.Break();
				}
			}
		}

		[DebuggerHidden]
		private void OnEnumerateDirectoriesFailed ( Exception e )
		{
			string message = string.Format(Resources.Plugin_DirScanFail, e);
			logger.Log(message, Category.Exception, Priority.High);

			Debugger.Break();
		}

		public override void Validate ()
		{
			RemoveModulesWithDuplicateNames();

			base.Validate();
		}

		private void RemoveModulesWithDuplicateNames ()
		{
			var groupedModules = Modules.GroupBy(m => m.ModuleName);
			foreach ( var group in groupedModules )
			{
				foreach ( var moduleInfo in group.Skip(1) )
				{
					Items.Remove(moduleInfo);

					string message = string.Format(Resources.Plugin_NameConflict,
						moduleInfo.ModuleName,
						moduleInfo.Ref,
						group.First().Ref
					);

					logger.Log(message, Category.Warn, Priority.Low);

					Debugger.Break();
				}
			}
		}
	}
}