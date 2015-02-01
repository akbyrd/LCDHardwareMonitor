using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using Microsoft.Management.Infrastructure;
using OpenHardwareMonitor.Hardware;

namespace LCDHardwareMonitor
{
	class WMIReader
	{
		PropertyInfo[] hardwarePropertyInfos;
		IHardware[] hardware;

		public WMIReader ()
		{
			PrepareReflection();

			CimSession cimSession = CimSession.Create("localhost");
			IEnumerable<CimInstance> instances = cimSession.EnumerateInstances(@"root\OpenHardwareMonitor", "Hardware");

			hardware = new IHardware[instances.Count()];

			int i = 0;
			foreach ( CimInstance instance in instances )
			{
				Console.WriteLine("Instance " + i);
				for ( int j = 0; j < hardwarePropertyInfos.Length; ++j )
				{
					PropertyInfo propertyInfo = hardwarePropertyInfos[j];

					CimProperty cimProperty = instance.CimInstanceProperties[propertyInfo.Name];
					string value = cimProperty != null ? cimProperty.Value.ToString() : "Not Present";

					Console.WriteLine(propertyInfo.Name + ": " + value);
				}
				Console.WriteLine();

				++i;
			}
		}

		private void PrepareReflection ()
		{
			hardwarePropertyInfos = typeof(IHardware).GetProperties();
		}
	}
}