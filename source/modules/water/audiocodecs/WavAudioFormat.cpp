/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   Water is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ==============================================================================
*/

#include "WavAudioFormat.h"

#include "../audioformat/AudioFormatReader.h"
#include "../memory/ByteOrder.h"
#include "../memory/MemoryBlock.h"
#include "../streams/InputStream.h"
#include "../streams/MemoryOutputStream.h"
#include "../xml/XmlDocument.h"
#include "../xml/XmlElement.h"

namespace water {

static const char* const wavFormatName = "WAV file";

//==============================================================================
const char* const WavAudioFormat::bwavDescription      = "bwav description";
const char* const WavAudioFormat::bwavOriginator       = "bwav originator";
const char* const WavAudioFormat::bwavOriginatorRef    = "bwav originator ref";
const char* const WavAudioFormat::bwavOriginationDate  = "bwav origination date";
const char* const WavAudioFormat::bwavOriginationTime  = "bwav origination time";
const char* const WavAudioFormat::bwavTimeReference    = "bwav time reference";
const char* const WavAudioFormat::bwavCodingHistory    = "bwav coding history";

#if 0
StringPairArray WavAudioFormat::createBWAVMetadata (const String& description,
                                                    const String& originator,
                                                    const String& originatorRef,
                                                    Time date,
                                                    const int64 timeReferenceSamples,
                                                    const String& codingHistory)
{
    StringPairArray m;

    m.set (bwavDescription, description);
    m.set (bwavOriginator, originator);
    m.set (bwavOriginatorRef, originatorRef);
    m.set (bwavOriginationDate, date.formatted ("%Y-%m-%d"));
    m.set (bwavOriginationTime, date.formatted ("%H:%M:%S"));
    m.set (bwavTimeReference, String (timeReferenceSamples));
    m.set (bwavCodingHistory, codingHistory);

    return m;
}
#endif

const char* const WavAudioFormat::acidOneShot          = "acid one shot";
const char* const WavAudioFormat::acidRootSet          = "acid root set";
const char* const WavAudioFormat::acidStretch          = "acid stretch";
const char* const WavAudioFormat::acidDiskBased        = "acid disk based";
const char* const WavAudioFormat::acidizerFlag         = "acidizer flag";
const char* const WavAudioFormat::acidRootNote         = "acid root note";
const char* const WavAudioFormat::acidBeats            = "acid beats";
const char* const WavAudioFormat::acidDenominator      = "acid denominator";
const char* const WavAudioFormat::acidNumerator        = "acid numerator";
const char* const WavAudioFormat::acidTempo            = "acid tempo";

const char* const WavAudioFormat::riffInfoArchivalLocation      = "IARL";
const char* const WavAudioFormat::riffInfoArtist                = "IART";
const char* const WavAudioFormat::riffInfoBaseURL               = "IBSU";
const char* const WavAudioFormat::riffInfoCinematographer       = "ICNM";
const char* const WavAudioFormat::riffInfoComment               = "CMNT";
const char* const WavAudioFormat::riffInfoComments              = "COMM";
const char* const WavAudioFormat::riffInfoCommissioned          = "ICMS";
const char* const WavAudioFormat::riffInfoCopyright             = "ICOP";
const char* const WavAudioFormat::riffInfoCostumeDesigner       = "ICDS";
const char* const WavAudioFormat::riffInfoCountry               = "ICNT";
const char* const WavAudioFormat::riffInfoCropped               = "ICRP";
const char* const WavAudioFormat::riffInfoDateCreated           = "ICRD";
const char* const WavAudioFormat::riffInfoDateTimeOriginal      = "IDIT";
const char* const WavAudioFormat::riffInfoDefaultAudioStream    = "ICAS";
const char* const WavAudioFormat::riffInfoDimension             = "IDIM";
const char* const WavAudioFormat::riffInfoDirectory             = "DIRC";
const char* const WavAudioFormat::riffInfoDistributedBy         = "IDST";
const char* const WavAudioFormat::riffInfoDotsPerInch           = "IDPI";
const char* const WavAudioFormat::riffInfoEditedBy              = "IEDT";
const char* const WavAudioFormat::riffInfoEighthLanguage        = "IAS8";
const char* const WavAudioFormat::riffInfoEncodedBy             = "CODE";
const char* const WavAudioFormat::riffInfoEndTimecode           = "TCDO";
const char* const WavAudioFormat::riffInfoEngineer              = "IENG";
const char* const WavAudioFormat::riffInfoFifthLanguage         = "IAS5";
const char* const WavAudioFormat::riffInfoFirstLanguage         = "IAS1";
const char* const WavAudioFormat::riffInfoFourthLanguage        = "IAS4";
const char* const WavAudioFormat::riffInfoGenre                 = "GENR";
const char* const WavAudioFormat::riffInfoKeywords              = "IKEY";
const char* const WavAudioFormat::riffInfoLanguage              = "LANG";
const char* const WavAudioFormat::riffInfoLength                = "TLEN";
const char* const WavAudioFormat::riffInfoLightness             = "ILGT";
const char* const WavAudioFormat::riffInfoLocation              = "LOCA";
const char* const WavAudioFormat::riffInfoLogoIconURL           = "ILIU";
const char* const WavAudioFormat::riffInfoLogoURL               = "ILGU";
const char* const WavAudioFormat::riffInfoMedium                = "IMED";
const char* const WavAudioFormat::riffInfoMoreInfoBannerImage   = "IMBI";
const char* const WavAudioFormat::riffInfoMoreInfoBannerURL     = "IMBU";
const char* const WavAudioFormat::riffInfoMoreInfoText          = "IMIT";
const char* const WavAudioFormat::riffInfoMoreInfoURL           = "IMIU";
const char* const WavAudioFormat::riffInfoMusicBy               = "IMUS";
const char* const WavAudioFormat::riffInfoNinthLanguage         = "IAS9";
const char* const WavAudioFormat::riffInfoNumberOfParts         = "PRT2";
const char* const WavAudioFormat::riffInfoOrganisation          = "TORG";
const char* const WavAudioFormat::riffInfoPart                  = "PRT1";
const char* const WavAudioFormat::riffInfoProducedBy            = "IPRO";
const char* const WavAudioFormat::riffInfoProductionDesigner    = "IPDS";
const char* const WavAudioFormat::riffInfoProductionStudio      = "ISDT";
const char* const WavAudioFormat::riffInfoRate                  = "RATE";
const char* const WavAudioFormat::riffInfoRated                 = "AGES";
const char* const WavAudioFormat::riffInfoRating                = "IRTD";
const char* const WavAudioFormat::riffInfoRippedBy              = "IRIP";
const char* const WavAudioFormat::riffInfoSecondaryGenre        = "ISGN";
const char* const WavAudioFormat::riffInfoSecondLanguage        = "IAS2";
const char* const WavAudioFormat::riffInfoSeventhLanguage       = "IAS7";
const char* const WavAudioFormat::riffInfoSharpness             = "ISHP";
const char* const WavAudioFormat::riffInfoSixthLanguage         = "IAS6";
const char* const WavAudioFormat::riffInfoSoftware              = "ISFT";
const char* const WavAudioFormat::riffInfoSoundSchemeTitle      = "DISP";
const char* const WavAudioFormat::riffInfoSource                = "ISRC";
const char* const WavAudioFormat::riffInfoSourceFrom            = "ISRF";
const char* const WavAudioFormat::riffInfoStarring_ISTR         = "ISTR";
const char* const WavAudioFormat::riffInfoStarring_STAR         = "STAR";
const char* const WavAudioFormat::riffInfoStartTimecode         = "TCOD";
const char* const WavAudioFormat::riffInfoStatistics            = "STAT";
const char* const WavAudioFormat::riffInfoSubject               = "ISBJ";
const char* const WavAudioFormat::riffInfoTapeName              = "TAPE";
const char* const WavAudioFormat::riffInfoTechnician            = "ITCH";
const char* const WavAudioFormat::riffInfoThirdLanguage         = "IAS3";
const char* const WavAudioFormat::riffInfoTimeCode              = "ISMP";
const char* const WavAudioFormat::riffInfoTitle                 = "INAM";
const char* const WavAudioFormat::riffInfoTrackNumber           = "TRCK";
const char* const WavAudioFormat::riffInfoURL                   = "TURL";
const char* const WavAudioFormat::riffInfoVegasVersionMajor     = "VMAJ";
const char* const WavAudioFormat::riffInfoVegasVersionMinor     = "VMIN";
const char* const WavAudioFormat::riffInfoVersion               = "TVER";
const char* const WavAudioFormat::riffInfoWatermarkURL          = "IWMU";
const char* const WavAudioFormat::riffInfoWrittenBy             = "IWRI";
const char* const WavAudioFormat::riffInfoYear                  = "YEAR";

const char* const WavAudioFormat::ISRC                 = "ISRC";
const char* const WavAudioFormat::tracktionLoopInfo    = "tracktion loop info";

//==============================================================================
namespace WavFileHelpers
{
    inline int chunkName (const char* const name) noexcept   { return (int) ByteOrder::littleEndianInt (name); }
    inline size_t roundUpSize (size_t sz) noexcept           { return (sz + 3) & ~3u; }

