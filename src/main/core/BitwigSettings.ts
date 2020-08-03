export interface BitwigSettings {
    colors: {
        trackColor: string,
        trackSelectedInactiveColor: string,
        trackSelectedColor: string,
        deviceBackgroundColor: string,
        deviceHeaderColor: string,
        deviceHeaderSelectedColor: string,
        // deviceHandleSelectedInactiveColor: string,
        automationButtonColor: string,
        automationButtonDisabledColor: string
    }
}

export function loadSettings(): void {

}

export function saveSettings(settings: BitwigSettings): void {

}