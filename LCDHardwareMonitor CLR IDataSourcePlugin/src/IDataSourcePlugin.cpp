#pragma managed
using namespace System;

public interface class IDataSourcePlugin
{
	void Initialize(IntPtr plugin);
	void Update(IntPtr plugin);
	void Teardown(IntPtr plugin);
};