    #if _MSVC_VER
     #pragma pack (push, 1)
    #endif

    struct BWAVChunk
    {
        char description [256];
        char originator [32];
        char originatorRef [32];
        char originationDate [10];
        char originationTime [8];
        uint32 timeRefLow;
        uint32 timeRefHigh;
        uint16 version;
        uint8 umid[64];
        uint8 reserved[190];
        char codingHistory[1];

        void copyTo (StringPairArray& values, const int totalSize) const
        {
            values.set (WavAudioFormat::bwavDescription,     String::fromUTF8 (description,     sizeof (description)));
            values.set (WavAudioFormat::bwavOriginator,      String::fromUTF8 (originator,      sizeof (originator)));
            values.set (WavAudioFormat::bwavOriginatorRef,   String::fromUTF8 (originatorRef,   sizeof (originatorRef)));
            values.set (WavAudioFormat::bwavOriginationDate, String::fromUTF8 (originationDate, sizeof (originationDate)));
            values.set (WavAudioFormat::bwavOriginationTime, String::fromUTF8 (originationTime, sizeof (originationTime)));

            const uint32 timeLow  = ByteOrder::swapIfBigEndian (timeRefLow);
            const uint32 timeHigh = ByteOrder::swapIfBigEndian (timeRefHigh);
            const int64 time = (((int64) timeHigh) << 32) + timeLow;

            values.set (WavAudioFormat::bwavTimeReference, String (time));
            values.set (WavAudioFormat::bwavCodingHistory,
                        String::fromUTF8 (codingHistory, totalSize - (int) offsetof (BWAVChunk, codingHistory)));
        }

    } WATER_PACKED;

