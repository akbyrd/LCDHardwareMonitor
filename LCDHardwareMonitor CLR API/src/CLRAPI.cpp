#pragma managed
using namespace System;

public interface class IDataSourcePlugin
{
	void Initialize ();
	void Update     ();
	void Teardown   ();
};
