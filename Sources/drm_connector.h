#pragma once

/**
 * enum drm_connector_status - status for a &drm_connector
 *
 * This enum is used to track the connector status. There are no separate
 * #defines for the uapi!
 */
enum drm_connector_status {
    /**
     * @connector_status_connected: The connector is definitely connected to
     * a sink device, and can be enabled.
     */
    connector_status_connected = 1,
    /**
     * @connector_status_disconnected: The connector isn't connected to a
     * sink device which can be autodetect. For digital outputs like DP or
     * HDMI (which can be realiable probed) this means there's really
     * nothing there. It is driver-dependent whether a connector with this
     * status can be lit up or not.
     */
    connector_status_disconnected = 2,
    /**
     * @connector_status_unknown: The connector's status could not be
     * reliably detected. This happens when probing would either cause
     * flicker (like load-detection when the connector is in use), or when a
     * hardware resource isn't available (like when load-detection needs a
     * free CRTC). It should be possible to light up the connector with one
     * of the listed fallback modes. For default configuration userspace
     * should only try to light up connectors with unknown status when
     * there's not connector with @connector_status_connected.
     */
    connector_status_unknown = 3,
};
