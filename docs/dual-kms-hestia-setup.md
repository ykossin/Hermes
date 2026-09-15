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
- `isolated_virtual_display_option = false` for shared multi-output (default here)
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

### 1. Hermes-KMS kernel module

```bash
git clone https://github.com/ykossin/Hermes-KMS.git
cd Hermes-KMS
makepkg -si
sudo modprobe hermes_kms initial_enabled=0 outputs=2
```

Install `hermes-kmsctl` to `~/.local/bin` from the package build or `tools/hermes-kmsctl` in the same repo.

Upstream driver: [MrOz59/Hermes-KMS](https://github.com/MrOz59/Hermes-KMS). Fork [ykossin/Hermes-KMS](https://github.com/ykossin/Hermes-KMS) tracks it without host-specific changes.

### 2. Hermes streaming host

```bash
git clone https://github.com/ykossin/Hermes.git
cd Hermes
makepkg -si
cp examples/dual-kms-hestia/hermes.conf ~/.config/hermes/hermes.conf
cp examples/dual-kms-hestia/apps.json ~/.config/hermes/apps.json
systemctl --user enable --now hermes.service
```

Merge `csrf_allowed_origins` and `sunshine_name` with your LAN hostname locally; do not commit those values.

### 3. Headless baseline (optional)

Copy [headless-desktop.sh](../examples/dual-kms-hestia/headless-desktop.sh) to `~/.local/bin/`, chmod +x, and enable [headless-desktop.service](../examples/dual-kms-hestia/headless-desktop.service) under `~/.config/systemd/user/`.


## Rebuild on Arch

If `makepkg` fails on missing `third-party/tray`, restore the submodule before build:

```bash
git submodule update --init third-party/tray
# or copy from a full clone:
cp -a ../_deprecated-Hermes/third-party/tray third-party/
makepkg -f --noconfirm
sudo pacman -U hermes-streaming-*-x86_64.pkg.tar.zst
systemctl --user restart hermes.service
```

## Forks

| Repo | Role |
|------|------|
| [ykossin/Hermes](https://github.com/ykossin/Hermes) | Dual-output Hermes, config-driven connectors |
| [ykossin/Hermes-KMS](https://github.com/ykossin/Hermes-KMS) | DRM virtual display module (mirror of upstream) |

Upstream: [MrOz59/Hermes](https://github.com/MrOz59/Hermes), [MrOz59/Hermes-KMS](https://github.com/MrOz59/Hermes-KMS).


## Post-install verification

After installing or upgrading Hermes-KMS:

```bash
hermes-kmsctl version    # uapi_version must be >= 11 (13 with current driver)
hermes-kmsctl caps       # output_count=2 for dual Hestia
lsmod | grep hermes_kms
getfacl /dev/dri/renderD128   # active desktop user needs rw after login
```

Install udev rule `72-hermes-kms-render-access.rules` from the Hermes-KMS repo. Without render-node access Hermes logs `Permission denied` on renderD128 and Hermes-KMS virtual sessions stay disabled.

Modprobe for **shared multi-output** (this guide):

```bash
# /etc/modprobe.d/hermes-kms.conf
options hermes_kms initial_enabled=0 outputs=2 hotplug_events=Y
```

Do not mix this with `hermes-kms-setup configure --session-devices N` unless you switch to isolated session cards (`hermes_kms_isolated_sessions = true`).

Hermes must run inside the Plasma session (`systemctl --user restart hermes.service` after graphical login). After kernel upgrades rebuild the DKMS package and reboot.

## Notes

1. Rebuild the package after pulling; do not rely on an old working tree alone.
2. Legacy `~/.config/sunshine` can be removed; Hermes uses `~/.config/hermes/`.
3. Permanent `hermes-kmsctl hold` plus session Virtual-N outputs: watch for layout races when two clients connect; check `hermes.log`.
4. Software encoder fallback: if VAAPI or NVENC is unavailable, Hermes may use `libx264`; check Audio/Video in the Web UI.
5. Physical connector probe uses `/sys/class/drm`; set `physical_capture_probe_connector` if your cable is not `DP-1`.
6. Before major upgrades, rebase onto MrOz59/main and run unit tests under `tests/unit/platform/`.

## Resolution and auto-scaling

Hermes already receives the client stream size on every launch (`width`, `height`, `fps` from Moonlight, or Hestia `session/prepare` with `requested_width` and `requested_height`). The open gap is **where** that size is applied: virtual outputs, physical DP-1, or only at the encoder.

### Virtual tiles (Monitor 1, Monitor 2, Cursor IDE)

Session-scoped Hermes-KMS outputs are created at `launch_session->width` x `launch_session->height` in `prepare_session_virtual_display()`. If the client picks 1920x1080, the virtual connector should run at 1920x1080, not at `hermes_kms_default_virtual_width`.

`hermes_kms_default_virtual_width = 3840` is only used for KScreen layout math when the output size is not known yet. It does not force the stream to 4K. For less idle overhead you can lower it to 1920; streaming still follows the client request.

### Physical tile (Desktop Real)

Capture is always at the native mode of `DP-1` (often 3840x2160). If the client asks for less:

1. **Encode-scale (default today):** KMS captures full 4K, VAAPI encodes to the client size. Simple, works without changing the desktop, but costs more GPU on Renoir.
2. **Host modeset (better for a 1080p phone or tablet):** enable display-device resolution change in `hermes.conf` (`dd.configuration_option = ensure_active`, `dd.resolution_option = automatic`) and turn on Optimize game settings in the client. Hermes modesets `DP-1` for the session and restores on exit.

If the client asks for **more** than the host panel or connector supports, clamp to the native max mode and log a warning. Upscaling in the encoder is possible but wastes bandwidth and looks soft; prefer capping in Hestia prepare or at launch.

### Hestia clients

Use `POST /api/hestia/session/prepare` before launch. Pass `stream.requested_width/height` for the encode target. Fields `client.display_width/height` are validated today but not yet used to clamp; a sensible rule is `effective = min(requested, host_native_max)` for upscale attempts and `effective = requested` for downscale on virtual outputs.

### Practical defaults on kossin

| Scenario | Recommendation |
|----------|----------------|
| Phone or tablet 1080p on Monitor 1 | Client 1920x1080; virtual output matches; zero-copy at 1080p |
| Laptop 1080p on Desktop Real 4K | Prefer host modeset to 1080p, or accept encode-scale |
| Two clients at once | Keep one 4K + one 1080p, not two 4K HEVC on Renoir |
| Client wants 4K, host is 1080p | Cap at 1080p native; do not upscale unless explicitly needed |

### Future code (fork)

Single helper `resolve_session_render_size()` shared by virtual prepare and physical launch: query connector native mode, clamp requested, optionally trigger `configure_display()` for physical tiles when `dd.resolution_option = automatic`.
