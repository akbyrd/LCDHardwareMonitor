#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
#include <string.h>

// NOTE: It looks like it's not possible to update individual sensors. Updates
// happen at the hardware level. */

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

template<typename T>
using SList = System::Collections::Generic::List<T>;

public ref struct State : ISensorPlugin, ISensorInitialize, ISensorUpdate, ISensorTeardown
{
	Computer^          computer;
	SList<ISensor^>^   activeSensors;
	SList<IHardware^>^ activeHardware;

	value struct StackEntry
	{
		i32 index;
		array<IHardware^>^ hardware;
	};

	virtual b32  Initialize(PluginContext*, SensorPlugin::InitializeAPI) { return true; }
	virtual void Update    (PluginContext*, SensorPlugin::UpdateAPI)     {}
	virtual void Teardown  (PluginContext*, SensorPlugin::TeardownAPI)   {}

	#if false
	virtual void
	Initialize(PluginContext* context, SensorPlugin::InitializeAPI* api)
	{
		computer       = gcnew Computer();
		activeSensors  = gcnew SList<ISensor^>;
		activeHardware = gcnew SList<IHardware^>;

		// ENABLE ALL THE THINGS!
		computer->CPUEnabled           = true;
		computer->FanControllerEnabled = true;
		computer->GPUEnabled           = true;
		computer->HDDEnabled           = true;
		computer->MainboardEnabled     = true;
		computer->RAMEnabled           = true;

		computer->Open();

		auto stack = gcnew Stack<StackEntry>(10);
		StackEntry current = { 0, computer->Hardware };
		// TODO: Use u32?
		for (i32 i = 0; i < current.hardware->Length; i++)
		{
			current.index = i;

			// Hardware
			IHardware^ currentHardware = current.hardware[i];
			activeHardware->Add(currentHardware);

			// Sensors
			array<ISensor^>^ ohmSensors = currentHardware->Sensors;
			for (i32 j = 0; j < ohmSensors->Length; j++)
			{
				ISensor^ ohmSensor = ohmSensors[j];
				activeSensors->Add(ohmSensor);

				String^ format;
				switch (ohmSensor->SensorType)
				{
					case SensorType::Clock:       format = "%i MHz";   break;
					case SensorType::Control:     format = "%s%%";     break;
					case SensorType::Data:        format = "%f";       break;
					case SensorType::Factor:      format = "%.2f";     break;
					case SensorType::Fan:         format = "%.0f RPM"; break;
					case SensorType::Flow:        format = "%.2f L/h"; break;
					case SensorType::Level:       format = "%.0f%%";   break;
					case SensorType::Load:        format = "%.0f%%";   break;
					case SensorType::Power:       format = "%.1f W";   break;
					case SensorType::SmallData:   format = "%s";       break;
					case SensorType::Temperature: format = "%.0f C";   break;
					case SensorType::Voltage:     format = "%.2f V";   break;
					default:                      format = "Unknown SensorType: " + ohmSensor->SensorType.ToString(); break;
				}
				// TODO: Format in C
				// TODO: Don't update unless changed
				String^ value  = String::Format(format, ohmSensor->Value);

				Sensor sensor = {};
				sensor.name       = (c8*) Marshal::StringToHGlobalAnsi(ohmSensor->Name).ToPointer();
				sensor.identifier = (c8*) Marshal::StringToHGlobalAnsi(ohmSensor->Identifier->ToString()).ToPointer();
				sensor.string     = (c8*) Marshal::StringToHGlobalAnsi(value).ToPointer();

				List_Append(s->sensors, sensor);
			}

			// Sub-Hardware
			array<IHardware^>^ subhardware = current.hardware[i]->SubHardware;
			if (subhardware != nullptr && subhardware->Length > 0)
			{
				stack->Push(current);
				current.hardware = subhardware;
				i = -1;
			}

			// Pop
			if (i >= current.hardware->Length - 1)
			{
				if (stack->Count > 0)
				{
					current = stack->Pop();
					i = current.index;
				}
			}
		}

		// TODO: Handle hardware and sensor changes
		//IHardware::SensorAdded
		//IHardware::SensorRemoved
		//computer->HardwareAdded   += OnHardwareAdded;
		//computer->HardwareRemoved += OnHardwareRemoved;
	}

	virtual void
	Update(PluginContext* context, SensorPlugin::UpdateAPI* api)
	{
		for (i32 i = 0; i < State::activeHardware->Count; i++)
			State::activeHardware[i]->Update();

		// TODO: This is stupid
		for (u32 i = 0; i < s->sensors.length; i++)
		{
			Sensor& sensor = s->sensors[i];

			String^ sensorName = gcnew String(sensor.name);
			for (i32 j = 0; j < State::activeSensors->Count; j++)
			{
				ISensor^ ohmSensor = State::activeSensors[j];
				if (ohmSensor->Name == sensorName)
				{
					sensor.value = ohmSensor->Value.GetValueOrDefault();
					break;
				}
			}
		}
	}

	virtual void
	Teardown(PluginContext* context, SensorPlugin::TeardownAPI* api)
	{
		for (u32 i = 0; i < s->sensors.length; i++)
		{
			Sensor& sensor = s->sensors[i];

			Marshal::FreeHGlobal((IntPtr) sensor.name);
			Marshal::FreeHGlobal((IntPtr) sensor.identifier);
			Marshal::FreeHGlobal((IntPtr) sensor.string);

			sensor = {};
		}
		List_Clear(s->sensors);
	}
	#endif
};