    //==============================================================================
    struct SMPLChunk
    {
        struct SampleLoop
        {
            uint32 identifier;
            uint32 type; // these are different in AIFF and WAV
            uint32 start;
            uint32 end;
            uint32 fraction;
            uint32 playCount;
        } WATER_PACKED;

        uint32 manufacturer;
        uint32 product;
        uint32 samplePeriod;
        uint32 midiUnityNote;
        uint32 midiPitchFraction;
        uint32 smpteFormat;
        uint32 smpteOffset;
        uint32 numSampleLoops;
        uint32 samplerData;
        SampleLoop loops[1];

        template <typename NameType>
        static void setValue (StringPairArray& values, NameType name, uint32 val)
        {
            values.set (name, String (ByteOrder::swapIfBigEndian (val)));
        }

        static void setValue (StringPairArray& values, int prefix, const char* name, uint32 val)
        {
            setValue (values, "Loop" + String (prefix) + name, val);
        }

        void copyTo (StringPairArray& values, const int totalSize) const
        {
            setValue (values, "Manufacturer",      manufacturer);
            setValue (values, "Product",           product);
            setValue (values, "SamplePeriod",      samplePeriod);
            setValue (values, "MidiUnityNote",     midiUnityNote);
            setValue (values, "MidiPitchFraction", midiPitchFraction);
            setValue (values, "SmpteFormat",       smpteFormat);
            setValue (values, "SmpteOffset",       smpteOffset);
            setValue (values, "NumSampleLoops",    numSampleLoops);
            setValue (values, "SamplerData",       samplerData);

            for (int i = 0; i < (int) numSampleLoops; ++i)
            {
                if ((uint8*) (loops + (i + 1)) > ((uint8*) this) + totalSize)
                    break;

                setValue (values, i, "Identifier", loops[i].identifier);
                setValue (values, i, "Type",       loops[i].type);
                setValue (values, i, "Start",      loops[i].start);
                setValue (values, i, "End",        loops[i].end);
                setValue (values, i, "Fraction",   loops[i].fraction);
                setValue (values, i, "PlayCount",  loops[i].playCount);
            }
        }

        template <typename NameType>
        static uint32 getValue (const StringPairArray& values, NameType name, const char* def)
        {
            return ByteOrder::swapIfBigEndian ((uint32) values.getValue (name, def).getIntValue());
        }

        static uint32 getValue (const StringPairArray& values, int prefix, const char* name, const char* def)
        {
            return getValue (values, "Loop" + String (prefix) + name, def);
        }

    } WATER_PACKED;

    //==============================================================================
    struct InstChunk
    {
        int8 baseNote;
        int8 detune;
        int8 gain;
        int8 lowNote;
        int8 highNote;
        int8 lowVelocity;
        int8 highVelocity;

        static void setValue (StringPairArray& values, const char* name, int val)
        {
            values.set (name, String (val));
        }

        void copyTo (StringPairArray& values) const
        {
            setValue (values, "MidiUnityNote",  baseNote);
            setValue (values, "Detune",         detune);
            setValue (values, "Gain",           gain);
            setValue (values, "LowNote",        lowNote);
            setValue (values, "HighNote",       highNote);
            setValue (values, "LowVelocity",    lowVelocity);
            setValue (values, "HighVelocity",   highVelocity);
        }

        static int8 getValue (const StringPairArray& values, const char* name, const char* def)
        {
            return (int8) values.getValue (name, def).getIntValue();
        }

    } WATER_PACKED;

    //==============================================================================
    struct CueChunk
    {
        struct Cue
        {
            uint32 identifier;
            uint32 order;
            uint32 chunkID;
            uint32 chunkStart;
            uint32 blockStart;
            uint32 offset;
        } WATER_PACKED;

        uint32 numCues;
        Cue cues[1];

        static void setValue (StringPairArray& values, int prefix, const char* name, uint32 val)
        {
            values.set ("Cue" + String (prefix) + name, String (ByteOrder::swapIfBigEndian (val)));
        }

        void copyTo (StringPairArray& values, const int totalSize) const
        {
            values.set ("NumCuePoints", String (ByteOrder::swapIfBigEndian (numCues)));

            for (int i = 0; i < (int) numCues; ++i)
            {
                if ((uint8*) (cues + (i + 1)) > ((uint8*) this) + totalSize)
                    break;

                setValue (values, i, "Identifier",  cues[i].identifier);
                setValue (values, i, "Order",       cues[i].order);
                setValue (values, i, "ChunkID",     cues[i].chunkID);
                setValue (values, i, "ChunkStart",  cues[i].chunkStart);
                setValue (values, i, "BlockStart",  cues[i].blockStart);
                setValue (values, i, "Offset",      cues[i].offset);
            }
        }

    } WATER_PACKED;

