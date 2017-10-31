
#include "juce_audio_graph.h"
#include <iostream>

using namespace juce2;

int main()
{
    String x = "haha";
    std::cout << x << std::endl;

    Atomic<float> a;
    CharPointer_UTF8 c("c");
    HeapBlock<String> hs;
    MemoryBlock m;
    Array<CharPointer_UTF8> ar;
    OwnedArray<String> ows;
    AudioSampleBuffer as;
    MidiBuffer mb;
    MidiMessage ms;
    AudioProcessorGraph g;

    return 0;
}
