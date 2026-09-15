# Hermes-KMS: dual Hestia, настройка

> Русская версия. English: [dual-kms-hestia-setup.md](dual-kms-hestia-setup.md)

Пример: хост Hermes с Hermes-KMS и два одновременных клиента Hestia (физический монитор плюс два виртуальных выхода).

## Стек

| Компонент | Роль |
|-----------|------|
| `hermes` | Стриминг-хост (Apollo-совместимый, расширения Hestia) |
| `hermes-kms-dkms-git` | Виртуальные DRM-выходы (`Virtual-1`, `Virtual-2`) |
| User systemd unit (опционально) | Базовый 4K desktop через `hermes-kmsctl hold` |
| Hestia | Клиент на других машинах |

Sunshine может стоять, но при активном `hermes.service` не используется. Конфиг в `~/.config/hermes/`, не в sunshine.

## Пример `hermes.conf`

См. [examples/dual-kms-hestia/hermes.conf](../examples/dual-kms-hestia/hermes.conf).

Ключевые опции:

- `virtual_display_backend = hermes_kms`
- `hermes_kms_multi_output = true` — два клиента без убийства первой сессии
- `isolated_virtual_display_option = false` для shared multi-output
- `capture = kms`
- Имена коннекторов и fallback настраиваются в конфиге

`csrf_allowed_origins` и `sunshine_name` под LAN добавляй локально, в git не коммить.

## Пример `apps.json`

См. [examples/dual-kms-hestia/apps.json](../examples/dual-kms-hestia/apps.json).

| App | Назначение |
|-----|------------|
| Desktop Real | Физический монитор (`capture-display` из конфига) |
| Desktop Virt | Headless на `Virtual-1` |
| Monitor 1 / 2 | Session-scoped виртуальные выходы |
| Cursor IDE | Запуск на virtual display |

Имена коннекторов (`DP-1`, `Virtual-1`, …) сверь с `drm-info` на хосте.

## Сборка и установка (Arch)

### 1. Модуль Hermes-KMS

```bash
git clone https://github.com/ykossin/Hermes-KMS.git
cd Hermes-KMS
makepkg -si
sudo modprobe hermes_kms initial_enabled=0 outputs=2
```

`hermes-kmsctl` в `~/.local/bin` из пакета или `tools/hermes-kmsctl` в том же репо.

Upstream: [MrOz59/Hermes-KMS](https://github.com/MrOz59/Hermes-KMS). Fork [ykossin/Hermes-KMS](https://github.com/ykossin/Hermes-KMS) без host-specific правок.

### 2. Hermes

```bash
git clone https://github.com/ykossin/Hermes.git
cd Hermes
makepkg -si
cp examples/dual-kms-hestia/hermes.conf ~/.config/hermes/hermes.conf
cp examples/dual-kms-hestia/apps.json ~/.config/hermes/apps.json
systemctl --user enable --now hermes.service
```

### 3. Headless baseline (опционально)

Скопируй [headless-desktop.sh](../examples/dual-kms-hestia/headless-desktop.sh) в `~/.local/bin/`, chmod +x, включи [headless-desktop.service](../examples/dual-kms-hestia/headless-desktop.service) в `~/.config/systemd/user/`.

## Пересборка на Arch

Если `makepkg` падает на `third-party/tray`:

```bash
./scripts/restore-tray-submodule.sh
makepkg -f --noconfirm
sudo pacman -U hermes-streaming-*-x86_64.pkg.tar.zst
systemctl --user restart hermes.service
```

## Forks

| Repo | Роль |
|------|------|
| [ykossin/Hermes](https://github.com/ykossin/Hermes) | Dual-output, config-driven connectors |
| [ykossin/Hermes-KMS](https://github.com/ykossin/Hermes-KMS) | DRM virtual display |

Upstream: [MrOz59/Hermes](https://github.com/MrOz59/Hermes), [MrOz59/Hermes-KMS](https://github.com/MrOz59/Hermes-KMS).

## Проверка после установки

```bash
hermes-kmsctl version    # uapi_version >= 11 (13 на текущем драйвере)
hermes-kmsctl caps       # output_count=2
lsmod | grep hermes_kms
getfacl /dev/dri/renderD128
```

Udev rule `72-hermes-kms-render-access.rules` из репо Hermes-KMS. Без render-node Hermes пишет Permission denied на renderD128.

Modprobe для shared multi-output:

```bash
# /etc/modprobe.d/hermes-kms.conf
options hermes_kms initial_enabled=0 outputs=2 hotplug_events=Y
```

Не смешивай с `hermes-kms-setup configure --session-devices N`, если не переключаешься на isolated sessions.

Hermes должен работать внутри Plasma session. После обновления ядра пересобери DKMS и reboot.

## Заметки

1. После pull пересобирай пакет.
2. Legacy `~/.config/sunshine` можно убрать.
3. При двух клиентах смотри layout races в `hermes.log`.
4. Fallback на software encoder, если VAAPI недоступен.
5. `physical_capture_probe_connector` настрой под свой кабель.

На Renoir физическая панель в sysfs может быть `card2-DP-1`, в apps.json по-прежнему `DP-1`.
6. Перед major upgrade: rebase на MrOz59/main и unit tests.

## Разрешение и автоскейл

Hermes получает размер стрима от клиента (`width`, `height`, `fps` из Moonlight или Hestia `session/prepare`).

### Virtual tiles (Monitor 1, Monitor 2, Cursor IDE)

Session-scoped выход Hermes-KMS создаётся в `prepare_session_virtual_display()` по `launch_session->width` x `height`. Клиент 1920x1080 даёт virtual 1920x1080, не 3840.

`hermes_kms_default_virtual_width` только для KScreen layout. На стрим не влияет. Для меньшей idle-нагрузки ставь 1920.

### Physical tile (Desktop Real)

Capture в native mode `DP-1` (часто 4K). Если клиент меньше:

1. **Encode-scale (сейчас):** capture 4K, VAAPI в размер клиента. Просто, но грузит iGPU.
2. **Host modeset:** `dd.configuration_option = ensure_active`, `dd.resolution_option = automatic` и Optimize game settings в клиенте.

### Hestia clients

`POST /api/hestia/session/prepare` перед launch. `client.display_width/height` clamp upscale: если requested больше панели клиента, сохраняется min(requested, display).

### Практика на kossin

| Сценарий | Рекомендация |
|----------|--------------|
| Телефон 1080p на Monitor 1 | Virtual 1080p, zero-copy |
| 1080p на Desktop Real 4K | modeset DP-1 или encode-scale |
| Два клиента | 4K + 1080p, не два 4K HEVC на Renoir |
| Клиент хочет 4K, хост 1080p | Cap по native, upscale не делать |

### Дальше в коде

`resolve_session_render_size()` для physical tiles: native mode коннектора и `configure_display()` при `dd.resolution_option = automatic`.
