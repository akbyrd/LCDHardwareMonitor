#pragma managed

#define EXPORTING 1
#include "LHMDataSource.h"
#include <string.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

ref struct State
{
	static Computer computer;
};

value struct StackEntry
{
	i32 index;
	array<IHardware^>^ hardware;
};

void
Initialize(::List<Sensor>& sensors)
{
	Computer^ computer = %State::computer;

	//ENABLE ALL THE THINGS!
	computer->CPUEnabled           = true;
	computer->FanControllerEnabled = true;
	computer->GPUEnabled           = true;
	computer->HDDEnabled           = true;
	computer->MainboardEnabled     = true;
	computer->RAMEnabled           = true;

	computer->Open();

	auto stack = gcnew Stack<StackEntry>;
	StackEntry current = { 0, computer->Hardware };
	for (i32 i = 0; i < current.hardware->Length; i++)
	{
		current.index = i;

		//Sensors
		array<ISensor^>^ ohmSensors = current.hardware[i]->Sensors;
		for (i32 j = 0; j < ohmSensors->Length; j++)
		{
			ISensor^ ohmSensor = ohmSensors[j];

			String^ format;
			switch (ohmSensor->SensorType)
			{
				case SensorType::Clock:       format = L"%i MHz"; break;
				case SensorType::Control:     format = L"%s%%";   break;
				case SensorType::Data:        format = L"%f";     break;
				case SensorType::Factor:      format = L"%f";     break;
				case SensorType::Fan:         format = L"%f RPM"; break;
				case SensorType::Flow:        format = L"%f L/h"; break;
				case SensorType::Level:       format = L"%f%%";   break;
				case SensorType::Load:        format = L"%f%%";   break;
				case SensorType::Power:       format = L"%f W";   break;
				case SensorType::SmallData:   format = L"%s";     break;
				case SensorType::Temperature: format = L"%f C";   break;
				case SensorType::Voltage:     format = L"%f V";   break;
				//TODO: Print actual sensor type
				default:                      format = L"Unknown Sensor Type"; break;
			}

			Sensor sensor = {};
			sensor.name          = (c16*) Marshal::StringToHGlobalUni(ohmSensor->Name).ToPointer();
			sensor.identifier    = (c16*) Marshal::StringToHGlobalUni(ohmSensor->Identifier->ToString()).ToPointer();
			sensor.displayFormat = (c16*) Marshal::StringToHGlobalUni(format).ToPointer();

			List_Append(sensors, sensor);
		}

		//Sub-Hardware
		array<IHardware^>^ subhardware = current.hardware[i]->SubHardware;
		if (subhardware != nullptr && subhardware->Length > 0)
		{
			stack->Push(current);
			current.index = 0;
			current.hardware = subhardware;
		}

		//Pop
		if (i >= current.hardware->Length - 1)
		{
			if (stack->Count > 0)
			{
				current = stack->Pop();
				i = current.index;
			}
		}
	}

	//IHardware::SensorAdded
	//IHardware::SensorRemoved
	//computer->HardwareAdded   += OnHardwareAdded;
	//computer->HardwareRemoved += OnHardwareRemoved;
}

void
Update()
{
}

void
Teardown(::List<Sensor>& sensors)
{
	for (i32 i = 0; i < sensors.length; i++)
	{
		Sensor& sensor = sensors.items[i];

		Marshal::FreeHGlobal((IntPtr) sensor.name);
		Marshal::FreeHGlobal((IntPtr) sensor.identifier);
		Marshal::FreeHGlobal((IntPtr) sensor.displayFormat);

		sensor = {};
	}
	List_Clear(sensors);
}