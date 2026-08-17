#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "FactoryPresets.h"
#include "DSP/SynthEngine.h"
#include "DSP/Effects.h"
#include "Recorder.h"

// Lock-free scope feed: audio thread pushes, UI thread pulls. Stereo, so
// the header can drive both the waveform trace and the lissajous/vector
// display off one queue.
class ScopeFifo
{
public:
    static constexpr int kSize = 16384;   // frames

    void push (const float* l, const float* r, int n)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (n, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) { left[(size_t) (s1 + i)] = l[i];      right_[(size_t) (s1 + i)] = r[i]; }
        for (int i = 0; i < n2; ++i) { left[(size_t) (s2 + i)] = l[n1 + i]; right_[(size_t) (s2 + i)] = r[n1 + i]; }
        fifo.finishedWrite (n1 + n2);
    }

    // Returns the number of frames written into destL / destR.
    int pull (float* destL, float* destR, int maxFrames)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (maxFrames, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) { destL[i] = left[(size_t) (s1 + i)];      destR[i] = right_[(size_t) (s1 + i)]; }
        for (int i = 0; i < n2; ++i) { destL[n1 + i] = left[(size_t) (s2 + i)]; destR[n1 + i] = right_[(size_t) (s2 + i)]; }
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }

private:
    juce::AbstractFifo fifo { kSize };
    std::array<float, kSize> left {}, right_ {};
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

// Trackpad gestures reaching the audio thread.
//
// Touches arrive on the message thread at screen rates, and none of them may
// touch the engine there. They go through this instead, drained at the top of
// processBlock - the same one-block granularity the UI keyboard already has,
// and nothing is shared but the FIFO.
struct GestureMsg
{
    enum Type : uint8_t { kArm, kNote, kLaneOff, kPatternStep, kRibbon, kPressure, kBright };

    Type    type = kArm;
    int8_t  lane = 0;
    int8_t  model = -1;      // 0 = CS-80, 1 = JP-8, -1 = follow the engine mode
    bool    flag = false;    // arm: pattern target. laneOff: damp.
    int     note = 0;
    float   value = 0.f;     // velocity / semitones / 0..1
    int     dir = 1;
};

class GestureQueue
{
public:
    static constexpr int kSize = 512;

    void push (const GestureMsg& m)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 > 0)      buf[(size_t) s1] = m;
        else if (n2 > 0) buf[(size_t) s2] = m;
        fifo.finishedWrite (n1 + n2);
    }

    template <typename Fn>
    void drain (Fn&& apply)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (kSize, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) apply (buf[(size_t) (s1 + i)]);
        for (int i = 0; i < n2; ++i) apply (buf[(size_t) (s2 + i)]);
        fifo.finishedRead (n1 + n2);
    }

