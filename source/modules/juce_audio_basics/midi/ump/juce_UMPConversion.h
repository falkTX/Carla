/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifndef DOXYGEN

namespace juce
{
namespace universal_midi_packets
{

/**
    Functions to assist conversion of UMP messages to/from other formats,
    especially older 'bytestream' formatted MidiMessages.

    @tags{Audio}
*/
struct Conversion
{
    /** Converts from a MIDI 1 bytestream to MIDI 1 on Universal MIDI Packets.

        `callback` is a function which accepts a single View argument.
    */
    template <typename PacketCallbackFunction>
    static void toMidi1 (const MidiMessage& m, PacketCallbackFunction&& callback)
    {
        const auto* data = m.getRawData();
        const auto firstByte = data[0];
        const auto size = m.getRawDataSize();

        if (firstByte != 0xf0)
        {
            const auto mask = [size]() -> uint32_t
            {
                switch (size)
                {
                    case 0: return 0xff000000;
                    case 1: return 0xffff0000;
                    case 2: return 0xffffff00;
                    case 3: return 0xffffffff;
                }

                return 0x00000000;
            }();

            const auto extraByte = (uint8_t) ((((firstByte & 0xf0) == 0xf0) ? 0x1 : 0x2) << 0x4);
            const PacketX1 packet { mask & Utils::bytesToWord (extraByte, data[0], data[1], data[2]) };
            callback (View (packet.data()));
            return;
        }

        const auto numSysExBytes = m.getSysExDataSize();
        const auto numMessages = SysEx7::getNumPacketsRequiredForDataSize ((uint32_t) numSysExBytes);
        auto* dataOffset = m.getSysExData();

        if (numMessages <= 1)
        {
            const auto packet = Factory::makeSysExIn1Packet (0, (uint8_t) numSysExBytes, dataOffset);
            callback (View (packet.data()));
            return;
        }

        constexpr auto byteIncrement = 6;

        for (auto i = numSysExBytes; i > 0; i -= byteIncrement, dataOffset += byteIncrement)
        {
            const auto func = [&]
            {
                if (i == numSysExBytes)
                    return Factory::makeSysExStart;

                if (i <= byteIncrement)
                    return Factory::makeSysExEnd;

                return Factory::makeSysExContinue;
            }();

            const auto bytesNow = std::min (byteIncrement, i);
            const auto packet = func (0, (uint8_t) bytesNow, dataOffset);
            callback (View (packet.data()));
        }
    }

    /** Converts a MidiMessage to one or more messages in UMP format, using
        the MIDI 1.0 Protocol.

        `packets` is an out-param to allow the caller to control
        allocation/deallocation. Returning a new Packets object would
        require every call to toMidi1 to allocate. With this version, no
        allocations will occur, provided that `packets` has adequate reserved
        space.
    */
    static void toMidi1 (const MidiMessage& m, Packets& packets)
    {
        toMidi1 (m, [&] (const View& view) { packets.add (view); });
    }

    /** Widens a 7-bit MIDI 1.0 value to a 8-bit MIDI 2.0 value. */
    static uint8_t scaleTo8 (uint8_t word7Bit)
    {
        const auto shifted = (uint8_t) (word7Bit << 0x1);
        const auto repeat = (uint8_t) (word7Bit & 0x3f);
        const auto mask = (uint8_t) (word7Bit <= 0x40 ? 0x0 : 0xff);
        return (uint8_t) (shifted | ((repeat >> 5) & mask));
    }

    /** Widens a 7-bit MIDI 1.0 value to a 16-bit MIDI 2.0 value. */
    static uint16_t scaleTo16 (uint8_t word7Bit)
    {
        const auto shifted = (uint16_t) (word7Bit << 0x9);
        const auto repeat = (uint16_t) (word7Bit & 0x3f);
        const auto mask = (uint16_t) (word7Bit <= 0x40 ? 0x0 : 0xffff);
        return (uint16_t) (shifted | (((repeat << 3) | (repeat >> 3)) & mask));
    }

