#ifndef _I_WASM
#define _I_WASM



struct data_area_4_wasm
{
    // has wasm runtime been initialized successfully ?
    bool         runtime_initialized;
    // is a wasm program running right now? (checked on restart)
    bool         in_progress;
};



#endif // _I_WASM
