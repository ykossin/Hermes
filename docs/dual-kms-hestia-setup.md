# Hermes-KMS dual Hestia setup

Example deployment: a Hermes host with Hermes-KMS and two simultaneous Hestia clients (physical monitor plus two virtual outputs).

## Stack

| Component | Role |
|-----------|------|
| `hermes` | Streaming host (Apollo-compatible, Hestia extensions) |
| `hermes-kms-dkms-git` | DRM virtual outputs (`Virtual-1`, `Virtual-2`) |
| User systemd unit (optional) | Keeps a baseline 4K virtual desktop via `hermes-kmsctl hold` |
| Hestia | Desktop client on other machines |

Sunshine may remain installed but is not used when `hermes.service` is active. Config lives in `~/.config/hermes/`, not `~/.config/sunshine/`.

## Example `hermes.conf`

See [examples/dual-kms-hestia/hermes.conf](../examples/dual-kms-hestia/hermes.conf).

Key options:

- `virtual_display_backend = hermes_kms`
- `hermes_kms_multi_output = true` — two clients without killing the first session
- `isolated_virtual_display_option = true`
- `capture = kms`
- Connector layout and physical capture fallbacks are configurable (no hardcoded names in code)

Add your LAN host to `csrf_allowed_origins` locally if you open the Web UI from another machine.

## Example `apps.json`

See [examples/dual-kms-hestia/apps.json](../examples/dual-kms-hestia/apps.json).

| App | Purpose |
|-----|---------|
| Desktop Real | Physical monitor (`capture-display` from config) |
| Desktop Virt | Existing `Virtual-1` headless desktop |
| Monitor 1 / 2 | Session-scoped virtual outputs |
| Cursor IDE | Launches on virtual display |

Adjust connector names (`DP-1`, `Virtual-1`, …) to match `drm-info` on your host.

## Build and install (Arch)

```bash
git clone https://github.com/ykossin/Hermes.git
cd Hermes
makepkg -si
systemctl --user enable --now hermes.service
```

Optional headless baseline (example user unit):

```bash
hermes-kmsctl hold 3840x2160@60
```

Run that from a user `@reboot` script or a small systemd user service on your machine.

## Upstream

[MrOz59/Hermes](https://github.com/MrOz59/Hermes). This fork adds dual-output patches with config-driven connector names on `main`.

## Notes

1. Rebuild the package after pulling; do not rely on an old working tree alone.
2. Legacy `~/.config/sunshine` can be removed; Hermes uses `~/.config/hermes/`.
3. Permanent `hermes-kmsctl hold` plus session Virtual-N outputs: watch for layout races when two clients connect; check `hermes.log`.
4. Software encoder fallback: if VAAPI or NVENC is unavailable, Hermes may use `libx264`; check Audio/Video in the Web UI.
5. Physical connector probe uses `/sys/class/drm`; set `physical_capture_probe_connector` if your cable is not `DP-1`.
6. Before major upgrades, rebase onto MrOz59/main and run unit tests under `tests/unit/platform/`.
