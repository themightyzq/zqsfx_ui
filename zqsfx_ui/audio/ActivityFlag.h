#pragma once
// ActivityFlag: audio thread writes the block peak, message thread reads it. Lives apart from
// the GUI headers so a processor header can include it without pulling in juce_gui_basics.
//
// Origin: split out of components/RestingDisplay.h.

#include <atomic>
#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zqsfx::ui
{
// A count of loud blocks, so any number of displays can read one flag without consuming it.
// Lock-free atomics only: safe to call from processBlock.
class ActivityFlag
{
public:
    static constexpr float silenceDb = -80.0f;

    void noteBlock (const juce::AudioBuffer<float>& buffer) noexcept
    {
        float blockPeak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            blockPeak = juce::jmax (blockPeak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
        notePeak (blockPeak);
    }

    void notePeak (float linearPeak) noexcept
    {
        if (linearPeak >= juce::Decibels::decibelsToGain (silenceDb))
            loudBlocks.fetch_add (1, std::memory_order_relaxed);

        auto held = heldPeak.load (std::memory_order_relaxed);
        while (linearPeak > held
               && ! heldPeak.compare_exchange_weak (held, linearPeak, std::memory_order_relaxed))
        {}
    }

    // The loudest peak since the last call, then cleared. For exactly ONE reader (a meter): it
    // consumes, unlike loudBlockCount, so two readers would each see only part of the signal.
    // RestClock polls four times a second, so a meter that reads this after waking still shows
    // the real peak of a short hit.
    float takePeak() noexcept { return heldPeak.exchange (0.0f, std::memory_order_relaxed); }

    uint32_t loudBlockCount() const noexcept { return loudBlocks.load (std::memory_order_relaxed); }

private:
    static_assert (std::atomic<uint32_t>::is_always_lock_free, "ActivityFlag must be lock-free");
    static_assert (std::atomic<float>::is_always_lock_free, "ActivityFlag must be lock-free");
    std::atomic<uint32_t> loudBlocks { 0 };
    std::atomic<float> heldPeak { 0.0f };
};
} // namespace zqsfx::ui