    //==============================================================================
    /** Reads a RIFF List Info chunk from a stream positioned just after the size byte. */
    namespace ListInfoChunk
    {
        static const char* const types[] =
        {
            WavAudioFormat::riffInfoArchivalLocation,
            WavAudioFormat::riffInfoArtist,
            WavAudioFormat::riffInfoBaseURL,
            WavAudioFormat::riffInfoCinematographer,
            WavAudioFormat::riffInfoComment,
            WavAudioFormat::riffInfoComments,
            WavAudioFormat::riffInfoCommissioned,
            WavAudioFormat::riffInfoCopyright,
            WavAudioFormat::riffInfoCostumeDesigner,
            WavAudioFormat::riffInfoCountry,
            WavAudioFormat::riffInfoCropped,
            WavAudioFormat::riffInfoDateCreated,
            WavAudioFormat::riffInfoDateTimeOriginal,
            WavAudioFormat::riffInfoDefaultAudioStream,
            WavAudioFormat::riffInfoDimension,
            WavAudioFormat::riffInfoDirectory,
            WavAudioFormat::riffInfoDistributedBy,
            WavAudioFormat::riffInfoDotsPerInch,
            WavAudioFormat::riffInfoEditedBy,
            WavAudioFormat::riffInfoEighthLanguage,
            WavAudioFormat::riffInfoEncodedBy,
            WavAudioFormat::riffInfoEndTimecode,
            WavAudioFormat::riffInfoEngineer,
            WavAudioFormat::riffInfoFifthLanguage,
            WavAudioFormat::riffInfoFirstLanguage,
            WavAudioFormat::riffInfoFourthLanguage,
            WavAudioFormat::riffInfoGenre,
            WavAudioFormat::riffInfoKeywords,
            WavAudioFormat::riffInfoLanguage,
            WavAudioFormat::riffInfoLength,
            WavAudioFormat::riffInfoLightness,
            WavAudioFormat::riffInfoLocation,
            WavAudioFormat::riffInfoLogoIconURL,
            WavAudioFormat::riffInfoLogoURL,
            WavAudioFormat::riffInfoMedium,
            WavAudioFormat::riffInfoMoreInfoBannerImage,
            WavAudioFormat::riffInfoMoreInfoBannerURL,
            WavAudioFormat::riffInfoMoreInfoText,
            WavAudioFormat::riffInfoMoreInfoURL,
            WavAudioFormat::riffInfoMusicBy,
            WavAudioFormat::riffInfoNinthLanguage,
            WavAudioFormat::riffInfoNumberOfParts,
            WavAudioFormat::riffInfoOrganisation,
            WavAudioFormat::riffInfoPart,
            WavAudioFormat::riffInfoProducedBy,
            WavAudioFormat::riffInfoProductionDesigner,
            WavAudioFormat::riffInfoProductionStudio,
            WavAudioFormat::riffInfoRate,
            WavAudioFormat::riffInfoRated,
            WavAudioFormat::riffInfoRating,
            WavAudioFormat::riffInfoRippedBy,
            WavAudioFormat::riffInfoSecondaryGenre,
            WavAudioFormat::riffInfoSecondLanguage,
            WavAudioFormat::riffInfoSeventhLanguage,
            WavAudioFormat::riffInfoSharpness,
            WavAudioFormat::riffInfoSixthLanguage,
            WavAudioFormat::riffInfoSoftware,
            WavAudioFormat::riffInfoSoundSchemeTitle,
            WavAudioFormat::riffInfoSource,
            WavAudioFormat::riffInfoSourceFrom,
            WavAudioFormat::riffInfoStarring_ISTR,
            WavAudioFormat::riffInfoStarring_STAR,
            WavAudioFormat::riffInfoStartTimecode,
            WavAudioFormat::riffInfoStatistics,
            WavAudioFormat::riffInfoSubject,
            WavAudioFormat::riffInfoTapeName,
            WavAudioFormat::riffInfoTechnician,
            WavAudioFormat::riffInfoThirdLanguage,
            WavAudioFormat::riffInfoTimeCode,
            WavAudioFormat::riffInfoTitle,
            WavAudioFormat::riffInfoTrackNumber,
            WavAudioFormat::riffInfoURL,
            WavAudioFormat::riffInfoVegasVersionMajor,
            WavAudioFormat::riffInfoVegasVersionMinor,
            WavAudioFormat::riffInfoVersion,
            WavAudioFormat::riffInfoWatermarkURL,
            WavAudioFormat::riffInfoWrittenBy,
            WavAudioFormat::riffInfoYear
        };

        static bool isMatchingTypeIgnoringCase (const int value, const char* const name) noexcept
        {
            for (int i = 0; i < 4; ++i)
                if ((uint32) name[i] != CharacterFunctions::toUpperCase ((uint32) ((value >> (i * 8)) & 0xff)))
                    return false;

            return true;
        }