    /** Widens a 14-bit MIDI 1.0 value to a 16-bit MIDI 2.0 value. */
    static uint16_t scaleTo16 (uint16_t word14Bit)
    {
        const auto shifted = (uint16_t) (word14Bit << 0x2);
        const auto repeat = (uint16_t) (word14Bit & 0x1fff);
        const auto mask = (uint16_t) (word14Bit <= 0x2000 ? 0x0 : 0xffff);
        return (uint16_t) (shifted | ((repeat >> 11) & mask));
    }

    /** Widens a 7-bit MIDI 1.0 value to a 32-bit MIDI 2.0 value. */
    static uint32_t scaleTo32 (uint8_t word7Bit)
    {
        const auto shifted = (uint32_t) (word7Bit << 0x19);
        const auto repeat = (uint32_t) (word7Bit & 0x3f);
        const auto mask = (uint32_t) (word7Bit <= 0x40 ? 0x0 : 0xffffffff);
        return (uint32_t) (shifted | (((repeat << 19)
                                     | (repeat << 13)
                                     | (repeat << 7)
                                     | (repeat << 1)
                                     | (repeat >> 5)) & mask));
    }

    /** Widens a 14-bit MIDI 1.0 value to a 32-bit MIDI 2.0 value. */
    static uint32_t scaleTo32 (uint16_t word14Bit)
    {
        const auto shifted = (uint32_t) (word14Bit << 0x12);
        const auto repeat = (uint32_t) (word14Bit & 0x1fff);
        const auto mask = (uint32_t) (word14Bit <= 0x2000 ? 0x0 : 0xffffffff);
        return (uint32_t) (shifted | (((repeat << 5) | (repeat >> 8)) & mask));
    }

    /** Narrows a 16-bit MIDI 2.0 value to a 7-bit MIDI 1.0 value. */
    static uint8_t scaleTo7 (uint8_t word8Bit) { return (uint8_t) (word8Bit >> 1); }

    /** Narrows a 16-bit MIDI 2.0 value to a 7-bit MIDI 1.0 value. */
    static uint8_t scaleTo7 (uint16_t word16Bit) { return (uint8_t) (word16Bit >> 9); }

    /** Narrows a 32-bit MIDI 2.0 value to a 7-bit MIDI 1.0 value. */
    static uint8_t scaleTo7 (uint32_t word32Bit) { return (uint8_t) (word32Bit >> 25); }

    /** Narrows a 32-bit MIDI 2.0 value to a 14-bit MIDI 1.0 value. */
    static uint16_t scaleTo14 (uint16_t word16Bit) { return (uint16_t) (word16Bit >> 2); }

    /** Narrows a 32-bit MIDI 2.0 value to a 14-bit MIDI 1.0 value. */
    static uint16_t scaleTo14 (uint32_t word32Bit) { return (uint16_t) (word32Bit >> 18); }

