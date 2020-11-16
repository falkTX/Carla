/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
/** Structure for VST speaker mappings

    @tags{Audio}
*/
struct SpeakerMappings  : private AudioChannelSet // (inheritance only to give easier access to items in the namespace)
{
    /** Structure describing a mapping */
    struct Mapping
    {
        int32 vst2;
        ChannelType channels[13];

        bool matches (const Array<ChannelType>& chans) const noexcept
        {
            auto n = static_cast<int> (sizeof (channels) / sizeof (ChannelType));

            for (int i = 0; i < n; ++i)
            {
                if (channels[i] == unknown)  return (i == chans.size());
                if (i == chans.size())       return (channels[i] == unknown);

                if (channels[i] != chans.getUnchecked (i))
                    return false;
            }

            return true;
        }
    };

    static AudioChannelSet vstArrangementTypeToChannelSet (int32 arr, int fallbackNumChannels)
    {
        if      (arr == Vst2::vstSpeakerConfigTypeEmpty)          return AudioChannelSet::disabled();
        else if (arr == Vst2::vstSpeakerConfigTypeMono)           return AudioChannelSet::mono();
        else if (arr == Vst2::vstSpeakerConfigTypeLR)             return AudioChannelSet::stereo();
        else if (arr == Vst2::vstSpeakerConfigTypeLRC)            return AudioChannelSet::createLCR();
        else if (arr == Vst2::vstSpeakerConfigTypeLRS)            return AudioChannelSet::createLRS();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCS)           return AudioChannelSet::createLCRS();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLsRs)        return AudioChannelSet::create5point0();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLfeLsRs)     return AudioChannelSet::create5point1();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLsRsCs)      return AudioChannelSet::create6point0();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLfeLsRsCs)   return AudioChannelSet::create6point1();
        else if (arr == Vst2::vstSpeakerConfigTypeLRLsRsSlSr)     return AudioChannelSet::create6point0Music();
        else if (arr == Vst2::vstSpeakerConfigTypeLRLfeLsRsSlSr)  return AudioChannelSet::create6point1Music();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLsRsSlSr)    return AudioChannelSet::create7point0();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLsRsLcRc)    return AudioChannelSet::create7point0SDDS();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLfeLsRsSlSr) return AudioChannelSet::create7point1();
        else if (arr == Vst2::vstSpeakerConfigTypeLRCLfeLsRsLcRc) return AudioChannelSet::create7point1SDDS();
        else if (arr == Vst2::vstSpeakerConfigTypeLRLsRs)         return AudioChannelSet::quadraphonic();

        for (const Mapping* m = getMappings(); m->vst2 != Vst2::vstSpeakerConfigTypeEmpty; ++m)
        {
            if (m->vst2 == arr)
            {
                AudioChannelSet s;

                for (int i = 0; m->channels[i] != 0; ++i)
                    s.addChannel (m->channels[i]);

                return s;
            }
        }

        return AudioChannelSet::discreteChannels (fallbackNumChannels);
    }

    static AudioChannelSet vstArrangementTypeToChannelSet (const Vst2::VstSpeakerConfiguration& arr)
    {
        return vstArrangementTypeToChannelSet (arr.type, arr.numberOfChannels);
    }

    static int32 channelSetToVstArrangementType (AudioChannelSet channels)
    {
        if      (channels == AudioChannelSet::disabled())           return Vst2::vstSpeakerConfigTypeEmpty;
        else if (channels == AudioChannelSet::mono())               return Vst2::vstSpeakerConfigTypeMono;
        else if (channels == AudioChannelSet::stereo())             return Vst2::vstSpeakerConfigTypeLR;
        else if (channels == AudioChannelSet::createLCR())          return Vst2::vstSpeakerConfigTypeLRC;
        else if (channels == AudioChannelSet::createLRS())          return Vst2::vstSpeakerConfigTypeLRS;
        else if (channels == AudioChannelSet::createLCRS())         return Vst2::vstSpeakerConfigTypeLRCS;
        else if (channels == AudioChannelSet::create5point0())      return Vst2::vstSpeakerConfigTypeLRCLsRs;
        else if (channels == AudioChannelSet::create5point1())      return Vst2::vstSpeakerConfigTypeLRCLfeLsRs;
        else if (channels == AudioChannelSet::create6point0())      return Vst2::vstSpeakerConfigTypeLRCLsRsCs;
        else if (channels == AudioChannelSet::create6point1())      return Vst2::vstSpeakerConfigTypeLRCLfeLsRsCs;
        else if (channels == AudioChannelSet::create6point0Music()) return Vst2::vstSpeakerConfigTypeLRLsRsSlSr;
        else if (channels == AudioChannelSet::create6point1Music()) return Vst2::vstSpeakerConfigTypeLRLfeLsRsSlSr;
        else if (channels == AudioChannelSet::create7point0())      return Vst2::vstSpeakerConfigTypeLRCLsRsSlSr;
        else if (channels == AudioChannelSet::create7point0SDDS())  return Vst2::vstSpeakerConfigTypeLRCLsRsLcRc;
        else if (channels == AudioChannelSet::create7point1())      return Vst2::vstSpeakerConfigTypeLRCLfeLsRsSlSr;
        else if (channels == AudioChannelSet::create7point1SDDS())  return Vst2::vstSpeakerConfigTypeLRCLfeLsRsLcRc;
        else if (channels == AudioChannelSet::quadraphonic())       return Vst2::vstSpeakerConfigTypeLRLsRs;

        if (channels == AudioChannelSet::disabled())
            return Vst2::vstSpeakerConfigTypeEmpty;

        auto chans = channels.getChannelTypes();

        for (auto* m = getMappings(); m->vst2 != Vst2::vstSpeakerConfigTypeEmpty; ++m)
            if (m->matches (chans))
                return m->vst2;

        return Vst2::vstSpeakerConfigTypeUser;
    }

    static void channelSetToVstArrangement (const AudioChannelSet& channels, Vst2::VstSpeakerConfiguration& result)
    {
        result.type = channelSetToVstArrangementType (channels);
        result.numberOfChannels = channels.size();

        for (int i = 0; i < result.numberOfChannels; ++i)
        {
            auto& speaker = result.speakers[i];

            zeromem (&speaker, sizeof (Vst2::VstIndividualSpeakerInfo));
            speaker.type = getSpeakerType (channels.getTypeOfChannel (i));
        }
    }

    /** Class to hold a speaker configuration */
    class VstSpeakerConfigurationHolder
    {
    public:
        VstSpeakerConfigurationHolder()
        {
            clear();
        }

        VstSpeakerConfigurationHolder (const Vst2::VstSpeakerConfiguration& vstConfig)
        {
            operator= (vstConfig);
        }

        VstSpeakerConfigurationHolder (const VstSpeakerConfigurationHolder& other)
        {
            operator= (other.get());
        }

        VstSpeakerConfigurationHolder (VstSpeakerConfigurationHolder&& other)
            : storage (std::move (other.storage))
        {
            other.clear();
        }

        VstSpeakerConfigurationHolder (const AudioChannelSet& channels)
        {
            auto numberOfChannels = channels.size();
            Vst2::VstSpeakerConfiguration& dst = *allocate (numberOfChannels);

            dst.type = channelSetToVstArrangementType (channels);
            dst.numberOfChannels = numberOfChannels;

            for (int i = 0; i < dst.numberOfChannels; ++i)
            {
                Vst2::VstIndividualSpeakerInfo& speaker = dst.speakers[i];

                zeromem (&speaker, sizeof (Vst2::VstIndividualSpeakerInfo));
                speaker.type = getSpeakerType (channels.getTypeOfChannel (i));
            }
        }

        VstSpeakerConfigurationHolder& operator= (const VstSpeakerConfigurationHolder& vstConfig) { return operator=(vstConfig.get()); }
        VstSpeakerConfigurationHolder& operator= (const Vst2::VstSpeakerConfiguration& vstConfig)
        {
            Vst2::VstSpeakerConfiguration& dst = *allocate (vstConfig.numberOfChannels);

            dst.type             = vstConfig.type;
            dst.numberOfChannels = vstConfig.numberOfChannels;

            for (int i = 0; i < dst.numberOfChannels; ++i)
                dst.speakers[i] = vstConfig.speakers[i];

            return *this;
        }

        VstSpeakerConfigurationHolder& operator= (VstSpeakerConfigurationHolder && vstConfig)
        {
            storage = std::move (vstConfig.storage);
            vstConfig.clear();

            return *this;
        }

        const Vst2::VstSpeakerConfiguration& get() const { return *storage.get(); }

    private:
        JUCE_LEAK_DETECTOR (VstSpeakerConfigurationHolder)

        HeapBlock<Vst2::VstSpeakerConfiguration> storage;

        Vst2::VstSpeakerConfiguration* allocate (int numChannels)
        {
            auto arrangementSize = (size_t) (jmax (8, numChannels) - 8) * sizeof (Vst2::VstIndividualSpeakerInfo)
                                    + sizeof (Vst2::VstSpeakerConfiguration);

            storage.malloc (1, arrangementSize);
            return storage.get();
        }

        void clear()
        {
            Vst2::VstSpeakerConfiguration& dst = *allocate (0);

            dst.type = Vst2::vstSpeakerConfigTypeEmpty;
            dst.numberOfChannels = 0;
        }
    };

    static const Mapping* getMappings() noexcept
    {
        static const Mapping mappings[] =
        {
            { Vst2::vstSpeakerConfigTypeMono,                          { centre, unknown } },
            { Vst2::vstSpeakerConfigTypeLR,                            { left, right, unknown } },
            { Vst2::vstSpeakerConfigTypeLsRs,                          { leftSurround, rightSurround, unknown } },
            { Vst2::vstSpeakerConfigTypeLcRc,                          { leftCentre, rightCentre, unknown } },
            { Vst2::vstSpeakerConfigTypeSlSr,                          { leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeCLfe,                          { centre, LFE, unknown } },
            { Vst2::vstSpeakerConfigTypeLRC,                           { left, right, centre, unknown } },
            { Vst2::vstSpeakerConfigTypeLRS,                           { left, right, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfe,                        { left, right, centre, LFE, unknown } },
            { Vst2::vstSpeakerConfigTypeLRLfeS,                        { left, right, LFE, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCS,                          { left, right, centre, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRLsRs,                        { left, right, leftSurround, rightSurround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeS,                       { left, right, centre, LFE, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRLfeLsRs,                     { left, right, LFE, leftSurround, rightSurround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRs,                       { left, right, centre, leftSurround, rightSurround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRs,                    { left, right, centre, LFE, leftSurround, rightSurround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRsCs,                     { left, right, centre, leftSurround, rightSurround, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRLsRsSlSr,                    { left, right, leftSurround, rightSurround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsCs,                  { left, right, centre, LFE, leftSurround, rightSurround, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRLfeLsRsSlSr,                 { left, right, LFE, leftSurround, rightSurround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRsLcRc,                   { left, right, centre, leftSurround, rightSurround, topFrontLeft, topFrontRight, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRsSlSr,                   { left, right, centre, leftSurround, rightSurround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsLcRc,                { left, right, centre, LFE, leftSurround, rightSurround, topFrontLeft, topFrontRight, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsSlSr,                { left, right, centre, LFE, leftSurround, rightSurround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRsLcRcCs,                 { left, right, centre, leftSurround, rightSurround, topFrontLeft, topFrontRight, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLsRsCsSlSr,                 { left, right, centre, leftSurround, rightSurround, surround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsLcRcCs,              { left, right, centre, LFE, leftSurround, rightSurround, topFrontLeft, topFrontRight, surround, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsCsSlSr,              { left, right, centre, LFE, leftSurround, rightSurround, surround, leftSurroundRear, rightSurroundRear, unknown } },
            { Vst2::vstSpeakerConfigTypeLRCLfeLsRsTflTfcTfrTrlTrrLfe2, { left, right, centre, LFE, leftSurround, rightSurround, topFrontLeft, topFrontCentre, topFrontRight, topRearLeft, topRearRight, LFE2, unknown } },
            { Vst2::vstSpeakerConfigTypeEmpty,                         { unknown } }
        };

        return mappings;
    }

    static int32 getSpeakerType (AudioChannelSet::ChannelType type) noexcept
    {
        static const std::map<AudioChannelSet::ChannelType, int32> speakerTypeMap =
        {
            { AudioChannelSet::left,              Vst2::vstIndividualSpeakerTypeLeft },
            { AudioChannelSet::right,             Vst2::vstIndividualSpeakerTypeRight },
            { AudioChannelSet::centre,            Vst2::vstIndividualSpeakerTypeCentre },
            { AudioChannelSet::LFE,               Vst2::vstIndividualSpeakerTypeLFE },
            { AudioChannelSet::leftSurround,      Vst2::vstIndividualSpeakerTypeLeftSurround },
            { AudioChannelSet::rightSurround,     Vst2::vstIndividualSpeakerTypeRightSurround },
            { AudioChannelSet::leftCentre,        Vst2::vstIndividualSpeakerTypeLeftCentre },
            { AudioChannelSet::rightCentre,       Vst2::vstIndividualSpeakerTypeRightCentre },
            { AudioChannelSet::surround,          Vst2::vstIndividualSpeakerTypeSurround },
            { AudioChannelSet::leftSurroundRear,  Vst2::vstIndividualSpeakerTypeLeftRearSurround },
            { AudioChannelSet::rightSurroundRear, Vst2::vstIndividualSpeakerTypeRightRearSurround },
            { AudioChannelSet::topMiddle,         Vst2::vstIndividualSpeakerTypeTopMiddle },
            { AudioChannelSet::topFrontLeft,      Vst2::vstIndividualSpeakerTypeTopFrontLeft },
            { AudioChannelSet::topFrontCentre,    Vst2::vstIndividualSpeakerTypeTopFrontCentre },
            { AudioChannelSet::topFrontRight,     Vst2::vstIndividualSpeakerTypeTopFrontRight },
            { AudioChannelSet::topRearLeft,       Vst2::vstIndividualSpeakerTypeTopRearLeft },
            { AudioChannelSet::topRearCentre,     Vst2::vstIndividualSpeakerTypeTopRearCentre },
            { AudioChannelSet::topRearRight,      Vst2::vstIndividualSpeakerTypeTopRearRight },
            { AudioChannelSet::LFE2,              Vst2::vstIndividualSpeakerTypeLFE2 }
        };

        if (speakerTypeMap.find (type) == speakerTypeMap.end())
            return 0;

        return speakerTypeMap.at (type);
    }

    static AudioChannelSet::ChannelType getChannelType (int32 type) noexcept
    {
        switch (type)
        {
            case Vst2::vstIndividualSpeakerTypeLeft:                 return AudioChannelSet::left;
            case Vst2::vstIndividualSpeakerTypeRight:                return AudioChannelSet::right;
            case Vst2::vstIndividualSpeakerTypeCentre:               return AudioChannelSet::centre;
            case Vst2::vstIndividualSpeakerTypeLFE:                  return AudioChannelSet::LFE;
            case Vst2::vstIndividualSpeakerTypeLeftSurround:         return AudioChannelSet::leftSurround;
            case Vst2::vstIndividualSpeakerTypeRightSurround:        return AudioChannelSet::rightSurround;
            case Vst2::vstIndividualSpeakerTypeLeftCentre:           return AudioChannelSet::leftCentre;
            case Vst2::vstIndividualSpeakerTypeRightCentre:          return AudioChannelSet::rightCentre;
            case Vst2::vstIndividualSpeakerTypeSurround:             return AudioChannelSet::surround;
            case Vst2::vstIndividualSpeakerTypeLeftRearSurround:     return AudioChannelSet::leftSurroundRear;
            case Vst2::vstIndividualSpeakerTypeRightRearSurround:    return AudioChannelSet::rightSurroundRear;
            case Vst2::vstIndividualSpeakerTypeTopMiddle:            return AudioChannelSet::topMiddle;
            case Vst2::vstIndividualSpeakerTypeTopFrontLeft:         return AudioChannelSet::topFrontLeft;
            case Vst2::vstIndividualSpeakerTypeTopFrontCentre:       return AudioChannelSet::topFrontCentre;
            case Vst2::vstIndividualSpeakerTypeTopFrontRight:        return AudioChannelSet::topFrontRight;
            case Vst2::vstIndividualSpeakerTypeTopRearLeft:          return AudioChannelSet::topRearLeft;
            case Vst2::vstIndividualSpeakerTypeTopRearCentre:        return AudioChannelSet::topRearCentre;
            case Vst2::vstIndividualSpeakerTypeTopRearRight:         return AudioChannelSet::topRearRight;
            case Vst2::vstIndividualSpeakerTypeLFE2:                 return AudioChannelSet::LFE2;
            default: break;
        }

        return AudioChannelSet::unknown;
    }
};

} // namespace juce