        static void addToMetadata (StringPairArray& values, InputStream& input, int64 chunkEnd)
        {
            while (input.getPosition() < chunkEnd)
            {
                const int infoType = input.readInt();

                int64 infoLength = chunkEnd - input.getPosition();

                if (infoLength > 0)
                {
                    infoLength = jmin (infoLength, (int64) input.readInt());

                    if (infoLength <= 0)
                        return;

                    for (int i = 0; i < numElementsInArray (types); ++i)
                    {
                        if (isMatchingTypeIgnoringCase (infoType, types[i]))
                        {
                            MemoryBlock mb;
                            input.readIntoMemoryBlock (mb, (ssize_t) infoLength);
                            values.set (types[i], String::createStringFromData ((const char*) mb.getData(),
                                                                                (int) mb.getSize()));
                            break;
                        }
                    }
                }
            }
        }
    }

    //==============================================================================
    struct AcidChunk
    {
        /** Reads an acid RIFF chunk from a stream positioned just after the size byte. */
        AcidChunk (InputStream& input, size_t length)
        {
            zerostruct (*this);
            input.read (this, (int) jmin (sizeof (*this), length));
        }

        AcidChunk (const StringPairArray& values)
        {
            zerostruct (*this);

            flags = getFlagIfPresent (values, WavAudioFormat::acidOneShot,   0x01)
                  | getFlagIfPresent (values, WavAudioFormat::acidRootSet,   0x02)
                  | getFlagIfPresent (values, WavAudioFormat::acidStretch,   0x04)
                  | getFlagIfPresent (values, WavAudioFormat::acidDiskBased, 0x08)
                  | getFlagIfPresent (values, WavAudioFormat::acidizerFlag,  0x10);

            if (values[WavAudioFormat::acidRootSet].getIntValue() != 0)
                rootNote = ByteOrder::swapIfBigEndian ((uint16) values[WavAudioFormat::acidRootNote].getIntValue());

            numBeats          = ByteOrder::swapIfBigEndian ((uint32) values[WavAudioFormat::acidBeats].getIntValue());
            meterDenominator  = ByteOrder::swapIfBigEndian ((uint16) values[WavAudioFormat::acidDenominator].getIntValue());
            meterNumerator    = ByteOrder::swapIfBigEndian ((uint16) values[WavAudioFormat::acidNumerator].getIntValue());

            if (values.containsKey (WavAudioFormat::acidTempo))
                tempo = swapFloatByteOrder (values[WavAudioFormat::acidTempo].getFloatValue());
        }

        MemoryBlock toMemoryBlock() const
        {
            return (flags != 0 || rootNote != 0 || numBeats != 0 || meterDenominator != 0 || meterNumerator != 0)
                      ? MemoryBlock (this, sizeof (*this)) : MemoryBlock();
        }

        void addToMetadata (StringPairArray& values) const
        {
            setBoolFlag (values, WavAudioFormat::acidOneShot,   0x01);
            setBoolFlag (values, WavAudioFormat::acidRootSet,   0x02);
            setBoolFlag (values, WavAudioFormat::acidStretch,   0x04);
            setBoolFlag (values, WavAudioFormat::acidDiskBased, 0x08);
            setBoolFlag (values, WavAudioFormat::acidizerFlag,  0x10);

            if (flags & 0x02) // root note set
                values.set (WavAudioFormat::acidRootNote, String (ByteOrder::swapIfBigEndian (rootNote)));

            values.set (WavAudioFormat::acidBeats,       String (ByteOrder::swapIfBigEndian (numBeats)));
            values.set (WavAudioFormat::acidDenominator, String (ByteOrder::swapIfBigEndian (meterDenominator)));
            values.set (WavAudioFormat::acidNumerator,   String (ByteOrder::swapIfBigEndian (meterNumerator)));
            values.set (WavAudioFormat::acidTempo,       String (swapFloatByteOrder (tempo)));
        }

        void setBoolFlag (StringPairArray& values, const char* name, uint32 mask) const
        {
            values.set (name, (flags & ByteOrder::swapIfBigEndian (mask)) ? "1" : "0");
        }

        static uint32 getFlagIfPresent (const StringPairArray& values, const char* name, uint32 flag)
        {
            return values[name].getIntValue() != 0 ? ByteOrder::swapIfBigEndian (flag) : 0;
        }

        static float swapFloatByteOrder (const float x) noexcept
        {
           #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
            union { uint32 asInt; float asFloat; } n;
            n.asFloat = x;
            n.asInt = ByteOrder::swap (n.asInt);
            return n.asFloat;
           #else
            return x;
           #endif
        }

        uint32 flags;
        uint16 rootNote;
        uint16 reserved1;
        float reserved2;
        uint32 numBeats;
        uint16 meterDenominator;
        uint16 meterNumerator;
        float tempo;

    } WATER_PACKED;

