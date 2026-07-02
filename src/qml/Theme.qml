pragma Singleton
import QtQuick

// Shared visual tokens for the Melody UI.
// Keep raw magic numbers out of components — reference Theme.* instead.
QtObject {
    // ── Corner radii ──
    readonly property int radiusXs: 6
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusPill: 23

    // ── Motion durations (ms) ──
    readonly property int durFast: 90
    readonly property int durNormal: 120
    readonly property int durSlow: 160
    readonly property int durPage: 260

    // ── Spacing ──
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16

    // ── Surfaces ──
    readonly property color surfaceBase: "#0d0d12"
    readonly property color surfaceRaised: "#19191f"
    readonly property color surfacePopup: "#1e1e24"

    // ── White-alpha overlays ──
    readonly property color overlay06: Qt.rgba(1,1,1,0.06)
    readonly property color overlay09: Qt.rgba(1,1,1,0.09)
    readonly property color overlay10: Qt.rgba(1,1,1,0.10)
    readonly property color overlay12: Qt.rgba(1,1,1,0.12)
    readonly property color overlay18: Qt.rgba(1,1,1,0.18)

    // ── Text ──
    readonly property color textPrimary: "white"
    readonly property color textSecondary: Qt.rgba(1,1,1,0.55)
    readonly property color textTertiary: Qt.rgba(1,1,1,0.45)

    // ── Accents / semantics ──
    readonly property color accentLight: "#d1d5db"   // progress / spinner
    readonly property color accentDark: "#9ca3af"    // progress gradient end
    readonly property color errorRed: "#ff6b6b"      // errors only
    readonly property color badgeNetease: "#e05555"
    readonly property color badgeBilibili: "#fb7299"
    readonly property color closeHover: "#e74c3c"    // destructive close hover

    // ── Font ──
    readonly property string fontMain: "HarmonyOS Sans SC"
}