private:
    juce::AbstractFifo fifo { kSize };
    std::array<GestureMsg, kSize> buf {};
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

    // Trackpad strum / ribbon, pushed by the editor, applied in processBlock
    GestureQueue gestures;
    std::atomic<int> activeVoices { 0 };
    std::atomic<int> lastLearnedCC { -1 };

    // Effective tempo, published for the sequencer's readout: the panel
    // TEMPO unless SYNC is on *and* the host actually reports one.
    std::atomic<float> uiBpm { 120.f };
    std::atomic<bool> uiBpmFromHost { false };

    // Notes currently held on any input, velocity 1-127 (0 = up). The
    // sequencer grid reads this so clicking a step can drop in whatever
    // you are holding down.
    std::array<std::atomic<uint8_t>, 128> heldNoteVel {};

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

    // ---- hosted VST3 synth layer (plays the engine's note stream) ----
    bool setSynthLayer (const juce::PluginDescription&, juce::String& error);
    void clearSynthLayer();
    juce::String getSynthLayerName() const;
    juce::AudioPluginInstance* getSynthLayer() const;

    // ---- presets ----
    // A preset is the synth itself: every parameter, the per-key engine map
    // and the sequencer pattern. Deliberately *not* the MIDI-learn map (that
    // belongs to your hardware, not the sound) or the hosted VST3 chain
    // (reloading plugins on every preset switch would stall the UI).
    // Factory starting points (FactoryPresets.h): built in, so they are always
    // there and cannot be overwritten or deleted. They live alongside the
    // user's saved files in the preset bar rather than in a separate browser.
    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int index);
    void loadFactoryPreset (int index);

    // ---- audio recording ----
    // Captures the final stereo output - post FX, post inserts, post master
    // volume and limiter, the same point the scopes read - to a stereo WAV
    // beside the presets. Deliberately not a parameter: it writes files, and
    // nothing that writes files should be reachable from host automation or
    // be restored by loading a patch.
    static juce::File recordingsFolder();
    bool startRecording (juce::String& error);
    void stopRecording() { recorder.stop(); }
    bool isRecording() const { return recorder.isRecording(); }
    double recordedSeconds() const { return recorder.elapsedSeconds(); }
    // Non-zero means the disk fell far enough behind to leave a gap.
    juce::int64 recordingGapSamples() const { return recorder.droppedSamples(); }
    juce::File lastRecording() const { return recorder.lastFile(); }

    static juce::File presetFolder();
    juce::Array<juce::File> presetFiles() const;
    bool savePreset (const juce::String& name, juce::String& error);
    bool loadPreset (const juce::File&, juce::String& error);
    juce::String currentPresetName() const { return presetName; }
    void markPresetEdited() { presetDirty = true; }
    bool isPresetEdited() const { return presetDirty; }

    // ---- per-key engine map (engine mode "Keys": 0 = CS, 1 = JP, 2 = both) ----
    uint8_t getKeyZone (int note) const
    { return keyZones[(size_t) juce::jlimit (0, 127, note)].load (std::memory_order_relaxed); }
    void setKeyZone (int note, uint8_t zone)
    { keyZones[(size_t) juce::jlimit (0, 127, note)].store ((uint8_t) juce::jmin (zone, (uint8_t) 2),
                                                            std::memory_order_relaxed); }
    void fillKeyZones (std::function<uint8_t (int)> zoneFor)
    { for (int i = 0; i < 128; ++i) keyZones[(size_t) i].store (zoneFor (i), std::memory_order_relaxed); }

private:
    void updateParameters();
    void handleMidiEvent (const juce::MidiMessage& m);
    void applyGesture (const GestureMsg&);

    juce::ValueTree soundToValueTree();         // params + key map + pattern
    void soundFromValueTree (const juce::ValueTree&);
    juce::String presetName { "Init" };
    bool presetDirty = false;

    juce::File pluginCacheFile() const;
    juce::File deadMansPedalFile() const;
    void saveKnownPlugins();
    juce::ValueTree chainToValueTree() const;
    void restoreChainFromValueTree (juce::ValueTree chain, juce::ValueTree synth);
    void updateChainLatency();

    juce::AudioPluginFormatManager formatManager;
    juce::OwnedArray<juce::AudioPluginInstance> insertChain;  // guarded by chainLock
    std::unique_ptr<juce::AudioPluginInstance> synthLayer;    // guarded by chainLock
    mutable juce::CriticalSection chainLock;
    juce::AudioBuffer<float> synthBuf;
    juce::MidiBuffer midiEcho;
    double curSampleRate = 44100.0;
    int curBlockSize = 512;

    // cached raw-value pointers
    std::map<juce::String, std::atomic<float>*> raw;
    float rawVal (const char* id) const { return raw.at (id)->load(); }

    eighty::AudioRecorder recorder;

    eighty::Chorus chorus;
    eighty::StereoDelay delay;
    eighty::Tremolo tremolo;
    eighty::Limiter limiter;

    std::array<std::atomic<uint8_t>, 128> keyZones;

    juce::AudioBuffer<float> scratch;
    bool lastArpOn[2] = { false, false };
    bool lastHold = false;
    bool lastSeqRec = false;
    bool lastSeqPlay = false;
    double hostBpm = 120.0;
    bool hostBpmValid = false;   // did the host actually give us a tempo?
    double curBpm = 120.0;       // effective tempo, resolved per block

    // Hosted synth layer gain (mixer level x mute/solo), ramped across the
    // block so hitting MUTE is a fade rather than a click.
    float synthGain = 0.8f, lastSynthGain = 0.8f;

    // Trackpad cross-axis brilliance: an offset over the panel macro, held
    // between blocks so a finger parked halfway up the pad stays there.
    float gestureBright = 0.f;

    JUCE_DECLARE_WEAK_REFERENCEABLE (EightyProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightyProcessor)
};
