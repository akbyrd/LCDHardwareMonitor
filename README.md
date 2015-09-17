# LCDHardwareMonitor
A WPF application, rendering engine, and USB driver for displaying custom content on an LCD screen. It is intended to be extremely customizable, allowing data from any conceivable source to be displayed along with Direct3D content. It is built entirely on a plugin based architecture to allow for easy extension and customization.

## About
This project started with the desire for a far more capable alternative to LCDSysInfo. It has evolved into an excuse to learn Git, WPF, MVVM, Direct3D, and USB drivers. You'll have to excuse the slow progress as I find time figure all these things out from scratch. Feel free to point out my mistakes! While the code will remain open source, I intend to build the accompanying hardware and sell it as a ready-to-go package with the software. It'll probably be Windows only, but I'm not ruling out other platforms entirely.

## Status
* ~~WPF shell~~
* ~~Plugin archtecture~~
* Content renderer (in progress)
* Choose hardware
* USB driver
* More WPF features
* More plugins

## Getting Started
You shouldn't need to do anything other than install the [DirectX SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812). Everything else will be pulled down through NuGet (Prism, Modern UI) or a submodule (OpenHardwareMonitor). **Correction** there's currently a dependency on System.Windows.Interactivity that I need to get rid of. In the short term the offending code can simply be disabled.

## Requirements
* [DirectX SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812)

## Resources
* [Modern UI for WPF](https://github.com/firstfloorsoftware/mui)
* [Prism](https://msdn.microsoft.com/en-us/library/gg406140.aspx)
* [OpenHardwareMonitor](http://openhardwaremonitor.org/) ([GitHub clone](https://github.com/Sycobob/OpenHardwareMonitor))

## License
Uh, it's free and stuff. I'll figure this out later.
