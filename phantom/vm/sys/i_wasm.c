/**
 *
 * Phantom OS
 *
 * by Kirill Samburskiy, 2024
 * 
 * WebAssembly runtime support
 *
**/


#define DEBUG_MSG_PREFIX "vm.dir"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 1
#define debug_level_info 0

#include <vm/object.h>
#include <vm/syscall_tools.h>
#include <vm/root.h>
#include <vm/internal.h>
#include <vm/alloc.h>

#include <wasm_export.h>
#include <wasm_runtime.h>

#define debug_print 0


// presumably void return type cannot be identified from `wasm_type`
pvm_object_t wasm_extract_return_value(uint32 buf[], uint8 wasm_type, int cell_count) {
    if (cell_count == 0) return pvm_create_null_object();

    switch (wasm_type) {
        case VALUE_TYPE_I32: return pvm_create_int_object(*((int*)buf));
        case VALUE_TYPE_F32: return pvm_create_float_object(*((float*)buf));
        case VALUE_TYPE_I64: {
            int64_t value;
            ph_memcpy(&value, &buf[0], sizeof(value));
            return pvm_create_long_object(value);
        }
        case VALUE_TYPE_F64: {
            double value;
            ph_memcpy(&value, &buf[0], sizeof(value));
            return pvm_create_double_object(value);
        }
    }

    return NULL;
}

void initialize_wasm(pvm_object_t o) {
    struct data_area_4_wasm      *da = (struct data_area_4_wasm *)o->da;

    da->runtime_initialized = wasm_runtime_init();

    pvm_add_object_to_restart_list( o );
}

void pvm_internal_init_wasm(pvm_object_t o)
{
    struct data_area_4_wasm      *da = (struct data_area_4_wasm *)o->da;

    da->in_progress = false;
    initialize_wasm(o);
}

// .internal.object  call(var module : string, var funcname : string, var args : .internal.object[])
static int si_invoke_wasm_wasm_8( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_wasm *wasm_da = pvm_data_area( me, wasm );

    if (!wasm_da->runtime_initialized) SYSCALL_THROW_STRING("Wasm runtime initialization failed");

    CHECK_PARAM_COUNT(3);

    pvm_object_t code_obj = args[0];
    pvm_object_t func_name_obj = args[1];
    pvm_object_t wasm_args_obj = args[2];

    if (wasm_args_obj->_class != pvm_get_array_class()) SYSCALL_THROW_STRING( "`args` should be an array" );

    size_t code_length = pvm_get_str_len( code_obj );
    size_t func_name_len = pvm_get_str_len( func_name_obj );
    if( func_name_len > 256 ) SYSCALL_THROW_STRING( "Function name too long" );

    // phantom strings are not null-terminated, so need to copy
    char func_name_cstr[func_name_len + 1]; ph_strlcpy( func_name_cstr, pvm_get_str_data(func_name_obj), func_name_len + 1 );
    // wasm_runtime_load() modifies the buffer for some reason, so need to copy first.
    // the code is too large to put on the stack, so using malloc
    char* code_text = ph_malloc(code_length); if (!code_text) SYSCALL_THROW_STRING( "Out of memory" );
    ph_memcpy(code_text, pvm_get_str_data(code_obj), code_length);

    size_t wasm_args_count = pvm_get_array_size(wasm_args_obj);
    // VLA magic does not work well with goto's, so defining it before any of them
    uint32 argv[wasm_args_count > 0 ? wasm_args_count * 2 : 2]; // 2 cells for return value

    // ##########################################################################################
    // ##########################################################################################
    // ##########################################################################################

    char error_buf[128];

// be careful with this, especially with error_buf
#define FAIL_SYSCALL(...) do { ph_snprintf(error_buf, sizeof(error_buf), __VA_ARGS__); goto destroy; } while (0)

    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    pvm_object_t return_value = NULL;
    uint32 stack_size = 8092, heap_size = 8092;

    // 03. Parse WASM file from buffer and create a WASM module
    wasm_module_t module = wasm_runtime_load(code_text, code_length, error_buf, sizeof(error_buf));
    if (!module) FAIL_SYSCALL("%s [LOAD]", error_buf);

    // 04. Create an instance of WASM module (WASM linear memory is ready)
    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));

    if (!module_inst) FAIL_SYSCALL("%s [INSTANTIATE]", error_buf);

    // 05. Create executation environment to execute WASM function
    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    if (!exec_env) FAIL_SYSCALL("Create wasm execution environment failed");

    // 06. Lookup WASM function by name
    WASMFunctionInstance* func = wasm_runtime_lookup_function(module_inst, func_name_cstr, NULL);
    if (!func) FAIL_SYSCALL("Failed to find wasm function `%s`", func_name_cstr);

    int required_param_count = func->param_count;
    
    if (wasm_args_count != required_param_count) 
        FAIL_SYSCALL("Expected %d, received %d parameters", required_param_count, wasm_args_count);

    for (size_t i = 0, argv_pos = 0; i < wasm_args_count; i++) {
        pvm_object_t item = pvm_get_array_ofield(wasm_args_obj, i);

        if (item->_class == pvm_get_int_class()) {
            if (func->param_types[i] != VALUE_TYPE_I32) FAIL_SYSCALL("Parameter %d: Unexpected int argument", i + 1);
            int value = pvm_get_int(item);
            ph_memcpy(&argv[argv_pos], &value, sizeof(value));
            argv_pos += sizeof(value) / sizeof(uint32);
        } else if (item->_class == pvm_get_float_class()) {
            if (func->param_types[i] != VALUE_TYPE_F32) FAIL_SYSCALL("Parameter %d: Unexpected float argument", i + 1);
            float value = pvm_get_float(item);
            ph_memcpy(&argv[argv_pos], &value, sizeof(value));
            argv_pos += sizeof(value) / sizeof(uint32);
        } else if (item->_class == pvm_get_long_class()) {
            if (func->param_types[i] != VALUE_TYPE_I64) FAIL_SYSCALL("Parameter %d: Unexpected long argument", i + 1);
            int64_t value = pvm_get_long(item);
            ph_memcpy(&argv[argv_pos], &value, sizeof(value));
            argv_pos += sizeof(value) / sizeof(uint32);
        } else if (item->_class == pvm_get_double_class()) {
            if (func->param_types[i] != VALUE_TYPE_F64) FAIL_SYSCALL("Parameter %d: Unexpected double argument", i + 1);
            double value = pvm_get_double(item);
            ph_memcpy(&argv[argv_pos], &value, sizeof(value));
            argv_pos += sizeof(value) / sizeof(uint32);
        } else FAIL_SYSCALL("Parameter %d: unsupported type", i + 1);
    }

    // for (size_t i = 0, argv_pos = 0; i < wasm_args_count; i++) {
    //     pvm_object_t item = pvm_get_array_ofield(wasm_args_obj, i);