    //==============================================================================
    namespace AXMLChunk
    {
        static void addToMetadata (StringPairArray& destValues, const String& source)
        {
            ScopedPointer<XmlElement> xml (XmlDocument::parse (source));

            if (xml != nullptr && xml->hasTagName ("ebucore:ebuCoreMain"))
            {
                if (XmlElement* xml2 = xml->getChildByName ("ebucore:coreMetadata"))
                {
                    if (XmlElement* xml3 = xml2->getChildByName ("ebucore:identifier"))
                    {
                        if (XmlElement* xml4 = xml3->getChildByName ("dc:identifier"))
                        {
                            const String ISRCCode (xml4->getAllSubText().fromFirstOccurrenceOf ("ISRC:", false, true));

                            if (ISRCCode.isNotEmpty())
                                destValues.set (WavAudioFormat::ISRC, ISRCCode);
                        }
                    }
                }
            }
        }
    };

    //==============================================================================
    struct ExtensibleWavSubFormat
    {
        uint32 data1;
        uint16 data2;
        uint16 data3;
        uint8  data4[8];

        bool operator== (const ExtensibleWavSubFormat& other) const noexcept   { return memcmp (this, &other, sizeof (*this)) == 0; }
        bool operator!= (const ExtensibleWavSubFormat& other) const noexcept   { return ! operator== (other); }

    } WATER_PACKED;

    static const ExtensibleWavSubFormat pcmFormat       = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
    static const ExtensibleWavSubFormat IEEEFloatFormat = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
    static const ExtensibleWavSubFormat ambisonicFormat = { 0x00000001, 0x0721, 0x11d3, { 0x86, 0x44, 0xC8, 0xC1, 0xCA, 0x00, 0x00, 0x00 } };

    struct DataSize64Chunk   // chunk ID = 'ds64' if data size > 0xffffffff, 'JUNK' otherwise
    {
        uint32 riffSizeLow;     // low 4 byte size of RF64 block
        uint32 riffSizeHigh;    // high 4 byte size of RF64 block
        uint32 dataSizeLow;     // low 4 byte size of data chunk
        uint32 dataSizeHigh;    // high 4 byte size of data chunk
        uint32 sampleCountLow;  // low 4 byte sample count of fact chunk
        uint32 sampleCountHigh; // high 4 byte sample count of fact chunk
        uint32 tableLength;     // number of valid entries in array 'table'
    } WATER_PACKED;

