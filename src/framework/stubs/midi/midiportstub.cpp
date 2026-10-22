/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "midiportstub.h"

using namespace muse;
using namespace muse::midi;

// MidiOutPortStub

MidiDeviceList MidiOutPortStub::availableDevices() const
{
    return {};
}

async::Notification MidiOutPortStub::availableDevicesChanged() const
{
    return async::Notification();
}

Ret MidiOutPortStub::connect(const MidiDeviceID&)
{
    return make_ret(Ret::Code::NotSupported);
}

void MidiOutPortStub::disconnect()
{
}

bool MidiOutPortStub::isConnected() const
{
    return false;
}

MidiDeviceID MidiOutPortStub::deviceID() const
{
    return MidiDeviceID();
}

async::Notification MidiOutPortStub::deviceChanged() const
{
    return async::Notification();
}

bool MidiOutPortStub::supportsMIDI20Output() const
{
    return false;
}

Ret MidiOutPortStub::sendEvent(const Event&)
{
    return make_ret(Ret::Code::NotSupported);
}

// MidiInPortStub

MidiDeviceList MidiInPortStub::availableDevices() const
{
    return {};
}

async::Notification MidiInPortStub::availableDevicesChanged() const
{
    return async::Notification();
}

Ret MidiInPortStub::connect(const MidiDeviceID&)
{
    return make_ret(Ret::Code::NotSupported);
}

void MidiInPortStub::disconnect()
{
}

bool MidiInPortStub::isConnected() const
{
    return false;
}

MidiDeviceID MidiInPortStub::deviceID() const
{
    return MidiDeviceID();
}

async::Notification MidiInPortStub::deviceChanged() const
{
    return async::Notification();
}

async::Channel<tick_t, Event> MidiInPortStub::eventReceived() const
{
    return async::Channel<tick_t, Event>();
}
