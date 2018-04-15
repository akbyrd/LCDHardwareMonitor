using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;

[ComVisible(true)]
[Guid("9D931427-5079-4437-89A5-165A99B14CC5")]
public interface ILHMAppDomainManager
{
	void LoadPlugin(string name, string directory);
	void UnloadPlugin(string name, string directory);
}

[ComVisible(true)]
[Guid("D7D75EEB-F3A3-4DA2-B7FF-82623D1304F8")]
public sealed class LHMAppDomainManager : AppDomainManager, ILHMAppDomainManager
{
	public override void InitializeNewDomain(AppDomainSetup appDomainInfo)
	{
		InitializationFlags = AppDomainManagerInitializationOptions.RegisterWithHost;
	}

	public void LoadPlugin(string name, string directory)
	{
		Debug.WriteLine("[{0}] {1}", AppDomain.CurrentDomain.FriendlyName, "Hello World!");

		var domainSetup = new AppDomainSetup();
		//domainSetup.ActivationArguments;
		//domainSetup.ApplicationName
		//domainSetup.ApplicationBase  = directory;
		domainSetup.PrivateBinPath     = directory;
		domainSetup.LoaderOptimization = LoaderOptimization.MultiDomainHost;

		//TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		//TODO: Prepend "[Plugin]"
		AppDomain domain   = CreateDomain(name, null, domainSetup);
		//TODO: Can't load here because we're in the default AppDomain
		//Assembly  assembly = domain.Load("OpenHardwareMonitor Plugin");
		//TODO : Store domain in Plugin
	}

	public void UnloadPlugin(string name, string directory)
	{
	}
}
