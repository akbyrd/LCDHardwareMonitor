namespace LCDHardwareMonitor.Presentation
{
	using System;
	using System.Collections.Generic;
	using System.Diagnostics;
	using System.IO;
	using System.Linq;
	using System.Security;
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
			//TODO: Move to resources
			const string directoryCreationErrorMsg = "Warning: Failed to scan for plugins.";
			const string moduleLoadErrorMsg = "Warning: Failed to load plugin at path: ";

			Action<Exception> onError = (e) => {
				logger.Log(directoryCreationErrorMsg + Environment.NewLine + e, Category.Exception, Priority.High);
			};

			IEnumerable<String> subDirectories;
			try
			{
				subDirectories = Directory.EnumerateDirectories(@".\Plugins", "*", SearchOption.AllDirectories);
			}
			catch ( ArgumentOutOfRangeException e ) { onError(e); return; }
			catch (       ArgumentNullException e ) { onError(e); return; }
			catch (           ArgumentException e ) { onError(e); return; }
			catch (  DirectoryNotFoundException e ) { onError(e); return; }
			catch (        PathTooLongException e ) { onError(e); return; }
			catch (                 IOException e ) { onError(e); return; }
			catch (           SecurityException e ) { onError(e); return; }
			catch ( UnauthorizedAccessException e ) { onError(e); return; }

			foreach ( string path in subDirectories )
			{
				ModulePath = path;

				try
				{
					base.InnerLoad();
				}
				catch ( Exception e )
				{
					Console.WriteLine(moduleLoadErrorMsg + ModulePath + Environment.NewLine + e); return;

					#if DEBUG
					throw;
					#endif
				}
			}
		}

		public override void Validate ()
		{
			RemoveModulesWithDuplicateNames();

			base.Validate();
		}

		private void RemoveModulesWithDuplicateNames ()
		{
			//TODO: Resources
			const string nameConflictWarning = "Ignoring plugin '{0}' from path '{1}' because another plugin with the same name already exists.";

			var groupedModules = Modules.GroupBy(m => m.ModuleName);
			foreach ( var group in groupedModules )
			{
				foreach ( var moduleInfo in group.Skip(1) )
				{
					Items.Remove(moduleInfo);

					string message = string.Format(nameConflictWarning, moduleInfo.ModuleName, moduleInfo.Ref);
					logger.Log(message, Category.Warn, Priority.Low);
					Debugger.Break();
				}
			}
		}
	}
}