    /** Converts UMP messages which may include MIDI 2.0 channel voice messages into
        equivalent MIDI 1.0 messages (still in UMP format).

        `callback` is a function that accepts a single View argument and will be
        called with each converted packet.

        Note that not all MIDI 2.0 messages have MIDI 1.0 equivalents, so such
        messages will be ignored.
    */
    template <typename Callback>
    static void midi2ToMidi1DefaultTranslation (const View& v, Callback&& callback)
    {
        const auto firstWord = v[0];

        if (Utils::getMessageType (firstWord) != 0x4)
        {
            callback (v);
            return;
        }

        const auto status = Utils::getStatus (firstWord);
        const auto typeAndGroup = (uint8_t) ((0x2 << 0x4) | Utils::getGroup (firstWord));

        switch (status)
        {
            case 0x8:   // note off
            case 0x9:   // note on
            case 0xa:   // poly pressure
            case 0xb:   // control change
            {
                const auto statusAndChannel = (uint8_t) ((firstWord >> 0x10) & 0xff);
                const auto byte2 = (uint8_t) ((firstWord >> 0x08) & 0xff);
                const auto byte3 = scaleTo7 (v[1]);

                // If this is a note-on, and the scaled byte is 0,
                // the scaled velocity should be 1 instead of 0
                const auto needsCorrection = status == 0x9 && byte3 == 0;
                const auto correctedByte = (uint8_t) (needsCorrection ? 1 : byte3);

                const auto shouldIgnore = status == 0xb && [&]
                {
                    switch (byte2)
                    {
                        case 0:
                        case 6:
                        case 32:
                        case 38:
                        case 98:
                        case 99:
                        case 100:
                        case 101:
                            return true;
                    }

                    return false;
                }();

                if (shouldIgnore)
                    return;

                const PacketX1 packet { Utils::bytesToWord (typeAndGroup,
                                                            statusAndChannel,
                                                            byte2,
                                                            correctedByte) };
                callback (View (packet.data()));
                return;
            }

            case 0xd: // channel pressure
            {
                const auto statusAndChannel = (uint8_t) ((firstWord >> 0x10) & 0xff);
                const auto byte2 = scaleTo7 (v[1]);

                const PacketX1 packet { Utils::bytesToWord (typeAndGroup,
                                                            statusAndChannel,
                                                            byte2,
                                                            0) };
                callback (View (packet.data()));
                return;
            }

            case 0x2:   // rpn
            case 0x3:   // nrpn
            {
                const auto ccX = (uint8_t) (status == 0x2 ? 101 : 99);
                const auto ccY = (uint8_t) (status == 0x2 ? 100 : 98);
                const auto statusAndChannel = (uint8_t) ((0xb << 0x4) | Utils::getChannel (firstWord));
                const auto data = scaleTo14 (v[1]);

                const PacketX1 packets[]
                {
                    PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, ccX, (uint8_t) ((firstWord >> 0x8) & 0x7f)) },
                    PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, ccY, (uint8_t) ((firstWord >> 0x0) & 0x7f)) },
                    PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, 6,   (uint8_t) ((data >> 0x7) & 0x7f)) },
                    PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, 38,  (uint8_t) ((data >> 0x0) & 0x7f)) },
                };

                for (const auto& packet : packets)
                    callback (View (packet.data()));

                return;
            }

            case 0xc: // program change / bank select
            {
                if (firstWord & 1)
                {
                    const auto statusAndChannel = (uint8_t) ((0xb << 0x4) | Utils::getChannel (firstWord));
                    const auto secondWord = v[1];

                    const PacketX1 packets[]
                    {
                        PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, 0,  (uint8_t) ((secondWord >> 0x8) & 0x7f)) },
                        PacketX1 { Utils::bytesToWord (typeAndGroup, statusAndChannel, 32, (uint8_t) ((secondWord >> 0x0) & 0x7f)) },
                    };

                    for (const auto& packet : packets)
                        callback (View (packet.data()));
                }

                const auto statusAndChannel = (uint8_t) ((0xc << 0x4) | Utils::getChannel (firstWord));
                const PacketX1 packet { Utils::bytesToWord (typeAndGroup,
                                                            statusAndChannel,
                                                            (uint8_t) ((v[1] >> 0x18) & 0x7f),
                                                            0) };
                callback (View (packet.data()));
                return;
            }

            case 0xe: // pitch bend
            {
                const auto data = scaleTo14 (v[1]);
                const auto statusAndChannel = (uint8_t) ((firstWord >> 0x10) & 0xff);
                const PacketX1 packet { Utils::bytesToWord (typeAndGroup,
                                                            statusAndChannel,
                                                            (uint8_t) (data & 0x7f),
                                                            (uint8_t) ((data >> 7) & 0x7f)) };
                callback (View (packet.data()));
                return;
            }

            default: // other message types do not translate
                return;
        }
    }
};

}
}

#endif
