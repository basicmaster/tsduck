//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsS2XSatelliteDeliverySystemDescriptor.h"
#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsVariable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"S2X_satellite_delivery_system_descriptor"
#define MY_CLASS ts::S2XSatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_S2X_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::S2XSatelliteDeliverySystemDescriptor::S2XSatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S2, MY_XML_NAME),
    receiver_profiles(0),
    S2X_mode(0),
    TS_GS_S2X_mode(0),
    scrambling_sequence_selector(false),
    scrambling_sequence_index(0),
    timeslice_number(0),
    master_channel(),
    num_channel_bonds_minus_one(false),
    channel_bond_0(),
    channel_bond_1(),
    reserved_future_use()
{
}

void ts::S2XSatelliteDeliverySystemDescriptor::clearContent()
{
    receiver_profiles = 0;
    S2X_mode = 0;
    TS_GS_S2X_mode = 0;
    scrambling_sequence_selector = false;
    scrambling_sequence_index = 0;
    timeslice_number = 0;
    master_channel.clear();
    num_channel_bonds_minus_one = false;
    channel_bond_0.clear();
    channel_bond_1.clear();
    reserved_future_use.clear();
}

ts::S2XSatelliteDeliverySystemDescriptor::S2XSatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    S2XSatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

ts::S2XSatelliteDeliverySystemDescriptor::Channel::Channel() :
    frequency(0),
    orbital_position(0),
    east_not_west(false),
    polarization(0),
    roll_off(0),
    symbol_rate(0),
    multiple_input_stream_flag(false),
    input_stream_identifier(0)
{
}

void ts::S2XSatelliteDeliverySystemDescriptor::Channel::clear()
{
    frequency = 0;
    orbital_position = 0;
    east_not_west = false;
    polarization = 0;
    roll_off = 0;
    symbol_rate = 0;
    multiple_input_stream_flag = false;
    input_stream_identifier = 0;
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::S2XSatelliteDeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(receiver_profiles, 5);
    buf.putBits(0x00, 3);
    buf.putBits(S2X_mode, 2);
    buf.putBit(scrambling_sequence_selector);
    buf.putBits(0x00, 3);
    buf.putBits(TS_GS_S2X_mode, 2);
    if (scrambling_sequence_selector) {
        buf.putBits(0x00, 6);
        buf.putBits(scrambling_sequence_index, 18);
    }
    serializeChannel(master_channel, buf);
    if (S2X_mode == 2) {
        buf.putUInt8(timeslice_number);
    }
    else if (S2X_mode == 3) {
        buf.putBits(0x00, 7);
        buf.putBit(num_channel_bonds_minus_one);
        serializeChannel(channel_bond_0, buf);
        if (num_channel_bonds_minus_one) {
            serializeChannel(channel_bond_1, buf);
        }
    }
    buf.putBytes(reserved_future_use);
}

