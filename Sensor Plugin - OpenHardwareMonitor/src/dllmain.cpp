#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
// NOTE: It looks like it's not possible to update individual sensors. Updates
// happen at the hardware level.

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

template<typename T>
using CLRList = System::Collections::Generic::List<T>;

public ref struct State : ISensorPlugin, ISensorPluginInitialize, ISensorPluginUpdate
{
	Computer^            computer;
	CLRList<ISensor^>^   activeSensors;
	CLRList<IHardware^>^ activeHardware;

	value struct StackEntry
	{
		i32 index;
		array<IHardware^>^ hardware;
	};

	virtual void
	GetPluginInfo(PluginInfo& info)
	{
		info.name       = String_FromView("OHM Sensors");
		info.author     = String_FromView("akbyrd");
		info.version    = 1;
		info.lhmVersion = LHMVersion;
	}

	virtual b8
	Initialize(PluginContext& context, SensorPluginAPI::Initialize api)
	{
		computer       = gcnew Computer();
		activeSensors  = gcnew CLRList<ISensor^>;
		activeHardware = gcnew CLRList<IHardware^>;

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
		for (i32 i = 0; i < current.hardware->Length; i++)
		{
			current.index = i;

			// Hardware
			IHardware% currentHardware = *current.hardware[i];
			activeHardware->Add(%currentHardware);

			// Sensors
			array<ISensor^>^ ohmSensors = currentHardware.Sensors;
			for (i32 j = 0; j < ohmSensors->Length; j++)
			{
				ISensor% ohmSensor = *ohmSensors[j];
				activeSensors->Add(%ohmSensor);

				CLRString^ format;
				switch (ohmSensor.SensorType)
				{
					default:
						Assert(false);
						break;

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
				}
				// TODO: Don't update unless changed
				CLRString^ value  = CLRString::Format(format, ohmSensor.Value);

				Sensor sensor = {};
				String_FromCLR(sensor.name,       ohmSensor.Name);
				String_FromCLR(sensor.identifier, ohmSensor.Identifier->ToString());
				String_FromCLR(sensor.format,     value);
				api.RegisterSensors(context, sensor);
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

		return true;
	}

	virtual void
	Update(PluginContext& context, SensorPluginAPI::Update api)
	{
		Unused(context);

		for (i32 i = 0; i < State::activeHardware->Count; i++)
			State::activeHardware[i]->Update();

		// TODO: This is stupid. Something something activeSensors
		for (u32 i = 0; i < api.sensors.length; i++)
		{
			Sensor& sensor = api.sensors[i];
			ISensor% ohmSensor = *State::activeSensors[(i32) i];

			sensor.value = ohmSensor.Value.GetValueOrDefault();
		}
	}
};
