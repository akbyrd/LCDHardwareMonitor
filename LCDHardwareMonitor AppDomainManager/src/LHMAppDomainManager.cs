using System;
using System.Reflection;
using System.Runtime.InteropServices;

[ComVisible(true)]
[Guid("9D931427-5079-4437-89A5-165A99B14CC5")]
public interface ILHMAppDomainManager
{
	int LoadPlugin(string name, string directory, out IntPtr updateFn);
}

[ComVisible(true)]
[Guid("D7D75EEB-F3A3-4DA2-B7FF-82623D1304F8")]
public sealed class LHMAppDomainManager : AppDomainManager, ILHMAppDomainManager
{
	public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
	{
		InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
	}

	public int LoadPlugin(string name, string directory, out IntPtr updateFn)
	{
		/* NOTE: LHMAppDomainManager is going to get loaded into each new
		 * AppDomain so we need let ApplicationBase get inherited from the default
		 * domain in order for it to be found. Instead, we set PrivateBinPath
		 * the actual plugin can be found when we load it. */
		var domainSetup = new AppDomainSetup();
		domainSetup.PrivateBinPath     = directory;
		domainSetup.LoaderOptimization = LoaderOptimization.MultiDomainHost;

		//TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain domain = CreateDomain(name, null, domainSetup);
		var pluginManager = (LHMAppDomainManager) domain.DomainManager;
		//NOTE: This is probably slow but it only happens once per plugin, so meh
		pluginManager.InitializePlugin(name, directory, out updateFn);
		return domain.Id;
	}

	private delegate void UpdateFn(IntPtr unused);
	UpdateFn updateDelegate;

	private int InitializePlugin(string name, string directory, out IntPtr updateFn)
	{
		//NOTE: Function calls average 200ns with this pattern
		var plugin = Assembly.Load(name);
		updateDelegate = (_) => Update(_);
		updateFn = Marshal.GetFunctionPointerForDelegate(updateDelegate);
		return AppDomain.CurrentDomain.Id;
	}

	public void Update (IntPtr unused)
	{
	}
}
