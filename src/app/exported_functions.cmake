
# Runtime entry points only. The web API functions (src/app/webapi_export.cpp)
# export themselves with EMSCRIPTEN_KEEPALIVE, because Qt's threaded builds
# replace this option with their own list when the target is finalized.
set(EXPORTED_FUNCTIONS "SHELL:-s EXPORTED_FUNCTIONS=[_main,_malloc,_free]")
