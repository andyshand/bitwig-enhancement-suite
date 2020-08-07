interface Event {
    type: string
}
interface KeyboardEvent extends Event {
    type: 'keydown' | 'keyup'
    keyCode: string
    metaKey: boolean
    shift: boolean
    alt: boolean
}
interface MouseEvent extends Event {
    type: 'mouseup' | 'mousedown',
    screenX: number,
    screenY: number
}
type EventHandler = (event: Event) => void
const listeners: {
    type: string,
    func: EventHandler
}[] = []

export function addGlobalEventListener(type: string, handler: EventHandler) {
    listeners.push({
        type,
        func: handler
    })
}

