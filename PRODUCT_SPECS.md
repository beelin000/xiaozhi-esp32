# Product Specs — ESP32-S3 Smart Hardware AI Agent

Product foundation for an ESP32-S3–based personal assistant: on-device voice interaction with a cloud (or optional local companion) brain for LLM reasoning, data aggregation, and search.

This document is the product source of truth for subsequent development. Architecture details live in [`DESIGN.md`](DESIGN.md). The additive code skeleton under `main/agent/` (see [PR #1](https://github.com/beelin000/xiaozhi-esp32/pull/1) while open) maps modules to these capabilities.

## Vision

A compact smart-hardware AI agent that feels like a personal assistant: wake it, speak naturally, get a spoken reply with low latency, and optionally extend it with camera, extra mics, and other peripherals. Interaction stays on the device; heavy language and search work runs in the cloud by default, with a local streaming path as a first-class target.

## Primary Platform

| Item | Spec |
|------|------|
| MCU / SoC | ESP32-S3 (ESP-IDF) |
| Role | Capture, wake/VAD, streaming I/O, playback, peripheral hub |
| Brain (default) | Cloud ASR → LLM → TTS |
| Brain (optional) | Local companion (Jetson Orin Nano 8GB) via the same device interfaces |

Heavy models (Whisper-class ASR, ~3B LLM, neural TTS) are **not** expected to run on the S3. They run in the cloud or on the Jetson companion.

## Core Capabilities

### 1. Real-time voice interaction

- Offline or on-device wake word + VAD to start a turn
- Continuous mic capture and streaming uplink while the user speaks
- Spoken response via streaming TTS playback on the device speaker
- Target UX: conversational, interruptible where the pipeline allows

### 2. Custom voice

- Configurable TTS / voice profile (voice id, style, language hooks)
- Device-side config hook that the cloud or local TTS backend honors
- Compatible with existing asset / server voice customization patterns in this repo

### 3. Cloud LLM personal assistant

- Cloud integration for ASR, LLM, and TTS
- Personal-assistant behaviors: Q&A, summarization, tool-backed actions
- Cloud brain for **LLM data aggregation and search** (knowledge / tool results fused into replies)
- Transport: WebSocket preferred for streaming; HTTP for request/response helpers

### 4. Expandable peripherals

- Microphone and speaker as baseline
- Optional camera for multimodal / visual context
- Extension API / registry so new sensors and actuators can plug in without rewriting the agent core
- Board-level MCP / device control remains available for IoT-style actions

### 5. On-device interaction + cloud brain

| Layer | Responsibility |
|-------|----------------|
| On device (S3) | Audio I/O, wake/VAD, stream pipeline, peripheral attach, playback |
| Cloud (default) | ASR, LLM, TTS, aggregation, search |
| Local companion (target) | Same ASR/LLM/TTS roles off-device on LAN, swappable behind one interface |

## Local Streaming Option (Product Target)

Cloud remains the default backend. A **local streaming** deployment is an explicit product capability:

| Item | Target |
|------|--------|
| Companion hardware | NVIDIA Jetson Orin Nano 8GB |
| ASR | Whisper **small** (or equivalent streaming ASR) |
| LLM | ~**3B** Qwen-class model |
| TTS | Piper (or equivalent low-latency neural TTS) |
| End-to-end latency | Sub-second to ~**1 s** (wake/utterance end → first audible speech), best-effort under local LAN |
| Device contract | Same uplink/downlink and service façade as cloud; backend swap only |

The S3 still owns capture, wake/VAD, and playback. The Jetson runs the heavy stack and streams results back.

## Latency & Pipeline Expectations

Voice turns use three **overlapping** stages (not strictly serial):

1. **Chunked ASR** while the user is still speaking  
2. **Streaming LLM tokens** as partial transcript / prompt context becomes available  
3. **Streaming TTS playback** as soon as speakable text is ready  

Product goal: minimize time-to-first-audio; avoid waiting for full utterance → full completion → full audio before playback starts.

## Non-Goals (for this foundation)

- Running Whisper / ~3B LLM / Piper **on** the ESP32-S3
- Replacing the existing production `audio/` + `protocols/` path in one step (skeleton adapters first)
- Defining a full cloud billing / multi-tenant product surface in this doc

## Success Criteria

- Users can wake, speak, and hear a streamed reply with cloud backends
- Custom voice can be selected via the documented config hook
- Camera / extra peripherals can be registered without core rewrites
- Switching to Jetson local backends does not require a different device API
- Docs + `main/agent/` skeleton stay aligned as implementation fills in stubs

## Related Docs

- [`DESIGN.md`](DESIGN.md) — architecture, module map, backend swap, streaming stages  
- `main/agent/README.md` — skeleton layout (when present / after PR #1 merges)  
- Existing protocol docs under `docs/` (WebSocket, MQTT+UDP, MCP) — transport and IoT control detail  
