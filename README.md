# Modwig 

_In Early Development_

Modwig adds helpful keyboard shortcuts and extra features to Bitwig Studio. Download the latest version from the [releases page](https://github.com/andyshand/modwig/releases).

![Screenshot of Modwig](https://github.com/andyshand/modwig/raw/master/github/screenshots/screenshot.png)

### Extra Shortcuts 

Shortcuts for actions only accessible through Controller API, such as collapsing devices, navigating device slots/layers, closing all plugin windows.

### VST Pass-through for Shortcuts

By default, Bitwig blocks certain shortcuts from working when VST windows are focused. With Modwig, you can bypass this and allow any shortcut to work when you expect.

### Create Custom Macros

Combine all available actions and shortcuts into your own macros using simple Javascript. For example, add a track, open device browser, type "EQ" and hit enter. See [the wiki](https://github.com/andyshand/modwig/wiki/Creating-a-Custom-Mod#running-other-actions-macros) for details.

### Project Track Search

For projects with many tracks, easily find what you're looking for using the project-wide track search. You can also change volume, solo and mute tracks without losing your place.

### Workflow Features

Middle-click to play from anywhere in the arranger view, exclusive-arm support, persistent device view scroll when switching tracks, and more. 

### Simple, Guided Setup Process

The first time you open Modwig, it will guide you through a simple 3 step process which automatically copies over a controller script to your User Library folder.

# Setup

1. Download the latest version of the app from the [releases tab](https://github.com/andyshand/modwig/releases) and copy it to your Applications folder.

2. Open Modwig and follow the guided setup process. The app is unsigned, so you may get a warning from macOS and go to your "Security & Privacy" settings and click "Open Anyway".

# Complete List of Shortcuts:

### Global

- Open Track Search
- Toggle Record
- Go Back
- Go Forward
- Select Previous Track
- Select Next Track
- Enter
- Arrow Up
- Arrow Down
- Arrow Left
- Arrow Right

### Arranger

- Toggle Large Track Height

### Browser

- Open Device Browser
- Clear Browser Filters
- Confirm Browser
- Previous Browser Tab
- Next Browser Tab
- Select Browser Tab 1...6

### Devices

- Focus Device Panel
- Select First Device
- Select Last Device
- Insert Device at Start
- Insert Device at End
- Collapse Selected Device
- Expand Selected Device
- Collapse All Devices in Chain
- Expand All Devices in Chain
- Close All Plugin Windows
- Navigate to Parent Device
- Select Device Slot 1...8
- Select Device Layer 1...8

# How It Works

BES in an electron application that uses C++ extensions to listen for global keypresses and communicates with Bitwig via its controller API. For some functions, plain mouse/keyboard automation is used. Some future features will likely require screenshot analysis.

# Contributing

Standard fork and pull request is fine. Please see the separate CONTRIBUTING.md for help with contributing, development setup.
