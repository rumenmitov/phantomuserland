/**
 *
 * Phantom OS
 *
 * by Kirill Samburskiy, 2024
 * 
 * WebAssembly runtime support
 *
**/

#include <wasm_export.h>
#include <wasm_runtime.h>
#undef LOG_ERROR

 // we don't need wasm's LOG_ERROR here, it is redefined by debug_ext.h
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
#include <kernel/snap_sync.h>

#define debug_print 0

static const u_int32_t stack_size = 8092, heap_size = 8092;
// XXX fix this :((
struct data_area_4_wasm *global_wasm;

// presumably void return type cannot be identified from `wasm_type`
static pvm_object_t wasm_extract_return_value(uint32 buf[], uint8 wasm_type, int cell_count) {
    if (cell_count == 0) return pvm_create_null_object();
    if (cell_count > 2) return NULL; // ?? idk what is that

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

static void initialize_wasm(pvm_object_t o) {
    struct data_area_4_wasm      *da = (struct data_area_4_wasm *)o->da;

    global_wasm = da;
    da->runtime_initialized = wasm_runtime_init();

    pvm_add_object_to_restart_list( o );
}

void pvm_internal_init_wasm(pvm_object_t o)
{
    struct data_area_4_wasm      *da = (struct data_area_4_wasm *)o->da;

    da->wasm_runtime_objects_array = pvm_create_array_object();
    da->wasm_instance_objects_array = pvm_create_array_object();
    da->wasm_code_str = NULL;
    da->module = NULL;
    da->module_instance = NULL;
    da->function_instance = NULL;
    da->exec_env = NULL;
    
    initialize_wasm(o);
}

static void wasm_destroy_instance(struct data_area_4_wasm *da) {
    // I am not sure as to whether we need this complicated destruction when we can just 
    // delete all the objects from the wasm_instance_objects_array
    if (da->exec_env) wasm_runtime_destroy_exec_env(da->exec_env);
    if (da->module_instance) wasm_runtime_deinstantiate(da->module_instance);
    if (da->module) wasm_runtime_unload(da->module);

    ref_dec_o(da->wasm_code_str);

    da->wasm_code_str = NULL;
    da->exec_env = NULL;
    da->module_instance = NULL;
    da->module = NULL;
    da->function_instance = NULL;
}

// void loadModule(.internal.string module)
static int si_load_module_wasm_8( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_wasm *wasm_da = pvm_data_area( me, wasm );

    if (!wasm_da->runtime_initialized) SYSCALL_THROW_STRING("Wasm runtime initialization failed");

    CHECK_PARAM_COUNT(1);
    if (wasm_da->function_instance) SYSCALL_THROW_STRING("Loading module while executing! ( how?? )");
    wasm_destroy_instance(wasm_da); // Unlodad previous module (if any)

    pvm_object_t code_obj = args[0];
    size_t code_length = pvm_get_str_len( code_obj );

    // WAMR modifies code (it shouldn't really but still) when loading it, so we create a copy to avoid
    // original string corruption 
    wasm_da->wasm_code_str = pvm_create_string_object_binary(pvm_get_str_data(code_obj), code_length);
    char* code_text = pvm_get_str_data(wasm_da->wasm_code_str);

    char error_buf[128];

    // keep in mind that this uses error_buf to form string
    #define FAIL_SYSCALL(...) do { ph_snprintf(error_buf, sizeof(error_buf), __VA_ARGS__); goto fail; } while (0)

    // Parse WASM file and create a WASM module
    wasm_da->module = wasm_runtime_load(code_text, code_length, error_buf, sizeof(error_buf));
    if (!wasm_da->module) FAIL_SYSCALL("%s [LOAD]", error_buf);

    // Create an instance of WASM module 
    wasm_da->module_instance = wasm_runtime_instantiate(wasm_da->module, stack_size, heap_size, error_buf, sizeof(error_buf));
    if (!wasm_da->module_instance) FAIL_SYSCALL("%s [INSTANTIATE]", error_buf);

    // Create execution environment for WASM module instance
    wasm_da->exec_env = wasm_runtime_create_exec_env(wasm_da->module_instance, stack_size);
    if (!wasm_da->exec_env) FAIL_SYSCALL("Create wasm execution environment failed");

    SYS_FREE_O(code_obj);
    SYSCALL_RETURN_NOTHING;

    #undef FAIL_SYSCALL
fail:
    wasm_destroy_instance(wasm_da);
    
    SYSCALL_THROW_STRING(error_buf);
}

// .internal.object  invokeWasm(var funcname : string, var args : .internal.object[])
static int si_invoke_wasm_wasm_9( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_wasm *wasm_da = pvm_data_area( me, wasm );
    // XXX are we SURE this is WASMFunctionInstance ???
    WASMFunctionInstance* func = wasm_da->function_instance;
    pvm_object_t return_value = NULL;
    // If param_cell_num wil be passed as -1, it will trigger a restart from snapshot
    int param_cell_num = -1;

    if (!wasm_da->runtime_initialized) SYSCALL_THROW_STRING("Wasm runtime initialization failed");
    if (!wasm_da->exec_env) SYSCALL_THROW_STRING("No Wasm module loaded");

    CHECK_PARAM_COUNT(2);

    pvm_object_t func_name_obj = args[0];
    pvm_object_t wasm_args_obj = args[1];

    if (wasm_args_obj->_class != pvm_get_array_class()) SYSCALL_THROW_STRING( "`args` should be an array" );

    size_t wasm_args_count = pvm_get_array_size(wasm_args_obj);
    // VLA magic does not work well with goto's, so defining it before any of them
    uint32 argv[wasm_args_count > 0 ? wasm_args_count * 2 : 2]; // 2 cells for return value

    char error_buf[128];
    // keep in mind that this uses error_buf to form strings
    #define FAIL_SYSCALL(...) do { ph_snprintf(error_buf, sizeof(error_buf), __VA_ARGS__); goto destroy; } while (0)

    if (!func) { // if func is NULL, means we are running this syscall the first time (not restart)
        size_t func_name_len = pvm_get_str_len(func_name_obj);
        if( func_name_len > 256 ) SYSCALL_THROW_STRING( "Function name too long" );
        // phantom strings are not null-terminated, so we need a copy
        char func_name_cstr[func_name_len + 1]; ph_strlcpy( func_name_cstr, pvm_get_str_data(func_name_obj), func_name_len + 1 );

        func = wasm_runtime_lookup_function(wasm_da->module_instance, func_name_cstr, NULL);
        if (!func) FAIL_SYSCALL("Failed to find wasm function `%s`", func_name_cstr);

        if (wasm_args_count != func->param_count) 
            FAIL_SYSCALL("Expected %d, received %d parameters", func->param_count, wasm_args_count);

        for (size_t i = 0, argv_pos = 0; i < wasm_args_count; i++) {
            #define GET_ARG(wasm_type, ph_name, c_type)                                 \
                case wasm_type: {                                                       \
                    if (item->_class != pvm_get_##ph_name##_class())                    \
                        FAIL_SYSCALL("Parameter %d: expected int argument", i + 1);     \
                    c_type value = pvm_get_##ph_name(item);                             \
                    ph_memcpy(&argv[argv_pos], &value, sizeof(c_type));                 \
                    argv_pos += sizeof(c_type) / sizeof(uint32);                        \
                    break;                                                              \
                }

            pvm_object_t item = pvm_get_array_ofield(wasm_args_obj, i);

            switch (func->param_types[i]) {
                GET_ARG(VALUE_TYPE_I32, int, int);
                GET_ARG(VALUE_TYPE_F32, float, float);
                GET_ARG(VALUE_TYPE_I64, long, int64_t);
                GET_ARG(VALUE_TYPE_F64, double, double);
                default: FAIL_SYSCALL("Parameter %d: unknown wasm type", i + 1);
            }

            #undef GET_ARG
        }

        param_cell_num = func->param_cell_num;
        wasm_da->function_instance = func;
    }

    // Actual execution happens here. This function will sleep thread whenever snapshot flag is set
    if (!wasm_runtime_call_wasm(wasm_da->exec_env, func, param_cell_num, argv))
        FAIL_SYSCALL("Wasm function call failed: %s\n", wasm_runtime_get_exception(wasm_da->module_instance));

    return_value = wasm_extract_return_value(argv, func->param_types[func->param_count], func->ret_cell_num);
    if (!return_value) FAIL_SYSCALL("Unexpected return type");

    SYS_FREE_O(func_name_obj);
    SYS_FREE_O(wasm_args_obj);

#undef FAIL_SYSCALL
destroy:
    wasm_da->function_instance = NULL;

    if (return_value) SYSCALL_RETURN(return_value);
    
    SYSCALL_THROW_STRING(error_buf);
}

void wamr_alloc_callback(pvm_object_t obj) {
    pvm_append_array(
        global_wasm->runtime_initialized 
            ? global_wasm->wasm_instance_objects_array 
            : global_wasm->wasm_runtime_objects_array,
        obj
    );
}

void wamr_free_callback(pvm_object_t obj) {
    pvm_pop_array(global_wasm->wasm_instance_objects_array, obj);
}

void pvm_gc_iter_wasm(gc_iterator_call_t func, pvm_object_t self, void *arg)
{
    struct data_area_4_wasm *da = (struct data_area_4_wasm *)self->da;

    func(da->wasm_runtime_objects_array, arg);
    func(da->wasm_instance_objects_array, arg);
    func(da->wasm_code_str, arg);
}

syscall_func_t  syscall_table_4_wasm[16] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &invalid_syscall,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &si_load_module_wasm_8,             &si_invoke_wasm_wasm_9,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall
};

DECLARE_SIZE(wasm); // create variable holding size of syscall table

// XXX : wasm_runtime_init is run even on restart. it allocates already allocated objects
void pvm_restart_wasm(pvm_object_t o)
{
    // XXX: do we reinitialize runtime if `runtime_initalized` is false?
    initialize_wasm(o);
}
