#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>

namespace eighty
{
// Captures the final stereo output to a WAV file.
//
// Writing to disk cannot happen on the audio thread, so this is JUCE's
// ThreadedWriter arrangement: the audio thread hands each block to a
// lock-free FIFO and a background thread drains it to the file. The audio
// thread only ever takes the writer lock with a *try*-lock it is allowed to
// lose - the worst a lost block can do is leave a gap in the recording, and
// never a dropout in what you are actually hearing.
class AudioRecorder
{
public:
    AudioRecorder() : thread ("Eighty recorder") { thread.startThread(); }

    ~AudioRecorder()
    {
        stop();
        thread.stopThread (2000);
    }

    void prepare (double sampleRate)
    {
        stop();                       // a rate change invalidates the header
        sr = sampleRate;
    }

    // Message thread. Returns false and fills `error` if the file could not
    // be opened - a recording that silently did not happen is worse than one
    // that says so.
    bool start (const juce::File& file, juce::String& error)
    {
        stop();

        if (! file.getParentDirectory().createDirectory())
        {
            error = "Couldn't create " + file.getParentDirectory().getFullPathName();
            return false;
        }
        file.deleteFile();

        auto stream = std::make_unique<juce::FileOutputStream> (file);
        if (! stream->openedOk())
        {
            error = "Couldn't open " + file.getFullPathName() + " for writing";
            return false;
        }

        juce::WavAudioFormat wav;
        // 24-bit: the limiter is always in circuit so nothing arrives above
        // full scale, and 24-bit is what anything downstream will expect.
        std::unique_ptr<juce::AudioFormatWriter> w (
            wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
        if (w == nullptr)
        {
            error = "Couldn't write a WAV header at " + juce::String (sr, 0) + " Hz";
            return false;
        }
        stream.release();             // the writer owns it now

        samplesWritten.store (0);
        dropped.store (0);
        {
            const juce::ScopedLock sl (writerLock);
            // ~0.7 s of slack at 48 kHz between the audio thread and the
            // disk, which is a long stall to ride out in real time.
            writer = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (
                w.release(), thread, 32768);
        }
        {
            const juce::ScopedLock sl (fileLock);
            current = last = file;
        }
        recording.store (true);
        return true;
    }

    // Message thread. Safe to call when not recording. Destroying the
    // ThreadedWriter is what flushes the queue and finalises the header, so
    // the file is complete by the time this returns.
    void stop()
    {
        if (! recording.exchange (false) && writer == nullptr) return;
        const juce::ScopedLock sl (writerLock);
        writer.reset();
    }

    bool isRecording() const { return recording.load(); }

    double elapsedSeconds() const
    { return sr > 0.0 ? (double) samplesWritten.load() / sr : 0.0; }

    // Non-zero means the file has a gap in it.
    int64_t droppedSamples() const { return dropped.load(); }

    juce::File lastFile() const
    {
        const juce::ScopedLock sl (fileLock);
        return last;
    }

    // Audio thread.
    void write (const float* l, const float* r, int numSamples)
    {
        if (! recording.load() || numSamples <= 0) return;
        const juce::ScopedTryLock stl (writerLock);
        if (! stl.isLocked() || writer == nullptr) return;

        const float* channels[2] = { l, r };
        // write() refuses when its FIFO is full - the disk fell far enough
        // behind that the queue backed up. Counting that is the whole point:
        // a recording with a silent hole in it and nothing said about it is
        // worse than one that admits the hole.
        if (writer->write (channels, numSamples))
            samplesWritten.fetch_add (numSamples);
        else
            dropped.fetch_add (numSamples);
    }

private:
    juce::TimeSliceThread thread;
    juce::CriticalSection writerLock;
    mutable juce::CriticalSection fileLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;

    std::atomic<bool> recording { false };
    std::atomic<int64_t> samplesWritten { 0 };
    std::atomic<int64_t> dropped { 0 };
    double sr = 44100.0;
    juce::File current, last;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecorder)
};
} // namespace eighty
