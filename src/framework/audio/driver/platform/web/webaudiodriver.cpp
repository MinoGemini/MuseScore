/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
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
#include "webaudiodriver.h"

#include <algorithm>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include "log.h"

//! NOTE No using-directive for emscripten: this file is part of a unity build,
//! and emscripten::async would clash with muse::async in the other sources.

using namespace muse;
using namespace muse::audio;

namespace muse::audio::web {
static const AudioDeviceID DEFAULT_DEVICE_ID = "default";

//! NOTE The ScriptProcessor callback is a free function, so the open driver's
//! state lives here. Everything is touched only on the browser's main thread.
static emscripten::val context = emscripten::val::undefined();
static emscripten::val processor = emscripten::val::undefined();
static const IAudioDriver::Spec* spec = nullptr;
static std::vector<float> interleaved;
static std::vector<float> planar;

static void audioCallback(emscripten::val event)
{
    if (!spec || !spec->callback) {
        return;
    }

    emscripten::val outputBuffer = event["outputBuffer"];
    const int frames = outputBuffer["length"].as<int>();
    const int channels = outputBuffer["numberOfChannels"].as<int>();
    const size_t sampleCount = static_cast<size_t>(frames) * channels;

    if (interleaved.size() != sampleCount) {
        interleaved.resize(sampleCount);
    }
    if (planar.size() != static_cast<size_t>(frames)) {
        planar.resize(frames);
    }

    spec->callback(spec->userdata, reinterpret_cast<uint8_t*>(interleaved.data()), static_cast<int>(sampleCount * sizeof(float)));

    // De-interleave and copy each channel with a single Float32Array.set()
    for (int ch = 0; ch < channels; ++ch) {
        for (int i = 0; i < frames; ++i) {
            planar[i] = interleaved[static_cast<size_t>(i) * channels + ch];
        }
        outputBuffer.call<emscripten::val>("getChannelData", ch)
        .call<void>("set", emscripten::val(emscripten::typed_memory_view(planar.size(), planar.data())));
    }
}

//! NOTE Browsers start an AudioContext suspended until the user interacts with the page
static EM_BOOL resumeOnUserGesture(int, const void*, void*)
{
    if (!context.isUndefined() && context["state"].as<std::string>() == "suspended") {
        context.call<emscripten::val>("resume");
    }
    return EM_FALSE;
}

static EM_BOOL resumeOnMouseDown(int eventType, const EmscriptenMouseEvent* e, void* userData)
{
    return resumeOnUserGesture(eventType, e, userData);
}

static EM_BOOL resumeOnKeyDown(int eventType, const EmscriptenKeyboardEvent* e, void* userData)
{
    return resumeOnUserGesture(eventType, e, userData);
}
}

EMSCRIPTEN_BINDINGS(muse_audio_web)
{
    emscripten::function("audioCallback", &muse::audio::web::audioCallback);
}

void WebAudioDriver::init()
{
}

std::string WebAudioDriver::name() const
{
    return "MUAUDIO(WEB)";
}

bool WebAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    if (isOpened()) {
        if (activeSpec) {
            *activeSpec = m_activeSpec;
        }
        return true;
    }

    emscripten::val AudioContext = emscripten::val::global("AudioContext");
    if (!AudioContext.as<bool>()) {
        AudioContext = emscripten::val::global("webkitAudioContext");
    }
    if (!AudioContext.as<bool>()) {
        LOGE() << "Web Audio is not available";
        return false;
    }

    emscripten::val options = emscripten::val::object();
    if (spec.sampleRate > 0) {
        options.set("sampleRate", spec.sampleRate);
    }
    web::context = AudioContext.new_(options);

    m_activeSpec = spec;
    m_activeSpec.format = Format::AudioF32;
    m_activeSpec.sampleRate = web::context["sampleRate"].as<unsigned int>();

    web::processor = web::context.call<emscripten::val>("createScriptProcessor", m_activeSpec.samples, 0, m_activeSpec.channels);
    if (!web::processor.as<bool>()) {
        LOGE() << "can't create audio script processor";
        web::context.call<emscripten::val>("close");
        web::context = emscripten::val::undefined();
        return false;
    }

    web::spec = &m_activeSpec;
    web::processor.set("onaudioprocess", emscripten::val::module_property("audioCallback"));
    web::processor.call<void>("connect", web::context["destination"]);

    emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, web::resumeOnMouseDown);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, web::resumeOnKeyDown);

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_opened = true;
    return true;
}