    #if _MSVC_VER
     #pragma pack (pop)
    #endif
}

//==============================================================================
class WavAudioFormatReader  : public AudioFormatReader
{
public:
    WavAudioFormatReader (InputStream* const in)
        : AudioFormatReader (in, wavFormatName),
          bwavChunkStart (0),
          bwavSize (0),
          dataLength (0),
          isRF64 (false)
    {
        using namespace WavFileHelpers;
        uint64 len = 0;
        uint64 end = 0;
        int cueNoteIndex = 0;
        int cueLabelIndex = 0;
        int cueRegionIndex = 0;

        const int firstChunkType = input->readInt();

        if (firstChunkType == chunkName ("RF64"))
        {
            input->skipNextBytes (4); // size is -1 for RF64
            isRF64 = true;
        }
        else if (firstChunkType == chunkName ("RIFF"))
        {
            len = (uint64) (uint32) input->readInt();
            end = len + (uint64) input->getPosition();
        }
        else
        {
            return;
        }

        const int64 startOfRIFFChunk = input->getPosition();

        if (input->readInt() == chunkName ("WAVE"))
        {
            if (isRF64 && input->readInt() == chunkName ("ds64"))
            {
                const uint32 length = (uint32) input->readInt();

                if (length < 28)
                    return;

                const int64 chunkEnd = input->getPosition() + length + (length & 1);
                len = (uint64) input->readInt64();
                end = len + (uint64) startOfRIFFChunk;
                dataLength = input->readInt64();
                input->setPosition (chunkEnd);
            }

            while ((uint64) input->getPosition() < end && ! input->isExhausted())
            {
                const int chunkType = input->readInt();
                uint32 length = (uint32) input->readInt();
                const int64 chunkEnd = input->getPosition() + length + (length & 1);

                if (chunkType == chunkName ("fmt "))
                {
                    // read the format chunk
                    const unsigned short format = (unsigned short) input->readShort();
                    numChannels = (unsigned int) input->readShort();
                    sampleRate = input->readInt();
                    const int bytesPerSec = input->readInt();
                    input->skipNextBytes (2);
                    bitsPerSample = (unsigned int) (int) input->readShort();

                    if (bitsPerSample > 64)
                    {
                        bytesPerFrame = bytesPerSec / (int) sampleRate;
                        bitsPerSample = 8 * (unsigned int) bytesPerFrame / numChannels;
                    }
                    else
                    {
                        bytesPerFrame = numChannels * bitsPerSample / 8;
                    }

                    if (format == 3)
                    {
                        usesFloatingPointData = true;
                    }
                    else if (format == 0xfffe /*WAVE_FORMAT_EXTENSIBLE*/)
                    {
                        if (length < 40) // too short
                        {
                            bytesPerFrame = 0;
                        }
                        else
                        {
                            input->skipNextBytes (4); // skip over size and bitsPerSample
                            metadataValues.set ("ChannelMask", String (input->readInt()));

                            ExtensibleWavSubFormat subFormat;
                            subFormat.data1 = (uint32) input->readInt();
                            subFormat.data2 = (uint16) input->readShort();
                            subFormat.data3 = (uint16) input->readShort();
                            input->read (subFormat.data4, sizeof (subFormat.data4));

                            if (subFormat == IEEEFloatFormat)
                                usesFloatingPointData = true;
                            else if (subFormat != pcmFormat && subFormat != ambisonicFormat)
                                bytesPerFrame = 0;
                        }
                    }
                    else if (format != 1)
                    {
                        bytesPerFrame = 0;
                    }
                }
                else if (chunkType == chunkName ("data"))
                {
                    if (! isRF64) // data size is expected to be -1, actual data size is in ds64 chunk
                        dataLength = length;

                    dataChunkStart = input->getPosition();
                    lengthInSamples = (bytesPerFrame > 0) ? (dataLength / bytesPerFrame) : 0;
                }
                else if (chunkType == chunkName ("bext"))
                {
                    bwavChunkStart = input->getPosition();
                    bwavSize = length;

                    HeapBlock<BWAVChunk> bwav;
                    bwav.calloc (jmax ((size_t) length + 1, sizeof (BWAVChunk)), 1);
                    input->read (bwav, (int) length);
                    bwav->copyTo (metadataValues, (int) length);
                }
                else if (chunkType == chunkName ("smpl"))
                {
                    HeapBlock<SMPLChunk> smpl;
                    smpl.calloc (jmax ((size_t) length + 1, sizeof (SMPLChunk)), 1);
                    input->read (smpl, (int) length);
                    smpl->copyTo (metadataValues, (int) length);
                }
                else if (chunkType == chunkName ("inst") || chunkType == chunkName ("INST")) // need to check which...
                {
                    HeapBlock<InstChunk> inst;
                    inst.calloc (jmax ((size_t) length + 1, sizeof (InstChunk)), 1);
                    input->read (inst, (int) length);
                    inst->copyTo (metadataValues);
                }
                else if (chunkType == chunkName ("cue "))
                {
                    HeapBlock<CueChunk> cue;
                    cue.calloc (jmax ((size_t) length + 1, sizeof (CueChunk)), 1);
                    input->read (cue, (int) length);
                    cue->copyTo (metadataValues, (int) length);
                }
                else if (chunkType == chunkName ("axml"))
                {
                    MemoryBlock axml;
                    input->readIntoMemoryBlock (axml, (ssize_t) length);
                    AXMLChunk::addToMetadata (metadataValues, axml.toString());
                }
                else if (chunkType == chunkName ("LIST"))
                {
                    const int subChunkType = input->readInt();

                    if (subChunkType == chunkName ("info") || subChunkType == chunkName ("INFO"))
                    {
                        ListInfoChunk::addToMetadata (metadataValues, *input, chunkEnd);
                    }
                    else if (subChunkType == chunkName ("adtl"))
                    {
                        while (input->getPosition() < chunkEnd)
                        {
                            const int adtlChunkType = input->readInt();
                            const uint32 adtlLength = (uint32) input->readInt();
                            const int64 adtlChunkEnd = input->getPosition() + (adtlLength + (adtlLength & 1));

                            if (adtlChunkType == chunkName ("labl") || adtlChunkType == chunkName ("note"))
                            {
                                String prefix;

                                if (adtlChunkType == chunkName ("labl"))
                                    prefix << "CueLabel" << cueLabelIndex++;
                                else if (adtlChunkType == chunkName ("note"))
                                    prefix << "CueNote" << cueNoteIndex++;

                                const uint32 identifier = (uint32) input->readInt();
                                const int stringLength = (int) adtlLength - 4;

                                MemoryBlock textBlock;
                                input->readIntoMemoryBlock (textBlock, stringLength);

                                metadataValues.set (prefix + "Identifier", String (identifier));
                                metadataValues.set (prefix + "Text", textBlock.toString());
                            }
                            else if (adtlChunkType == chunkName ("ltxt"))
                            {
                                const String prefix ("CueRegion" + String (cueRegionIndex++));
                                const uint32 identifier     = (uint32) input->readInt();
                                const uint32 sampleLength   = (uint32) input->readInt();
                                const uint32 purpose        = (uint32) input->readInt();
                                const uint16 country        = (uint16) input->readInt();
                                const uint16 language       = (uint16) input->readInt();
                                const uint16 dialect        = (uint16) input->readInt();
                                const uint16 codePage       = (uint16) input->readInt();
                                const uint32 stringLength   = adtlLength - 20;

                                MemoryBlock textBlock;
                                input->readIntoMemoryBlock (textBlock, (int) stringLength);

                                metadataValues.set (prefix + "Identifier",   String (identifier));
                                metadataValues.set (prefix + "SampleLength", String (sampleLength));
                                metadataValues.set (prefix + "Purpose",      String (purpose));
                                metadataValues.set (prefix + "Country",      String (country));
                                metadataValues.set (prefix + "Language",     String (language));
                                metadataValues.set (prefix + "Dialect",      String (dialect));
                                metadataValues.set (prefix + "CodePage",     String (codePage));
                                metadataValues.set (prefix + "Text",         textBlock.toString());
                            }

                            input->setPosition (adtlChunkEnd);
                        }
                    }
                }
                else if (chunkType == chunkName ("acid"))
                {
                    AcidChunk (*input, length).addToMetadata (metadataValues);
                }
                else if (chunkType == chunkName ("Trkn"))
                {
                    MemoryBlock tracktion;
                    input->readIntoMemoryBlock (tracktion, (ssize_t) length);
                    metadataValues.set (WavAudioFormat::tracktionLoopInfo, tracktion.toString());
                }
                else if (chunkEnd <= input->getPosition())
                {
                    break;
                }

                input->setPosition (chunkEnd);
            }
        }

        if (cueLabelIndex > 0)          metadataValues.set ("NumCueLabels",    String (cueLabelIndex));
        if (cueNoteIndex > 0)           metadataValues.set ("NumCueNotes",     String (cueNoteIndex));
        if (cueRegionIndex > 0)         metadataValues.set ("NumCueRegions",   String (cueRegionIndex));
        if (metadataValues.size() > 0)  metadataValues.set ("MetaDataSource",  "WAV");
    }