// 
    //     switch (func->param_types[i]) {
    //         case VALUE_TYPE_I32: {
    //             if (item->_class != pvm_get_int_class()) FAIL_SYSCALL("Parameter %d: expected int argument", i + 1);
    //             ph_memcpy(&argv[argv_pos], &pvm_get_int(item), sizeof(int));
    //             argv_pos += sizeof(int) / sizeof(uint32);
    //             break;
    //         }
    //         case VALUE_TYPE_F32:
    //             break;
    //         case VALUE_TYPE_I64:
    //             break;
    //         case VALUE_TYPE_F64:
    //             break;
    //         default:
    //             FAIL_SYSCALL("Parameter %d: unknown wasm type", i + 1);
    //     }
// 
// 
// 
    //     if (item->_class == pvm_get_int_class()) {
    //         if (func->param_types[i] != VALUE_TYPE_I32) FAIL_SYSCALL("Parameter %d: Unexpected int argument", i + 1);
    //         int value = pvm_get_int(item);
    //         ph_memcpy(&argv[argv_pos], &value, sizeof(value));
    //         argv_pos += sizeof(value) / sizeof(uint32);
    //     } else if (item->_class == pvm_get_float_class()) {
    //         if (func->param_types[i] != VALUE_TYPE_F32) FAIL_SYSCALL("Parameter %d: Unexpected float argument", i + 1);
    //         float value = pvm_get_float(item);
    //         ph_memcpy(&argv[argv_pos], &value, sizeof(value));
    //         argv_pos += sizeof(value) / sizeof(uint32);
    //     } else if (item->_class == pvm_get_long_class()) {
    //         if (func->param_types[i] != VALUE_TYPE_I64) FAIL_SYSCALL("Parameter %d: Unexpected long argument", i + 1);
    //         int64_t value = pvm_get_long(item);
    //         ph_memcpy(&argv[argv_pos], &value, sizeof(value));
    //         argv_pos += sizeof(value) / sizeof(uint32);
    //     } else if (item->_class == pvm_get_double_class()) {
    //         if (func->param_types[i] != VALUE_TYPE_F64) FAIL_SYSCALL("Parameter %d: Unexpected double argument", i + 1);
    //         double value = pvm_get_double(item);
    //         ph_memcpy(&argv[argv_pos], &value, sizeof(value));
    //         argv_pos += sizeof(value) / sizeof(uint32);
    //     } else FAIL_SYSCALL("Parameter %d: unsupported type", i + 1);
    // }

    wasm_da->in_progress = true;

    // 07. Function call with parameters in an array of 32 bits elements and size
    if (!wasm_runtime_call_wasm(exec_env, func, func->param_cell_num, argv))
        FAIL_SYSCALL("Call wasm function main failed. %s\n", wasm_runtime_get_exception(module_inst));

    return_value = wasm_extract_return_value(argv, func->param_types[required_param_count], func->ret_cell_num);
    if (!return_value) FAIL_SYSCALL("Unexpected return type");

    SYS_FREE_O(code_obj);
    SYS_FREE_O(func_name_obj);
    SYS_FREE_O(wasm_args_obj);

#undef FAIL_SYSCALL

destroy:
    wasm_da->in_progress = false;

    ph_free(code_text);
    if (exec_env) wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst) wasm_runtime_deinstantiate(module_inst);
    if (module) wasm_runtime_unload(module);
    
    if (return_value) SYSCALL_RETURN(return_value);
    
    SYSCALL_THROW_STRING(error_buf);
}

void pvm_gc_iter_wasm(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_wasm      *da = (struct data_area_4_wasm *)os->da;

    (void)da;

    // gc_fcall( func, arg, da->keys );
    // gc_fcall( func, arg, da->values );
}

syscall_func_t  syscall_table_4_wasm[16] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &invalid_syscall,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &si_invoke_wasm_wasm_8,             &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall
};

DECLARE_SIZE(wasm); // create variable holding size of syscall table

void pvm_gc_finalizer_wasm( pvm_object_t  o )
{
    (void) o;
    wasm_runtime_destroy();
    //struct data_area_4_window      *da = (struct data_area_4_window *)os->da;
    //assert(0);
}

void pvm_restart_wasm(pvm_object_t o)
{
    // XXX: do we reinitialize runtime if `runtime_initalized` is false?
    initialize_wasm(o);

    struct data_area_4_wasm *da = (struct data_area_4_wasm *)o->da;

    if (da->in_progress) {
        ph_printf("WARNING: WASM program execution was in progress!\n");
    }
}