void WebAudioDriver::close()
{
    if (!m_opened) {
        return;
    }

    web::spec = nullptr;
    web::processor.set("onaudioprocess", emscripten::val::null());
    web::processor.call<void>("disconnect");
    web::processor = emscripten::val::undefined();

    web::context.call<emscripten::val>("close");
    web::context = emscripten::val::undefined();

    m_opened = false;
}

bool WebAudioDriver::isOpened() const
{
    return m_opened;
}

const WebAudioDriver::Spec& WebAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

bool WebAudioDriver::reopen()
{
    if (!m_opened) {
        return true;
    }

    const Spec spec = m_activeSpec;
    close();
    return open(spec, nullptr);
}

AudioDeviceID WebAudioDriver::outputDevice() const
{
    return web::DEFAULT_DEVICE_ID;
}

bool WebAudioDriver::selectOutputDevice(const AudioDeviceID& id)
{
    //! NOTE The browser routes the AudioContext to its own output device
    return id == web::DEFAULT_DEVICE_ID;
}

bool WebAudioDriver::resetToDefaultOutputDevice()
{
    return true;
}

async::Notification WebAudioDriver::outputDeviceChanged() const
{
    return m_outputDeviceChanged;
}

AudioDeviceList WebAudioDriver::availableOutputDevices() const
{
    return { AudioDevice { web::DEFAULT_DEVICE_ID, "Browser default" } };
}

async::Notification WebAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}

unsigned int WebAudioDriver::outputDeviceBufferSize() const
{
    return m_opened ? m_activeSpec.samples : 0;
}

bool WebAudioDriver::setOutputDeviceBufferSize(unsigned int bufferSize)
{
    if (m_activeSpec.samples == bufferSize) {
        return true;
    }

    const std::vector<unsigned int> sizes = availableOutputDeviceBufferSizes();
    if (std::find(sizes.begin(), sizes.end(), bufferSize) == sizes.end()) {
        LOGE() << "unsupported buffer size: " << bufferSize;
        return false;
    }

    m_activeSpec.samples = static_cast<uint16_t>(bufferSize);
    if (!reopen()) {
        return false;
    }

    m_bufferSizeChanged.notify();
    return true;
}

async::Notification WebAudioDriver::outputDeviceBufferSizeChanged() const
{
    return m_bufferSizeChanged;
}

std::vector<unsigned int> WebAudioDriver::availableOutputDeviceBufferSizes() const
{
    //! NOTE createScriptProcessor() accepts powers of two from 256 to 16384;
    //! the worker's AudioBuffer holds less than 8192 frames per channel
    return { 1024, 2048, 4096 };
}

unsigned int WebAudioDriver::outputDeviceSampleRate() const
{
    return m_opened ? m_activeSpec.sampleRate : 0;
}

bool WebAudioDriver::setOutputDeviceSampleRate(unsigned int sampleRate)
{
    if (m_activeSpec.sampleRate == sampleRate) {
        return true;
    }

    m_activeSpec.sampleRate = sampleRate;
    if (!reopen()) {
        return false;
    }

    m_sampleRateChanged.notify();
    return true;
}

async::Notification WebAudioDriver::outputDeviceSampleRateChanged() const
{
    return m_sampleRateChanged;
}

std::vector<unsigned int> WebAudioDriver::availableOutputDeviceSampleRates() const
{
    return { 44100, 48000 };
}

void WebAudioDriver::resume()
{
    if (!web::context.isUndefined()) {
        web::context.call<emscripten::val>("resume");
    }
}

void WebAudioDriver::suspend()
{
    if (!web::context.isUndefined()) {
        web::context.call<emscripten::val>("suspend");
    }
}
