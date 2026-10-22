import qtLoad from "./qtloader.js";

function setupInternalCallbacks(Module) {

    // Interactive
    Module.openFileDialog = function(callback) {
        console.log("[js] openFileDialog")
        const input = document.createElement('input');
        input.type = 'file';
        input.onchange = (e) => {
            const file = e.target.files[0];
            const fileName = file.name
            const reader = new FileReader();
            reader.onload = (e) => {
                const contents = e.target.result;
                const uint8View = new Uint8Array(contents);
                console.log("[js] openFileDialog fileName: ", fileName, ", contents: ", uint8View.length, ", [0]=", uint8View[0])
                callback(fileName, uint8View);
            };
            reader.readAsArrayBuffer(file); 
        };
        input.click();
    }
}

const MuImpl = {

    loadModule: async function(config) {
        const instance = await qtLoad({
            qt: {
                onLoaded: config.onLoaded,
                onExit: config.onExit,
                entryFunction: window.MuseScoreStudio_entry, // from MuseScoreStudio.js
                containerElements: [config.screen],
                // JSON files listing {source, destination} pairs copied into the
                // in-memory file system before startup, e.g. the playback soundfont
                preload: config.preload ?? [],
            }
        });

        setupInternalCallbacks(instance)

        return instance;
    },

    loadScoreFile: async function(file) {
        if (!file) {
            return
        }

        const buffer = await file.arrayBuffer();
        this.loadScoreData(new Uint8Array(buffer), file.name)
    },

    // The file name's suffix selects the reader: mscz, mscx, musicxml, mxl, xml...
    loadScoreData: function(data, fileName = "current.mscz") {
        // Allocate through the API's own exports: _malloc is not exported in threaded builds
        const ptr = this.Module._webapiAlloc(data.length);
        try {
            // Read HEAPU8 after allocating, since the allocation may grow the memory
            this.Module.HEAPU8.set(data, ptr);
            this.Module.ccall("loadFile", null, ["string", "number", "number"], [fileName, ptr, data.length]);
        } finally {
            this.Module._webapiFree(ptr);
        }
    },

    // Resolves with the current score encoded in the given format (e.g. "musicxml", "mxl", "mscz")
    exportScore: function(format) {
        const Module = this.Module;
        return new Promise((resolve, reject) => {
            const done = () => {
                delete Module.onScoreExported;
                delete Module.onScoreExportFailed;
            };
            Module.onScoreExported = (data) => { done(); resolve(data); };
            Module.onScoreExportFailed = (message) => { done(); reject(new Error(message)); };
            Module.ccall("exportScore", null, ["string"], [format]);
        });
    },

    // Runs an application action by its code, e.g. "play", "stop", "rewind"
    dispatch: function(actionCode) {
        this.Module.ccall("dispatch", null, ["string"], [actionCode]);
    },
}

export default MuImpl;
