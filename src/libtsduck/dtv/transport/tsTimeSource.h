//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Sources of time information for transport streams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsTS.h" // required by GCC, see comment below

namespace ts {

    // GCC complains that PCR, DTS, PTS shadow the global types of the same name.
    // However this is irrelevant because TimeSource is an enum class and the declared
    // identifiers must be prefixed. ts::TimeSource::PCR is an enum identifier, ts::PCR
    // is a type, without ambiguity.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(shadow)
    //!
    //! Sources of time information for transport streams.
    //!
    enum class TimeSource : uint8_t {
        UNDEFINED = 0,  //!< Undefined source of time information.
        HARDWARE,       //!< Hardware-generated time, any local hardware (NIC for instance).
        KERNEL,         //!< OS kernel time stamp.
        TSP,            //!< Application time stamp, generated by tsp when the chunk of TS packets is received.
        RTP,            //!< RTP (Real Time Protocol) time stamp.
        SRT,            //!< SRT (Secure Reliable Transport) source time.
        M2TS,           //!< M2TS Bluray-style time stamp.
        PCR,            //!< PCR (Program Clock Reference), the transport stream system clock.
        DTS,            //!< DTS (Decoding Time Stamp), in a video or audio stream.
        PTS,            //!< PTS (Presentation Time Stamp), in a video or audio stream.
        PCAP,           //!< Timestamp from a pcap or pcap-ng file.
    };
    TS_POP_WARNING()

    //!
    //! Enumeration description of ts::TimeSource.
    //!
    TSDUCKDLL extern const Enumeration TimeSourceEnum;
}
