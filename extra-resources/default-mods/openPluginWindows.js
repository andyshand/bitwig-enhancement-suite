/**
 * @name Open Plugin Windows
 * @id open-plugin-windows
 * @description Shortcuts for opening all plugin windows for a track
 * @category global
 */

let lowLatencyModeOn = false

Mod.registerAction({
    title: "Restore Open Plugin Windows",
    id: "restore-open-plugin-windows",
    description: `Restore all open plugin windows for the current track from the previous session.`,
    defaultSetting: {
        keys: ["Meta", "Alt", "O"]
    },
    action: async () => {
        restoreOpenedPluginsForTrack(Bitwig.currentTrack)
    }
})

const getFocusedPluginWindow = () => {
    const pluginWindows = Bitwig.getPluginWindowsPosition()
    return Object.values(pluginWindows).find(w => w.focused)
}
const toggleBypassFocusedPluginWindow = async () => {
    const focused = getFocusedPluginWindow()
    if (!focused) {
        return Bitwig.showMessage('No focused plugin window')
    }
    Bitwig.sendPacket({
        type: 'open-plugin-windows/toggle-bypass',
        data: {
            devicePath: focused.id
        }
    })
}

Mod.registerAction({
    title: "Toggle Bypass Focused Plugin Window",
    id: "toggle-bypass-focused-plugin-window",
    description: `Finds the focused plugin window in the device change and toggles its bypassed state.`,
    defaultSetting: {
        keys: ["0"]
    },
    action: toggleBypassFocusedPluginWindow
})

Mod.registerAction({
    title: 'Toggle low latency mode',
    id: 'toggle-low-latency-mode',
    description: 'Disables or enables all devices in the latency list',
    defaultSetting: {
        keys: ["F6"]
    },
    action: async () => {
        const listsByTrackName = (await Db.getCurrentProjectData() || {})
        const initiallySelectedTrack = Bitwig.currentTrack
        lowLatencyModeOn = !lowLatencyModeOn
        Bitwig.showMessage(`Low latency mode: ${lowLatencyModeOn ? 'On' : 'Off'}`)

        for (const track in listsByTrackName) {
            log(`Processing track: ${track}`)
            await Bitwig.sendPacketPromise({
                type: 'track/select',
                data: {
                    name: track,
                    scroll: false,
                    allowExitGroup: false,
                    enter: false
                }
            })
            const { data: { toggled }} = await Bitwig.sendPacketPromise({
                type: 'open-plugin-windows/toggle-devices-active',
                data: {
                    active: !lowLatencyModeOn,
                    deviceNames: listsByTrackName[track]
                }
            })
            if (toggled.length) {
                Bitwig.showMessage(`${lowLatencyModeOn ? `Deactivated` : `Activated`} ${toggled.join(', ')}`)
            }
        }
        await Bitwig.sendPacket({
            type: 'track/select',
            data: {
                name: initiallySelectedTrack,
                scroll: false,
                allowExitGroup: false,
                enter: false
            }
        })
    }
})

Mod.registerAction({
    title: "Toggle device in latency list",
    id: "toggle-device-in-latency-list",
    description: `Adds or removes the currently selected device from the latency list`,
    defaultSetting: {
        keys: ["F5"]
    },
    action: async () => {
        const listsByTrackName = (await Db.getCurrentProjectData() || {})
        const track = Bitwig.currentTrack
        const device = Bitwig.currentDevice
        if (!(track && device)) {
            return Bitwig.showMessage('No active device or track')
        }

        const deviceName = device.name
        const list = (listsByTrackName[track] || [])
        if (list.indexOf(deviceName) >= 0) {
            if (list.length === 1) {
                delete listsByTrackName[track]
                await Db.setCurrentProjectData(listsByTrackName)
            } else {
                await Db.setCurrentProjectData({
                    ...listsByTrackName,
                    [track]: list.filter(name => name !== deviceName)
                })
            }
            Bitwig.showMessage(`${track}/${deviceName} removed from latency list`)
        } else {
            await Db.setCurrentProjectData({
                ...listsByTrackName,
                [track]: list.concat(deviceName)
            })
            Bitwig.showMessage(`${track}/${deviceName} added to latency list`)
        }
        const newList = (await (Db.getCurrentProjectData()) || {})[track] || []
        // Bitwig.showMessage(`Latency list for ${track}: ${JSON.stringify(newList)}`)
    }
})

Mouse.on('mouseup', event => {
    if (event.button === 3) { 
        const intersection = event.intersectsPluginWindows()
        if (intersection) {
            if (!intersection.focused) {
                const position = {
                    x: intersection.x + intersection.w - 10,
                    y: intersection.y + 5
                }
                Mouse.click(0, position)
                Mouse.setPosition(event.x, event.y)
                toggleBypassFocusedPluginWindow()
            } else {
                toggleBypassFocusedPluginWindow()
            }
        }
    }
})

async function restoreOpenedPluginsForTrack(track) {
    const { positions } = await Db.getTrackData(track, { 
        modId: 'move-plugin-windows'
    })
    const windowIds = Object.keys(positions || {})
    if (windowIds.length) {
        const presetNames = windowIds.map(id => id.split('/').slice(-1).join('').trim())
        log(`Reopening preset names: ${presetNames.join(', ')}`)
        Bitwig.sendPacket({
            type: 'open-plugin-windows/open-with-preset-name',
            data: {
                presetNames: _.indexBy(presetNames)
            }
        })
    }
}

// Ensure we only attempt to automatically restore plugin positions
// once per project session. This obj gets reset when a new project
// is detected
let openedPluginsForTracks = {}

Bitwig.on('activeEngineProjectChanged', async () => {
    openedPluginsForTracks = {}    
    log('Project Changed')
})
Bitwig.on('selectedTrackChanged', debounce(async (track, prev) => {
    if (track in openedPluginsForTracks) {
        log('Track already has plugins opened')
        return
    }
    log('Reopening plugins for track ' + track)
    openedPluginsForTracks[track] = true
    restoreOpenedPluginsForTrack(track)
}, 1500))