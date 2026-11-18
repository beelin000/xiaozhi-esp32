#ifndef AGENT_BRAIN_STAGE_INTERFACES_H
#define AGENT_BRAIN_STAGE_INTERFACES_H

#include <functional>
#include <memory>
#include <string>

#include "audio_stream_pipeline.h"

/**
 * Conceptual stage interfaces for the overlapping streaming pipeline.
 * Backends may implement these internally or map them to a single multiplexed
 * companion/cloud protocol. Kept as documentation-grade contracts for stubs.
 *
 * Overlap model (target ~sub-second–1s e2e on Jetson Orin Nano 8GB):
 *
 *   time →
 *   ASR:  [==== chunked while user speaks ====]
 *   LLM:            [==== tokens as ASR finals / prompt ====]
 *   TTS:                  [==== audio as tokens arrive ====]
 *   Play:                       [==== speaker ====]
 */

class AsrStage {
public:
    using PartialCallback = std::function<void(const std::string& text, bool is_final)>;
    virtual ~AsrStage() = default;
    virtual bool PushAudio(std::unique_ptr<AudioChunk> chunk) = 0;
    virtual bool EndOfSpeech() = 0;
    virtual void OnPartial(PartialCallback callback) = 0;
};

class LlmStage {
public:
    using TokenCallback = std::function<void(const std::string& token, bool is_final)>;
    virtual ~LlmStage() = default;
    /** Feed recognized text (from ASR final, or barge-in prompt). */
    virtual bool PushPrompt(const std::string& text) = 0;
    virtual void OnToken(TokenCallback callback) = 0;
};

class TtsStage {
public:
    using AudioCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;
    virtual ~TtsStage() = default;
    /** Feed LLM tokens incrementally for chunked synthesis. */
    virtual bool PushToken(const std::string& token, bool is_final) = 0;
    virtual void OnAudio(AudioCallback callback) = 0;
};

#endif  // AGENT_BRAIN_STAGE_INTERFACES_H