    //==============================================================================
    bool readSamples (int** destSamples, int numDestChannels, int startOffsetInDestBuffer,
                      int64 startSampleInFile, int numSamples) override
    {
        clearSamplesBeyondAvailableLength (destSamples, numDestChannels, startOffsetInDestBuffer,
                                           startSampleInFile, numSamples, lengthInSamples);

        if (numSamples <= 0)
            return true;

        input->setPosition (dataChunkStart + startSampleInFile * bytesPerFrame);

        while (numSamples > 0)
        {
            const int tempBufSize = 480 * 3 * 4; // (keep this a multiple of 3)
            char tempBuffer [tempBufSize];

            const int numThisTime = jmin (tempBufSize / bytesPerFrame, numSamples);
            const int bytesRead = input->read (tempBuffer, numThisTime * bytesPerFrame);

            if (bytesRead < numThisTime * bytesPerFrame)
            {
                jassert (bytesRead >= 0);
                zeromem (tempBuffer + bytesRead, (size_t) (numThisTime * bytesPerFrame - bytesRead));
            }

            copySampleData (bitsPerSample, usesFloatingPointData,
                            destSamples, startOffsetInDestBuffer, numDestChannels,
                            tempBuffer, (int) numChannels, numThisTime);

            startOffsetInDestBuffer += numThisTime;
            numSamples -= numThisTime;
        }

        return true;
    }

    static void copySampleData (unsigned int bitsPerSample, const bool usesFloatingPointData,
                                int* const* destSamples, int startOffsetInDestBuffer, int numDestChannels,
                                const void* sourceData, int numChannels, int numSamples) noexcept
    {
        switch (bitsPerSample)
        {
            case 8:
                ReadHelper<AudioData::Int32, AudioData::UInt8, AudioData::LittleEndian>::read (destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, numChannels, numSamples);
                break;
            case 16:
                ReadHelper<AudioData::Int32, AudioData::Int16, AudioData::LittleEndian>::read (destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, numChannels, numSamples);
                break;
            case 24:
                ReadHelper<AudioData::Int32, AudioData::Int24, AudioData::LittleEndian>::read (destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, numChannels, numSamples);
                break;
            case 32:
                if (usesFloatingPointData)
                    ReadHelper<AudioData::Float32, AudioData::Float32, AudioData::LittleEndian>::read (destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, numChannels, numSamples);
                else
                    ReadHelper<AudioData::Int32,   AudioData::Int32,   AudioData::LittleEndian>::read (destSamples, startOffsetInDestBuffer, numDestChannels, sourceData, numChannels, numSamples);
                break;
            default:
                jassertfalse;
                break;
        }
    }

    int64 bwavChunkStart, bwavSize;
    int64 dataChunkStart, dataLength;
    int bytesPerFrame;
    bool isRF64;

private:
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavAudioFormatReader)
};

//==============================================================================
WavAudioFormat::WavAudioFormat()  : AudioFormat (wavFormatName, ".wav .bwf") {}
WavAudioFormat::~WavAudioFormat() {}

Array<int> WavAudioFormat::getPossibleSampleRates()
{
    const int rates[] = { 8000, 11025, 12000, 16000, 22050, 32000, 44100,
                          48000, 88200, 96000, 176400, 192000, 352800, 384000 };

    return Array<int> (rates, numElementsInArray (rates));
}

Array<int> WavAudioFormat::getPossibleBitDepths()
{
    const int depths[] = { 8, 16, 24, 32 };

    return Array<int> (depths, numElementsInArray (depths));
}

bool WavAudioFormat::canDoStereo()  { return true; }
bool WavAudioFormat::canDoMono()    { return true; }

AudioFormatReader* WavAudioFormat::createReaderFor (InputStream* sourceStream,
                                                    const bool deleteStreamIfOpeningFails)
{
    ScopedPointer<WavAudioFormatReader> r (new WavAudioFormatReader (sourceStream));

    if (r->sampleRate > 0 && r->numChannels > 0 && r->bytesPerFrame > 0)
        return r.release();

    if (! deleteStreamIfOpeningFails)
        r->input = nullptr;

    return nullptr;
}

}
