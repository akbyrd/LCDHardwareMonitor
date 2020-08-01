# LCDHardwareMonitor
An application for designing and displaying custom content on an LCD screen. It is intended to be extremely customizable, allowing data from any conceivable source to be displayed along with Direct3D content. It is built entirely on a plugin based architecture to allow for easy extension and customization.

This project started with the desire for a far more capable alternative to LCDSysInfo. The primary use case is displaying CPU and GPU utilization in real time. Currently it is Windows-only but is likely to support Linux in the future. The software will remain free, but I intend to build and sell hardware kits for a plug-and-play experience.

## Status
* ~~WPF shell prototype~~
* ~~Plugin architecture~~
* ~~Content renderer~~
* ~~Choose hardware~~
* Hardware Communication
* GUI Application
* Core functionality
* More features
* More plugins
* Hardware Kit

![Renderer Preview](doc/progress/LCD%20Hardware%20Monitor%2006%20-%20Renderer%20In%20WPF.png)

![Source Layout](doc/progress/LCD%20Hardware%20Monitor%2001%20-%20Source%20Layout.png)

![Designer Layout](doc/progress/LCD%20Hardware%20Monitor%2000%20-%20Designer%20Layout.png)

![Widget Progress](doc/progress/LCD%20Hardware%20Monitor%2010%20-%20Widgets%20For%20Days.gif)

![Sensor Progress](doc/progress/LCD%20Hardware%20Monitor%2012%20-%20Actual%20Data.gif)

## Development Requirements
* Visual Studio 2019
* Windows 10 SDK
* DirectX 11 SDK
* Windows Runtime C++ Template Library (WRL)
* C++/CLI
* .NET 4.7.2 targeting pack
* .NET 4.7.2 SDK

## Third Party Dependencies
(all included or automatic)
* FTDI d2xx driver (included with Windows)
* FTDI d2xx headers and import library (included)
* OpenHardwareMonitor.dll (included)
* MahApps.Metro (automatic through NuGet)
* ControlzEz (automatic through NuGet)

## Getting Started
If the development requirements are installed you shouldn't need to do anything other than open the Visual Studio solution and compile.

## License
Uh, it's free and stuff. I'll figure this out later.
