#ifndef _I_WASM
#define _I_WASM

#include <wasm_export.h>

struct data_area_4_wasm
{
    // array (currently stores objects with native symbols info) 
    pvm_object_t wasm_global_objects_array;
    // array (instance objects)
    pvm_object_t wasm_instance_objects_array;

    wasm_module_t module;
    wasm_module_inst_t module_instance;
    wasm_exec_env_t exec_env;
    wasm_function_inst_t function_instance;
    void* natives_list_ptr;

    pvm_object_t wasm_code_str;
    
    // array of strings (environment variables)
    pvm_object_t env_vars_array;

    // arrays of hal mutexes / conds created by WAMR
    pvm_object_t wasm_mutex_array;
    pvm_object_t wasm_cond_array;
};

#endif // _I_WASM
