# Agent Skeleton Architecture

Additive ESP-IDF skeleton for an ESP32-S3 smart-hardware AI agent. Interfaces and stubs live under `main/agent/`; production audio/protocol code remains in `main/audio/` and `main/protocols/`.

Enable with `CONFIG_AGENT_SKELETON_ENABLE` (menuconfig → Xiaozhi Assistant → Agent Skeleton).

## Goals

- Real-time voice interaction on device
- Custom voice configuration hook
- Cloud LLM as personal assistant (ASR → LLM → TTS)
- Expandable camera / mic / other peripherals
- On-device interaction + cloud brain for LLM, aggregation, and search

## Module Map

```
main/agent/
├── agent_skeleton.h/.cc          # Init entry; wires modules when enabled
├── hal/                          # Hardware abstraction
│   ├── audio_io.*                # Speaker / PCM output HAL
│   ├── microphone.*              # Mic capture HAL
│   └── camera_hal.*              # Optional camera HAL
├── wake/                         # Wake word + VAD stubs
├── pipeline/                     # Streaming audio → cloud pipeline
├── cloud/                        # WebSocket / HTTP for ASR / LLM / TTS
├── voice/                        # Custom voice config hook
└── peripherals/                  # Extension registry for new peripherals
    └── example_peripheral.h      # Copy-paste pattern for a new peripheral
```

## Data Flow (skeleton)

```
Mic (HAL) → Wake/VAD → Audio Stream Pipeline → Cloud Client (WS/HTTP)
                                                      │
                                              ASR / LLM / TTS
                                                      │
Speaker (HAL) ← Playback ←────────────────────────────┘
```

Optional camera frames and other peripherals attach via the peripheral registry and can be forwarded through the cloud client for multimodal / aggregation use cases.

## Relation to Existing Code

| Skeleton | Existing counterpart |
|----------|----------------------|
| `hal/audio_io`, `hal/microphone` | `audio/audio_codec.h` |
| `hal/camera_hal` | `boards/common/camera.h` |
| `wake/wake_vad` | `audio/wake_word.h`, `audio/audio_processor.h` (VAD) |
| `pipeline/audio_stream_pipeline` | `audio/audio_service.h` |
| `cloud/cloud_client` | `protocols/protocol.h` (+ websocket / mqtt) |
| `voice/custom_voice_config` | assets / server-side TTS voice settings |
| `peripherals/` | board MCP tools, `Board` accessors |

Stubs compile independently. Replace stub bodies with adapters to the production classes when integrating.

## Enable / Build

```bash
idf.py set-target esp32s3
idf.py menuconfig   # Xiaozhi Assistant → Agent Skeleton → Enable
idf.py build
```
