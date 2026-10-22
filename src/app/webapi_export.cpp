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

#include <cstdlib>

#include <emscripten/emscripten.h>

#include "log.h"

#include "webbridge/webapi.h"

using namespace mu::webbridge;

//! NOTE EMSCRIPTEN_KEEPALIVE exports each function as Module._<name>. Qt's
//! threaded builds replace -sEXPORTED_FUNCTIONS with their own list when the
//! target is finalized, so the API must not depend on that link option.
extern "C" {
EMSCRIPTEN_KEEPALIVE void* webapiAlloc(unsigned int size) { return std::malloc(size); }
EMSCRIPTEN_KEEPALIVE void webapiFree(void* ptr) { std::free(ptr); }

EMSCRIPTEN_KEEPALIVE void onclickTest1(int num) { WebApi::instance()->onclickTest1(num); }
EMSCRIPTEN_KEEPALIVE void load(const void* source, unsigned int len) { WebApi::instance()->load(source, len); }
EMSCRIPTEN_KEEPALIVE void loadFile(const char* fileName, const void* source, unsigned int len)
{
    WebApi::instance()->loadFile(fileName, source, len);
}

EMSCRIPTEN_KEEPALIVE void exportScore(const char* suffix) { WebApi::instance()->exportScore(suffix); }
EMSCRIPTEN_KEEPALIVE void dispatch(const char* actionCode) { WebApi::instance()->dispatch(actionCode); }
}