// Serialization of a channel description.
void ts::S2XSatelliteDeliverySystemDescriptor::serializeChannel(const Channel& channel, PSIBuffer& buf) const
{
    buf.putBCD(channel.frequency / 10000, 8);  // unit is 10 kHz
    buf.putBCD(channel.orbital_position, 4);
    buf.putBit(channel.east_not_west);
    buf.putBits(channel.polarization, 2);
    buf.putBit(channel.multiple_input_stream_flag);
    buf.putBit(0);
    buf.putBits(channel.roll_off, 3);
    buf.putBits(0x00, 4);
    buf.putBCD(channel.symbol_rate / 100, 7); // unit is 100 sym/s
    if (channel.multiple_input_stream_flag) {
        buf.putUInt8(channel.input_stream_identifier);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    receiver_profiles = buf.getBits<uint8_t>(5);
    buf.skipBits(3);
    S2X_mode = buf.getBits<uint8_t>(2);
    scrambling_sequence_selector = buf.getBool();
    buf.skipBits(3);
    TS_GS_S2X_mode = buf.getBits<uint8_t>(2);
    if (scrambling_sequence_selector) {
        buf.skipBits(6);
        scrambling_sequence_index = buf.getBits<uint32_t>(18);
    }
    deserializeChannel(master_channel, buf);
    if (S2X_mode == 2) {
        timeslice_number = buf.getUInt8();
    }
    if (S2X_mode == 3) {
        buf.skipBits(7);
        num_channel_bonds_minus_one = buf.getBool();
        deserializeChannel(channel_bond_0, buf);
        if (num_channel_bonds_minus_one) {
            deserializeChannel(channel_bond_1, buf);
        }
    }
    buf.getBytes(reserved_future_use);
}

// Deserialization of a channel description.
void ts::S2XSatelliteDeliverySystemDescriptor::deserializeChannel(Channel& channel, PSIBuffer& buf)
{
    channel.frequency = buf.getBCD<uint64_t>(8) * 10000;  // unit is 10 Hz
    channel.orbital_position = buf.getBCD<uint16_t>(4);
    channel.east_not_west = buf.getBool();
    channel.polarization = buf.getBits<uint8_t>(2);
    channel.multiple_input_stream_flag = buf.getBool();
    buf.skipBits(1);
    channel.roll_off = buf.getBits<uint8_t>(3);
    buf.skipBits(4);
    channel.symbol_rate = buf.getBCD<uint64_t>(7) * 100;  // unit is 100 sym/sec
    if (channel.multiple_input_stream_flag) {
        channel.input_stream_identifier = buf.getUInt8();
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::S2XSatelliteDeliverySystemDescriptor::RollOffNames({
    {u"0.35",     0},
    {u"0.25",     1},
    {u"0.20",     2},
    {u"0.15",     4},
    {u"0.10",     5},
    {u"0.05",     6},
});


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    const UString margin(indent, ' ');
    bool ok = true;

    if (size >= 2) {
        const uint8_t profiles = (data[0] >> 3) & 0x1F;
        disp << margin << UString::Format(u"Receiver profiles: 0x%X", {profiles});
        if ((profiles & 0x01) != 0) {
            disp << ", broadcast services";
        }
        if ((profiles & 0x02) != 0) {
            disp << ", interactive services";
        }
        if ((profiles & 0x04) != 0) {
            disp << ", DSNG";
        }
        if ((profiles & 0x08) != 0) {
            disp << ", professional services";
        }
        if ((profiles & 0x10) != 0) {
            disp << ", VL-SNR";
        }
        const uint8_t mode = (data[1] >> 6) & 0x03;
        const bool sseq_sel = (data[1] & 0x20) != 0;
        disp << std::endl;
        disp << margin << "S2X mode: " << NameFromSection(u"S2XMode", mode, names::FIRST) << std::endl;
        disp << margin << "TS/GS S2X mode: " << NameFromSection(u"TSGSS2XMode", data[1] & 0x03, names::DECIMAL_FIRST) << std::endl;
        data += 2; size -= 2;

        if (ok && sseq_sel) {
            ok = size >= 3;
            if (ok) {
                disp << margin << UString::Format(u"Scrambling sequence index: 0x%05X", { GetUInt24(data) & 0x03FFFF }) << std::endl;
                data += 3; size -= 3;
            }
        }
        ok = ok && DisplayChannel(disp, u"Master channel", data, size, indent);
        if (ok && mode == 2) {
            ok = size >= 1;
            if (ok) {
                disp << margin << UString::Format(u"Timeslice number: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                data++; size--;
            }
        }
        if (ok && mode == 3) {
            ok = size >= 1;
            if (ok) {
                const bool num = (data[0] & 0x01) != 0;
                data++; size--;
                ok = DisplayChannel(disp, u"Channel bond 0", data, size, indent) &&
                    (!num || DisplayChannel(disp, u"Channel bond 1", data, size, indent));
            }
        }
        disp.displayPrivateData(u"Reserved for future use", data, size, margin);
    }
    else {
        disp.displayExtraData(data, size, margin);
    }
}

// Display a channel description.
bool ts::S2XSatelliteDeliverySystemDescriptor::DisplayChannel(TablesDisplay& disp, const UString& title, const uint8_t*& data, size_t& size, int indent)
{
    if (size < 11) {
        return false;
    }

    const UString margin(indent, ' ');

    const bool east = (data[6] & 0x80) != 0;
    const uint8_t polar = (data[6] >> 5) & 0x03;
    const bool multiple = (data[6] & 0x10) != 0;
    const uint8_t roll_off = data[6] & 0x07;
    std::string freq, srate, orbital;
    BCDToString(freq, data, 8, 3);
    BCDToString(orbital, data + 4, 4, 3);
    BCDToString(srate, data + 7, 7, 3, false);
    data += 11; size -= 11;

    disp << margin << title << ":" << std::endl
         << margin << "  Orbital position: " << orbital << " degree, " << (east ? "east" : "west") << std::endl
         << margin << "  Frequency: " << freq << " GHz" << std::endl
         << margin << "  Symbol rate: " << srate << " Msymbol/s" << std::endl
         << margin << "  Polarization: " << SatelliteDeliverySystemDescriptor::PolarizationNames.name(polar) << std::endl
         << margin << "  Roll-off factor: " << RollOffNames.name(roll_off) << std::endl
         << margin << "  Multiple input stream: " << UString::YesNo(multiple) << std::endl;

    if (multiple) {
        if (size < 1) {
            return false;
        }
        const uint8_t id = data[0];
        data++; size--;
        disp << margin << UString::Format(u"  Input stream identifier: 0x%X (%d)", {id, id}) << std::endl;
    }

    return true;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::S2XSatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"receiver_profiles", receiver_profiles, true);
    root->setIntAttribute(u"S2X_mode", S2X_mode, false);
    root->setIntAttribute(u"TS_GS_S2X_mode", TS_GS_S2X_mode, false);
    if (scrambling_sequence_selector) {
        root->setIntAttribute(u"scrambling_sequence_index", scrambling_sequence_index, true);
    }
    if (S2X_mode == 2) {
        root->setIntAttribute(u"timeslice_number", timeslice_number, true);
    }
    buildChannelXML(master_channel, root, u"master_channel");
    if (S2X_mode == 3) {
        buildChannelXML(channel_bond_0, root, u"channel_bond");
        if (num_channel_bonds_minus_one) {
            buildChannelXML(channel_bond_1, root, u"channel_bond");
        }
    }
    if (!reserved_future_use.empty()) {
        root->addHexaTextChild(u"reserved_future_use", reserved_future_use);
    }
}

// Build an XML element for a channel.
void ts::S2XSatelliteDeliverySystemDescriptor::buildChannelXML(const Channel& channel, xml::Element* parent, const UString& name) const
{
    xml::Element* e = parent->addElement(name);
    e->setIntAttribute(u"frequency", channel.frequency, false);
    e->setAttribute(u"orbital_position", UString::Format(u"%d.%d", {channel.orbital_position / 10, channel.orbital_position % 10}));
    e->setIntEnumAttribute(SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", channel.east_not_west);
    e->setIntEnumAttribute(SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", channel.polarization);
    e->setIntEnumAttribute(RollOffNames, u"roll_off", channel.roll_off);
    e->setIntAttribute(u"symbol_rate", channel.symbol_rate, false);
    if (channel.multiple_input_stream_flag) {
        e->setIntAttribute(u"input_stream_identifier", channel.input_stream_identifier, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::S2XSatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    Variable<uint32_t> scrambling;
    xml::ElementVector xmaster;
    xml::ElementVector xbond;

    bool ok =
        element->getIntAttribute<uint8_t>(receiver_profiles, u"receiver_profiles", true, 0, 0, 0x1F) &&
        element->getIntAttribute<uint8_t>(S2X_mode, u"S2X_mode", true, 0, 0, 0x03) &&
        element->getIntAttribute<uint8_t>(TS_GS_S2X_mode, u"TS_GS_S2X_mode", true, 0, 0, 0x03) &&
        element->getOptionalIntAttribute<uint32_t>(scrambling, u"scrambling_sequence_index", 0x00000000, 0x0003FFFF) &&
        (S2X_mode != 2 || element->getIntAttribute<uint8_t>(timeslice_number, u"timeslice_number", true)) &&
        element->getHexaTextChild(reserved_future_use, u"reserved_future_use") &&
        element->getChildren(xmaster, u"master_channel", 1, 1) &&
        element->getChildren(xbond, u"channel_bond", S2X_mode == 3 ? 1 : 0, S2X_mode == 3 ? 2 : 0) &&
        getChannelXML(master_channel, duck, xmaster[0]) &&
        (S2X_mode != 3 || getChannelXML(channel_bond_0, duck, xbond[0]));

    if (ok) {
        scrambling_sequence_selector = scrambling.set();
        scrambling_sequence_index = scrambling.value(0);
        num_channel_bonds_minus_one = S2X_mode == 3 && xbond.size() > 1;
        if (num_channel_bonds_minus_one) {
            ok = getChannelXML(channel_bond_1, duck, xbond[1]);
        }
    }
    return ok;
}

// Analyze an XML element for a channel.
bool ts::S2XSatelliteDeliverySystemDescriptor::getChannelXML(Channel& channel, DuckContext& duck, const xml::Element* element)
{
    UString orbit;
    Variable<uint8_t> stream;

    bool ok =
        element != nullptr &&
        element->getIntAttribute<uint64_t>(channel.frequency, u"frequency", true) &&
        element->getIntAttribute<uint64_t>(channel.symbol_rate, u"symbol_rate", true) &&
        element->getAttribute(orbit, u"orbital_position", true) &&
        element->getIntEnumAttribute(channel.east_not_west, SatelliteDeliverySystemDescriptor::DirectionNames, u"west_east_flag", true) &&
        element->getIntEnumAttribute(channel.polarization, SatelliteDeliverySystemDescriptor::PolarizationNames, u"polarization", true) &&
        element->getIntEnumAttribute(channel.roll_off, RollOffNames, u"roll_off", true) &&
        element->getOptionalIntAttribute<uint8_t>(stream, u"input_stream_identifier");

    if (ok) {
        channel.multiple_input_stream_flag = stream.set();
        channel.input_stream_identifier = stream.value(0);

        // Expected orbital position is "XX.X" as in "19.2".
        uint16_t o1, o2;
        ok = orbit.scan(u"%d.%d", {&o1, &o2});
        if (ok) {
            channel.orbital_position = (o1 * 10) + o2;
        }
        else {
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", {orbit, element->name(), element->lineNumber()});
        }
    }
    return ok;
}
