# Design — ESP32-S3 AI Agent Architecture

Architecture and design notes for the product described in [`PRODUCT_SPECS.md`](PRODUCT_SPECS.md). Aligns with the additive skeleton under `main/agent/` ([PR #1](https://github.com/beelin000/xiaozhi-esp32/pull/1) while open). Production paths in `main/audio/` and `main/protocols/` stay intact; the skeleton is interfaces + stubs that can later adapt to those classes.

## Design Principles

1. **Device owns interaction; backends own intelligence** — S3 captures, gates, streams, and plays; ASR/LLM/TTS live off-device.
2. **One façade, swappable backends** — cloud default; Jetson local stack plugs in behind the same client/pipeline interfaces.
3. **Stream early, overlap stages** — chunked ASR, token streaming, and TTS playback run concurrently where possible.
4. **ESP-IDF first** — C++ modules, Kconfig enablement, Google style; no heavy models on-chip.
5. **Extend via HAL + peripheral registry** — new mics, cameras, and actuators without rewriting the agent core.

## High-Level Topology

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-S3 (ESP-IDF)                      │
│  HAL (mic / audio out / optional camera)                    │
│       ↓                                                     │
│  Wake word + VAD                                            │
│       ↓                                                     │
│  Streaming audio pipeline (uplink PCM/Opus ↔ downlink)      │
│       ↓                              ↑                      │
│  Cloud / Local client façade (ASR · LLM · TTS · search)     │
│  Custom voice config hook · Peripheral extension API        │
└───────────────────────────┬─────────────────────────────────┘
                            │ WebSocket / HTTP (LAN or WAN)
            ┌───────────────┴───────────────┐
            ▼                               ▼
   Cloud brain (default)          Jetson Orin Nano 8GB (target)
   ASR / LLM / TTS                Whisper small / ~3B Qwen / Piper
   aggregation + search           same streaming contract
```

**Constraint:** Whisper-class ASR, ~3B LLM, and Piper-class TTS run on the **Jetson companion or cloud**, never on the S3.

## Module Map (`main/agent/`)

Matches the skeleton layout:

| Path | Responsibility |
|------|-----------------|
| `hal/` | Hardware abstraction: `audio_io`, `microphone`, optional `camera_hal` |
| `wake/` | Wake word + VAD stubs (gate turns into the pipeline) |
| `pipeline/` | Streaming uplink/downlink (`AudioStreamPipeline`, `AudioChunk`) |
| `cloud/` | Transport (WebSocket/HTTP) + `CloudClient` façade for ASR/LLM/TTS/search |
| `voice/` | Custom voice / TTS profile hook |
| `peripherals/` | Extension registry + example peripheral pattern |
| `agent_skeleton.*` | Optional composition root / menuconfig-gated auto-start |

Enable via **menuconfig → Xiaozhi Assistant → Agent Skeleton** (`CONFIG_AGENT_SKELETON_ENABLE`). Default off so existing builds are unchanged.

### Relation to existing production code

| Skeleton | Existing counterpart |
|----------|----------------------|
| `hal/audio_io`, `hal/microphone` | `audio/audio_codec.h` |
| `hal/camera_hal` | `boards/common/camera.h` |
| `wake/` | `audio/wake_word.h`, `audio/audio_processor.h` |
| `pipeline/` | `audio/audio_service.h` |
| `cloud/` | `protocols/protocol.h` (+ WebSocket / MQTT+UDP) |
| `voice/` | Assets / server-side TTS voice settings |
| `peripherals/` | Board MCP tools, `Board` accessors |

Integration path: keep stubs compiling, then replace bodies with thin adapters to production classes.

## HAL

- **Microphone** — capture PCM at pipeline sample rate (skeleton default 16 kHz, ~60 ms frames).
- **Audio I/O** — speaker / PCM (or decoded Opus) playback for TTS downlink.
- **Optional camera** — frame source for multimodal or aggregation use cases; not required for voice-only.
- Board-specific pinmux and codecs stay behind these HAL interfaces so the agent core stays portable across supported S3 boards.

## Wake Word & VAD

- Wake word starts a listening turn (reuse ESP-SR / existing wake path when adapting).
- VAD marks speech vs silence for chunk boundaries and end-of-utterance.
- After wake, mic frames feed the streaming pipeline; silence / barge-in policy is pipeline-owned once backends support it.

## Streaming Audio Pipeline

`AudioStreamPipeline` responsibilities:

- Accept capture PCM (`PushCapturePcm`) → optional encode → uplink callbacks
- Accept downlink TTS/audio chunks (`PushDownlink`) → playback via audio HAL
- Carry `AudioChunk` metadata: sample rate, frame duration, timestamp, payload (PCM or Opus)

Uplink consumers are the cloud/local client; downlink producers are TTS backends.

## Cloud Communication

`CloudClient` + transport layer:

- **WebSocket** — preferred for bidirectional streaming (audio up, ASR partials, LLM tokens, TTS audio)
- **HTTP** — helpers for non-streaming or one-shot search / config / auth
- Service kinds: ASR, LLM, TTS, search, aggregate
- Callbacks: `OnAsrResult`, `OnLlmToken`, `OnTtsAudio`, `OnError`

Config includes WebSocket URL, HTTP base URL, auth token, session id, and `prefer_websocket`.

Existing protocol docs under `docs/websocket.md` and `docs/mqtt-udp.md` remain the reference for wire formats when adapting to production transports.

## Custom Voice Hook

`voice/` holds a TTS profile / voice-id configuration surface. `RequestTts(text, voice_id)` (and local equivalents) must honor the selected profile. No model weights on device—only identifiers and parameters passed to the backend.

## Peripheral Extension API

`peripherals/` registry pattern:

- Register named peripherals (camera, extra mic, GPIO actuators, etc.)
- Lifecycle: init / deinit / optional event or frame callbacks
- Cloud/local client may forward peripheral payloads for multimodal LLM or aggregation
- Example peripheral header documents the copy-paste integration pattern

## Swappable Local vs Cloud Backends

Single device-facing interface (`CloudClient`-shaped façade + pipeline):

| Mode | Where models run | Device behavior |
|------|------------------|-----------------|
| **Cloud (default)** | Remote ASR/LLM/TTS + search/aggregation | Unchanged streaming contract |
| **Local companion** | Jetson Orin Nano 8GB: Whisper small, ~3B Qwen, Piper | Same URLs/session swapped to LAN endpoint; same callbacks |

Selection is configuration (URL / backend kind), not a fork of the agent pipeline. Heavy inference never moves onto the S3.

## Overlapping Turn Pipeline (Three Stages)

A single user turn is three concurrent stages:

```
User speaking ────────────────────────────────────────────►
│ chunked ASR partials ───────────────────► final transcript
│         │ streaming LLM tokens ─────────► completion
│         │         │ streaming TTS audio ► speaker playback
▼         ▼         ▼
time ─────────────────────────────────────────────────────►
     (target: first audio in sub-second … ~1 s locally)
```

1. **Chunked ASR while speaking** — uplink audio frames immediately; emit partial then final text.  
2. **Streaming LLM tokens** — start generation from partial/final ASR; stream tokens as they arrive.  
3. **Streaming TTS playback** — synthesize speakable spans as soon as available; pipeline plays downlink chunks without waiting for full audio.

Backends must support streaming; the device pipeline must not buffer an entire turn before starting the next stage.

## Composition & Build

- `agent_skeleton` wires HAL → wake/VAD → pipeline → client → voice/peripherals when enabled.
- Kconfig gates compile-in and optional auto-start so stock firmware behavior is preserved.
- Target: `idf.py set-target esp32s3` with ESP-IDF ≥ 5.4 (per project README).

## Implementation Guidance

1. Fill stub methods behind existing interfaces; prefer adapters over duplicate audio stacks.
2. Prove cloud streaming path first (default); add Jetson backend as a second transport endpoint.
3. Keep latency instrumentation at stage boundaries (ASR partial, first LLM token, first TTS chunk, first PCM out).
4. Document any wire-protocol divergence from `docs/websocket.md` when local companion protocols differ.

## Related

- [`PRODUCT_SPECS.md`](PRODUCT_SPECS.md) — product requirements and local streaming targets  
- `main/agent/README.md` — skeleton module notes (PR #1 / post-merge)  
- `docs/` — WebSocket, MQTT+UDP, MCP protocol details  
