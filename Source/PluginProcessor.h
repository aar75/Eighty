#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "DSP/SynthEngine.h"
#include "DSP/Effects.h"

// Lock-free scope feed: audio thread pushes, UI thread pulls.
class ScopeFifo
{
public:
    static constexpr int kSize = 16384;

    void push (const float* data, int n)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) buffer[(size_t) (s1 + i)] = data[i];
        for (int i = 0; i < n2; ++i) buffer[(size_t) (s2 + i)] = data[n1 + i];
        fifo.finishedWrite (n1 + n2);
    }

    int pull (float* dest, int maxN)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (maxN, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) dest[i] = buffer[(size_t) (s1 + i)];
        for (int i = 0; i < n2; ++i) dest[n1 + i] = buffer[(size_t) (s2 + i)];
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }

private:
    juce::AbstractFifo fifo { kSize };
    std::array<float, kSize> buffer {};
};

// CC -> parameter mapping with a "learn" handshake from the UI.
class MidiLearnManager
{
public:
    void startLearn (const juce::String& paramID)
    {
        pendingParam = paramID;
        learning.store (true);
    }
    void cancelLearn() { learning.store (false); }
    bool isLearning() const { return learning.load(); }

    // audio thread: returns true if the CC was consumed as a learn assignment
    bool handleCC (int cc)
    {
        if (! learning.load()) return false;
        const juce::ScopedLock sl (lock);
        // remove any previous binding of this cc or this param
        for (auto it = map.begin(); it != map.end();)
            it = (it->second == pendingParam || it->first == cc) ? map.erase (it) : std::next (it);
        map[cc] = pendingParam;
        learning.store (false);
        return true;
    }

    juce::String paramForCC (int cc) const
    {
        const juce::ScopedLock sl (lock);
        auto it = map.find (cc);
        return it != map.end() ? it->second : juce::String();
    }

    void clearParam (const juce::String& paramID)
    {
        const juce::ScopedLock sl (lock);
        for (auto it = map.begin(); it != map.end();)
            it = (it->second == paramID) ? map.erase (it) : std::next (it);
    }

    int ccForParam (const juce::String& paramID) const
    {
        const juce::ScopedLock sl (lock);
        for (auto& [cc, pid] : map)
            if (pid == paramID) return cc;
        return -1;
    }

    juce::ValueTree toValueTree() const
    {
        const juce::ScopedLock sl (lock);
        juce::ValueTree t ("midiMap");
        for (auto& [cc, pid] : map)
        {
            juce::ValueTree e ("map");
            e.setProperty ("cc", cc, nullptr);
            e.setProperty ("param", pid, nullptr);
            t.appendChild (e, nullptr);
        }
        return t;
    }

    void fromValueTree (const juce::ValueTree& t)
    {
        const juce::ScopedLock sl (lock);
        map.clear();
        for (auto e : t)
            if (e.hasType ("map"))
                map[(int) e.getProperty ("cc")] = e.getProperty ("param").toString();
    }

private:
    mutable juce::CriticalSection lock;
    std::map<int, juce::String> map;
    juce::String pendingParam;
    std::atomic<bool> learning { false };
};

class EightyProcessor : public juce::AudioProcessor
{
public:
    EightyProcessor();
    ~EightyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Eighty"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- shared with the editor ----
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // UI keyboard -> midi
    ScopeFifo scopeFifo;
    MidiLearnManager midiLearn;
    eighty::SynthEngine engine;
    std::atomic<float> uiBend { 0.f };       // set by UI pitch wheel / arrow keys
    std::atomic<bool> uiBendActive { false };
    std::atomic<int> activeVoices { 0 };
    std::atomic<int> lastLearnedCC { -1 };

    // ---- external VST3 insert chain (post-FX, pre master volume) ----
    juce::KnownPluginList knownPlugins;
    std::atomic<int> chainVersion { 0 };     // bumped on every chain edit

    void rescanPlugins();                    // message thread; blocking scan
    bool addInsert (const juce::PluginDescription&, juce::String& error);
    void removeInsert (int index);
    int getNumInserts() const;
    juce::String getInsertName (int index) const;
    juce::AudioPluginInstance* getInsert (int index) const;
    static constexpr int kMaxInserts = 4;

private:
    void updateParameters();
    void handleMidiEvent (const juce::MidiMessage& m);

    juce::File pluginCacheFile() const;
    juce::ValueTree chainToValueTree() const;
    void restoreChainFromValueTree (juce::ValueTree state);   // message thread
    void updateChainLatency();

    juce::AudioPluginFormatManager formatManager;
    juce::OwnedArray<juce::AudioPluginInstance> insertChain;  // guarded by chainLock
    mutable juce::CriticalSection chainLock;
    double curSampleRate = 44100.0;
    int curBlockSize = 512;

    // cached raw-value pointers
    std::map<juce::String, std::atomic<float>*> raw;
    float rawVal (const char* id) const { return raw.at (id)->load(); }

    eighty::Chorus chorus;
    eighty::StereoDelay delay;
    eighty::Tremolo tremolo;

    juce::AudioBuffer<float> scratch;
    bool lastArpOn = false;
    bool lastHold = false;
    double hostBpm = 120.0;

    JUCE_DECLARE_WEAK_REFERENCEABLE (EightyProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightyProcessor)
};
