# 10days

## Build and run

This repository contains only the game project. Engine binaries and SDK files are installed by CalyxLauncher from the CalyxEngine GitHub release that matches `.calyxproj` `engineVersion`.

1. Open `10days.sln` in Visual Studio.
2. Build `CalyxLauncher` when you want to install or update the engine package.
3. Set `10days` as the startup project and run Debug, Develop, or Release.

`10days` builds the game code as a DLL under `Generated/Outputs/<Configuration>` and copies the engine-side host there as `10days.exe`. Run the executable from the repository/package tree so it can discover `10days.calyxproj` and load the matching DLL.

Default SDK path: `%LOCALAPPDATA%\\CalyxEngine\\Engines\\v1.1.6\\SDK`.
`CALYX_ENGINE_SDK_DIR` can override this path for local SDK testing.

Debug and Develop builds are intended for editor/development UI. Release builds are intended to run without that GUI.
