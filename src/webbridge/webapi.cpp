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
#include "webapi.h"

#ifdef Q_OS_WASM
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "global/io/file.h"
#include "global/io/buffer.h"

#include "log.h"

using namespace muse;
using namespace mu::webbridge;

static const io::path_t TEMP_DIR = "/mu/temp/";

#ifdef Q_OS_WASM
static void callJsWithBytes(const char* fnname, const uint8_t* data, size_t size)
{
    emscripten::val fn = emscripten::val::module_property(fnname);
    if (fn.isUndefined()) {
        LOGW() << "Module." << fnname << " is not set";
        return;
    }

    emscripten::val jsArray = emscripten::val::global("Uint8Array").new_(
        emscripten::typed_memory_view(size, data)
        );

    fn(jsArray);
}

static void callJsWithText(const char* fnname, const std::string& text)
{
    emscripten::val fn = emscripten::val::module_property(fnname);
    if (fn.isUndefined()) {
        LOGW() << "Module." << fnname << " is not set";
        return;
    }

    fn(text);
}

#else
static void callJsWithBytes(const char*, const uint8_t*, size_t)
{
    NOT_SUPPORTED;
}

static void callJsWithText(const char*, const std::string&)
{
    NOT_SUPPORTED;
}

#endif

WebApi* WebApi::instance()
{
    static WebApi a;
    return &a;
}

void WebApi::init()
{
    auto onProjectChanged = [this]() {
        if (m_currentProject) {
            m_currentProject->saveComplited().resetOnReceive(this);
        }

        m_currentProject = globalContext()->currentProject();

        if (m_currentProject) {
            m_currentProject->saveComplited().onReceive(this, [this](const muse::io::path_t& path, project::SaveMode mode) {
                onProjectSaved(path, mode);
            });
        }
    };

    globalContext()->currentProjectChanged().onNotify(this, onProjectChanged);

    onProjectChanged();
}

void WebApi::deinit()
{
    if (m_currentProject) {
        m_currentProject->saveComplited().resetOnReceive(this);
    }
}

void WebApi::onclickTest1(int num)
{
    LOGI() << "num: " << num;
    interactive()->info("onclickTest1", "Hey!");
}

void WebApi::load(const void* source, unsigned int len)
{
    loadFile("current.mscz", source, len);
}

void WebApi::loadFile(const std::string& fileName, const void* source, unsigned int len)
{
    LOGI() << fileName << ", len: " << len;

    //! NOTE Keep only the file name, so a caller-supplied path cannot leave the temp directory
    const io::path_t name = io::filename(io::path_t(fileName));
    if (name.empty() || io::suffix(name).empty()) {
        LOGE() << "file name must have a suffix that selects the reader: " << fileName;
        return;
    }

    const io::path_t tempFilePath = TEMP_DIR + name;

    //! NOTE Remove the previous file, which may have had another name
    if (!m_lastLoadedPath.empty()) {
        io::File::remove(m_lastLoadedPath);
    }
    io::File::remove(tempFilePath);

    ByteArray data = ByteArray::fromRawData(reinterpret_cast<const char*>(source), len);
    Ret ret = io::File::writeFile(tempFilePath, data);
    if (!ret) {
        LOGE() << "failed write file, path: " << tempFilePath << ", err: " << ret.toString();
        return;
    }
    m_lastLoadedPath = tempFilePath;

    dispatcher()->dispatch("file-open", actions::ActionData::make_arg1(QUrl::fromLocalFile(tempFilePath.toQString())));
}

void WebApi::exportScore(const std::string& suffix)
{
    notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        callJsWithText("onScoreExportFailed", "no score is open");
        return;
    }

    project::INotationWriterPtr writer = writers()->writer(suffix);
    if (!writer) {
        callJsWithText("onScoreExportFailed", "unsupported export format: " + suffix);
        return;
    }

    io::Buffer buffer;
    buffer.open(io::IODevice::WriteOnly);
    Ret ret = writer->write(notation, buffer);
    buffer.close();

    if (!ret) {
        callJsWithText("onScoreExportFailed", "export failed: " + ret.toString());
        return;
    }

    const ByteArray& data = buffer.data();
    callJsWithBytes("onScoreExported", data.constData(), data.size());
}

void WebApi::onProjectSaved(const muse::io::path_t& path, mu::project::SaveMode)
{
    IF_ASSERT_FAILED(io::File::exists(path)) {
        LOGE() << "file does not exist, path: " << path;
        return;
    }

    ByteArray data;
    Ret ret  = io::File::readFile(path, data);
    if (!ret) {
        LOGE() << "failed read file, path: " << path;
        return;
    }

    callJsWithBytes("onProjectSaved", data.constData(), data.size());
}
