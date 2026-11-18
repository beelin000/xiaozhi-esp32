# Agent Skeleton Architecture

Additive ESP-IDF skeleton for an ESP32-S3 smart-hardware AI agent. Interfaces and stubs live under `main/agent/`; production audio/protocol code remains in `main/audio/` and `main/protocols/`.

Enable with `CONFIG_AGENT_SKELETON_ENABLE` (menuconfig → Xiaozhi Assistant → Agent Skeleton).

## Goals

- Real-time voice interaction on device
- Custom voice configuration hook
- Swappable **cloud** (default) or **local companion** brain for ASR → LLM → TTS
- Expandable camera / mic / other peripherals
- On-device interaction + brain for LLM, aggregation, and search

## Module Map

```
main/agent/
├── agent_skeleton.h/.cc          # Init entry; wires modules when enabled
├── hal/                          # Hardware abstraction (stays on S3)
│   ├── audio_io.*                # Speaker / PCM output HAL
│   ├── microphone.*              # Mic capture HAL
│   └── camera_hal.*              # Optional camera HAL
├── wake/                         # Wake word + VAD stubs
├── pipeline/                     # Device audio stream (uplink/downlink chunks)
├── brain/                        # Swappable brain + overlapping session
│   ├── brain_backend.h           # ONE interface (cloud | local)
│   ├── stage_interfaces.h        # ASR / LLM / TTS stage contracts
│   ├── streaming_voice_session.* # Orchestrates overlap + playback
│   ├── cloud_brain_backend.cc    # Default: wraps CloudClient
│   ├── local_brain_backend.cc    # Off-device Jetson companion adapter
│   └── brain_backend_factory.cc  # Selects backend from Kconfig
├── cloud/                        # WebSocket / HTTP transports + CloudClient
├── local/                        # Local companion client (Jetson, not on S3)
├── voice/                        # Custom voice config hook
└── peripherals/                  # Extension registry for new peripherals
```

## Swappable Brain (cloud default)

Device HAL, wake/VAD, and `AudioStreamPipeline` always run on the ESP32-S3.
The **brain** (ASR / LLM / TTS) is selected behind `BrainBackend`:

| Backend | Kconfig | Where models run |
|---------|---------|------------------|
| **Cloud** (default) | `AGENT_SKELETON_BRAIN_CLOUD` | Remote ASR/LLM/TTS service |
| **Local** | `AGENT_SKELETON_BRAIN_LOCAL` | Companion board (e.g. Jetson Orin Nano 8GB) over LAN |

Local models are **not** intended to run on the S3. Suggested companion stack for ~sub-second to ~1s e2e:

- ASR: Whisper small (streaming)
- LLM: ~3B Qwen (token streaming)
- TTS: Piper (chunked)

## Overlapping Streaming Pipeline

```
Mic (HAL) → Wake/VAD → Audio Stream Pipeline
                              │
                              ▼
                    BrainBackend (cloud | local)
                 ┌────────────┼────────────┐
                 │            │            │
              ASR stage   LLM stage    TTS stage
           (while speak) (tokens)   (as tokens arrive)
                 │            │            │
                 └────────────┴─────► Speaker (HAL)
```

Stages intentionally **overlap**: audio chunks stream to ASR while the user speaks; recognized text feeds the LLM for streaming tokens; tokens feed TTS immediately so playback starts before the full reply is ready.

## Relation to Existing Code

| Skeleton | Existing counterpart |
|----------|----------------------|
| `hal/audio_io`, `hal/microphone` | `audio/audio_codec.h` |
| `hal/camera_hal` | `boards/common/camera.h` |
| `wake/wake_vad` | `audio/wake_word.h`, `audio/audio_processor.h` (VAD) |
| `pipeline/audio_stream_pipeline` | `audio/audio_service.h` |
| `brain/brain_backend` | unified façade over cloud / local |
| `cloud/cloud_client` | `protocols/protocol.h` (+ websocket / mqtt) |
| `local/local_companion` | LAN companion process (not in-tree firmware) |
| `voice/custom_voice_config` | assets / server-side TTS voice settings |
| `peripherals/` | board MCP tools, `Board` accessors |

Stubs compile independently. Replace stub bodies with adapters to the production classes when integrating.

## Enable / Build

```bash
idf.py set-target esp32s3
idf.py menuconfig   # Xiaozhi Assistant → Agent Skeleton → Enable
                    # Brain Backend: Cloud (default) or Local companion
idf.py build
```
