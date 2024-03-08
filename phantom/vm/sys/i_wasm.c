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
#include <wasm_memory.h>
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
#include <ph_malloc.h>

#define debug_print 0

// Release assert - works both in debug and release builds
#define r_assert(e)     ((e) ? (void)0 : panic( __FILE__ ":%u, %s: assertion '" #e "' failed" , __LINE__, __func__ ))

// ev_assert - always evaluates but only panics in debug
#ifdef NDEBUG
#define	ev_assert(e)	(e)
#else
#define	ev_assert(e)    r_assert(e)
#endif

typedef struct data_area_4_wasm *pvm_wasm_da_t;

static const u_int32_t stack_size = 8092, heap_size = 8092;

// for wamr malloc / free / mutex / cond callbacks (rework?)
static pvm_wasm_da_t master_instance = NULL;
// We only need mutex for first initialization, so might as well use someone else's
extern hal_mutex_t  *vm_alloc_mutex;

// To use in syscalls. Throws exception if wasm not initialized
#define CHECK_WASM_INITIALIZED() do {                                                           \
    if (master_instance == NULL) SYSCALL_THROW_STRING("Wasm runtime initialization failed");    \
    assert(pvm_object_is_allocated(pvm_da_to_object(master_instance)));                         \
    assert(pvm_da_to_object(master_instance)->_class == pvm_get_wasm_class());                  \
    } while (0)

// ######################################################
// ##########     UNMANAGED ARRAY FUNCTIONS    ##########
// ######################################################

/* unmanaged array: can store pointers, but does not report them to gc */


static void dump_uarray(pvm_object_t uarray, const char* title) __attribute__((__unused__));
static void dump_uarray(pvm_object_t uarray, const char* title)
{
    ph_printf("%s: [", title);
    u_int32_t *ptr = (u_int32_t*)pvm_get_str_data(uarray);
    for (int i = 0; i < pvm_get_str_len(uarray) / sizeof(u_int32_t); i++) 
        ph_printf("%08x", ptr[i]);
    ph_printf("] [%d]\n", pvm_get_str_len(uarray));
}

static pvm_object_t create_uarray(int capacity) {
    int capacity_bytes = capacity * sizeof(uintptr_t);
    pvm_object_t array = pvm_create_string_object_binary(NULL, capacity_bytes);
    // creating phantom string from NULL leaves its length at 0, fix ?
    pvm_data_area(array, string)->length = capacity_bytes;
    ph_memset(pvm_get_str_data(array), 0, capacity_bytes);
    return array;
}

static void append_uarray(pvm_object_t *array_ptr, void *value) {
    int capacity_bytes = pvm_get_str_len(*array_ptr);
    int capacity = capacity_bytes / sizeof(uintptr_t);
    uintptr_t *array = (uintptr_t*) pvm_get_str_data(*array_ptr);

    if (array[capacity - 1] != 0) { // grow array
        int new_capacity = capacity * 2;
        int new_bytes = new_capacity * sizeof(uintptr_t);
        pvm_object_t new_array = pvm_create_string_object_binary(NULL, new_bytes);
        pvm_data_area(new_array, string)->length = new_bytes;
        char* new_ptr = (char*) pvm_get_str_data(new_array);
        ph_memcpy(new_ptr, array, capacity_bytes);
        ph_memset(new_ptr + capacity_bytes, 0, new_capacity - capacity_bytes);
        ref_dec_o(*array_ptr);
        *array_ptr = new_array;
        array = (uintptr_t*) new_ptr;
        capacity = new_capacity;
    }

    for (int i = 0; i < capacity; i++) {
        if (array[i] == 0) {
            array[i] = (uintptr_t) value;
            return;
        }
    }

    SHOW_ERROR0(1, "append_uarray: element not appended");
}

static void pop_uarray(pvm_object_t uarray, void* value) {
    int capacity = pvm_get_str_len(uarray) / sizeof(uintptr_t);
    uintptr_t *array = (uintptr_t*) pvm_get_str_data(uarray);

    for (int i = 0; i < capacity; i++) {
        if (array[i] == (uintptr_t) value) {
            array[i] = array[capacity - 1];
            array[capacity - 1] = 0;
            return;
        }
    }

    SHOW_ERROR0(1, "pop_uarray: element not found");
}

#define foreach_in_uarray(uarray, entry)                                            \
    for (uintptr_t* entry = (uintptr_t*) pvm_get_str_data(uarray);                  \
        (char*) entry - (char*) pvm_get_str_data(uarray) < pvm_get_str_len(uarray); \
        entry++)

// ######################################################
// ################     WASM RELATED    #################
// ######################################################

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

// Initializes WAMR's non-persistent state as well as sets `master_instance`
static bool initialize_wasm(pvm_object_t o) {
    pvm_wasm_da_t da = pvm_data_area(o, wasm);

    hal_mutex_lock(vm_alloc_mutex);
    if ( // if singleton existed but got destroyed, no one clears `master_instance`
        master_instance == NULL                                      ||
        !pvm_object_is_allocated(pvm_da_to_object(master_instance))  ||
        (pvm_da_to_object(master_instance))->_class != pvm_get_wasm_class()
        )
    {
        master_instance = da;
    }
    hal_mutex_unlock(vm_alloc_mutex);
    
    if (da != master_instance) return true; // wasm already initialized, return

    // the following code (roughly) performs functions of `wasm_runtime_init()`
    ev_assert(wasm_runtime_memory_init(Alloc_With_System_Allocator, NULL));
    ev_assert(bh_platform_init() == 0); // TODO: need it? is empty anyways..
    if (da->natives_list_ptr) wasm_set_natives_list(da->natives_list_ptr);
    else {
        // we need this *before* we initialize natives
        da->wasm_global_objects_array = pvm_create_array_object();
        
        // TODO : native symbols are stored as a linked list. 
        //      maybe compact natives in a single pvm object with all nodes?
        if (!wasm_native_init()) { 
            SHOW_ERROR0(0, "Failed to initialize WASM native symbols");

            hal_mutex_lock(vm_alloc_mutex);
            master_instance = NULL;
            hal_mutex_unlock(vm_alloc_mutex);

            ref_dec_o(da->wasm_global_objects_array);
            return false;
        }
        da->natives_list_ptr = wasm_get_natives_list();
        da->wasm_instance_objects_array = pvm_create_array_object();
        da->wasm_mutex_array = create_uarray(8);
        da->wasm_cond_array = create_uarray(8);
    }

    pvm_add_object_to_restart_list(o);

    return true;
}

void pvm_internal_init_wasm(pvm_object_t o) {
    pvm_wasm_da_t da = pvm_data_area(o, wasm);
    ph_memset(da, 0, sizeof(struct data_area_4_wasm));

    if (!initialize_wasm(o)) return;

    // non-master instances rely on master, need to ref it
    if (da != master_instance) ref_inc_o(pvm_da_to_object(master_instance));
    da->env_vars_array = pvm_create_array_object();
}

static void wasm_destroy_instance(pvm_wasm_da_t da) {
    // I am not sure as to whether we need this complicated destruction when we can just 
    // delete all the objects from the wasm_instance_objects_array
    // ...well right now the array is global for all instances, so that's not an option
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
static int si_load_module_wasm_8(pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args)
{
    DEBUG_INFO;
    CHECK_PARAM_COUNT(1);
    CHECK_WASM_INITIALIZED();

    pvm_wasm_da_t wasm_da = pvm_data_area(me, wasm);
    pvm_object_t code_obj = args[0];
    size_t code_length = pvm_get_str_len(code_obj);

    if (wasm_da->function_instance) SYSCALL_THROW_STRING("Loading module while executing! ( how?? )");

    // Unlodad previous module (if any)
    wasm_destroy_instance(wasm_da);

    // WAMR modifies code (it shouldn't really but still) when loading it, so we create a copy to avoid
    // original string corruption 
    wasm_da->wasm_code_str = pvm_create_string_object_binary(pvm_get_str_data(code_obj), code_length);
    char* code_text = pvm_get_str_data(wasm_da->wasm_code_str);

    char error_buf[128];

    // Parse WASM file and create a WASM module
    wasm_da->module = wasm_runtime_load(code_text, code_length, error_buf, sizeof(error_buf));
    if (!wasm_da->module) {
        ph_snprintf(error_buf, sizeof(error_buf), "%s [LOAD]", error_buf);
        goto fail;
    }

    SYS_FREE_O(code_obj);
    SYSCALL_RETURN_NOTHING;

fail:
    wasm_destroy_instance(wasm_da);
    
    SYSCALL_THROW_STRING(error_buf);
}

// Instantiate current wasm module, create exec_env, and lookup function by name
// Previous instance and exec_env are destroyed. Returns true on success
static bool wasm_prepare_function(pvm_wasm_da_t da, const char* func_name, char* error_buf, size_t err_buf_size) {
    // Destroy previous instance & exec_env (if any)
    if (da->exec_env) wasm_runtime_destroy_exec_env(da->exec_env);
    if (da->module_instance) wasm_runtime_deinstantiate(da->module_instance);

    da->module_instance = NULL;
    da->exec_env = NULL;

    // Create an instance of WASM module 
    da->module_instance = wasm_runtime_instantiate(da->module, stack_size, heap_size, error_buf, err_buf_size);
    if (!da->module_instance) {
        ph_snprintf(error_buf, err_buf_size, "%s [INSTANTIATE]", error_buf);
        return false;
    }

    // Create execution environment for WASM module instance
    da->exec_env = wasm_runtime_create_exec_env(da->module_instance, stack_size);
    if (!da->exec_env) {
        ph_snprintf(error_buf, err_buf_size, "Create wasm execution environment failed");
        return false;
    }

    da->function_instance = wasm_runtime_lookup_function(da->module_instance, func_name, NULL);
    if (!da->function_instance) {
        ph_snprintf(error_buf, err_buf_size, "Failed to find wasm function `%s`", func_name);
        return false;
    }

    return true;
}

// .internal.object  invokeWasm(var funcname : string, var args : .internal.object[])
static int si_invoke_wasm_wasm_9(pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args)
{
    DEBUG_INFO;
    CHECK_PARAM_COUNT(2);
    CHECK_WASM_INITIALIZED();

    pvm_wasm_da_t wasm_da = pvm_data_area(me, wasm);
    pvm_object_t func_name_obj = args[0];
    pvm_object_t wasm_args_obj = args[1];

    if (!wasm_da->module) SYSCALL_THROW_STRING("No Wasm module loaded");
    if (wasm_args_obj->_class != pvm_get_array_class()) SYSCALL_THROW_STRING( "`args` should be an array" );

    // XXX are we SURE this is WASMFunctionInstance ???
    WASMFunctionInstance* func = wasm_da->function_instance;
    pvm_object_t return_value = NULL;
    // If param_cell_num wil be passed as -1, it will trigger a restart from snapshot
    int param_cell_num = -1;

    size_t wasm_args_count = pvm_get_array_size(wasm_args_obj);
    // VLA magic does not work well with goto's, so defining it before any of them
    uint32 argv[wasm_args_count > 0 ? wasm_args_count * 2 : 2]; // 2 cells for return value

    char error_buf[128];
    // keep in mind that this uses error_buf to form strings
    #define FAIL_SYSCALL(...) do { ph_snprintf(error_buf, sizeof(error_buf), __VA_ARGS__); goto destroy; } while (0)

    if (!func) { // if func is NULL, means we are running this syscall the first time (not restart)
        size_t func_name_len = pvm_get_str_len(func_name_obj);
        if(func_name_len > 256) SYSCALL_THROW_STRING("Function name too long");
        // phantom strings are not null-terminated, so we need a copy
        char func_name_cstr[func_name_len + 1]; ph_strlcpy(func_name_cstr, pvm_get_str_data(func_name_obj), func_name_len + 1);
        
        if (!wasm_prepare_function(wasm_da, func_name_cstr, error_buf, sizeof(error_buf)))
            goto destroy; // error_buf already set

        func = wasm_da->function_instance;
        if (wasm_args_count != func->param_count) 
            FAIL_SYSCALL("Expected %d, received %d parameters", func->param_count, wasm_args_count);

        for (size_t i = 0, argv_pos = 0; i < wasm_args_count; i++) {
            #define GET_ARG(wasm_type, ph_name, c_type)                                         \
                case wasm_type: {                                                               \
                    if (item->_class != pvm_get_##ph_name##_class())                            \
                        FAIL_SYSCALL("Parameter %d: expected " #ph_name " argument", i + 1);    \
                    c_type value = pvm_get_##ph_name(item);                                     \
                    ph_memcpy(&argv[argv_pos], &value, sizeof(c_type));                         \
                    argv_pos += sizeof(c_type) / sizeof(uint32);                                \
                    break;                                                                      \
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
    }

    // Actual execution happens here. This function will sleep thread whenever snapshot flag is set
    if (!wasm_runtime_call_wasm(wasm_da->exec_env, func, param_cell_num, argv))
        FAIL_SYSCALL("Wasm function call failed: %s", wasm_runtime_get_exception(wasm_da->module_instance));

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

// .internal.int 		invokeWasiStart(var args : .internal.object) [10] { }
static int si_wasi_invoke_start_wasm_10(pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args)
{
    DEBUG_INFO;
    CHECK_PARAM_COUNT(1);
    CHECK_WASM_INITIALIZED();

    pvm_wasm_da_t wasm_da = pvm_data_area(me, wasm);
    pvm_object_t wasi_args_obj = args[0];
    pvm_object_t return_value = NULL;
    // If param_cell_num wil be passed as -1, it will trigger a restart from snapshot
    int param_cell_num = -1;

    if (!wasm_da->module) SYSCALL_THROW_STRING("No Wasm module loaded");
    if (wasi_args_obj->_class != pvm_get_array_class()) SYSCALL_THROW_STRING("`args` should be an array");

    int wasi_argc = pvm_get_array_size(wasi_args_obj);
    char* wasi_argv[wasi_argc];
    ph_memset(wasi_argv, 0, sizeof(char*) * wasi_argc);

    int wasi_envcount = pvm_get_array_size(wasm_da->env_vars_array);
    char* wasi_envs[wasi_envcount];

    char error_buf[128];
    // keep in mind that this uses error_buf to form strings
    #define FAIL_SYSCALL(...) do { ph_snprintf(error_buf, sizeof(error_buf), __VA_ARGS__); goto destroy; } while (0)

    if (!wasm_da->function_instance) { // if NULL, means we are running this syscall the first time (not restart)
        const char *start_func_name = "_start"; // some wasm files may override this (see `start_function` in WASMModule)

        // String in env_vars_array are already zero terminated
        for (size_t i = 0; i < wasi_envcount; i++) {
            wasi_envs[i] = pvm_get_str_data(pvm_get_array_ofield(wasm_da->env_vars_array, i));
        }

        for (size_t i = 0; i < wasi_argc; i++) {
            pvm_object_t item = pvm_get_array_ofield(wasi_args_obj, i);
            if (item->_class != pvm_get_string_class()) FAIL_SYSCALL("Expected string arguments");
            
            size_t length = pvm_get_str_len(item);
            wasi_argv[i] = ph_malloc(length + 1);
            ph_strlcpy(wasi_argv[i], pvm_get_str_data(item), length + 1);
        }

        wasm_runtime_set_wasi_args(wasm_da->module,
            NULL, 0, // dir_list, dir_count
            NULL, 0, // map_dir_list, map_dir_count
            (const char**)wasi_envs, wasi_envcount, wasi_argv, wasi_argc
        );

        if (!wasm_prepare_function(wasm_da, start_func_name, error_buf, sizeof(error_buf)))
            goto destroy; // error_buf already set

        param_cell_num = ((WASMFunctionInstance*)wasm_da->function_instance)->ret_cell_num;
        if (param_cell_num != 0) FAIL_SYSCALL("Unexpected WASI function to return value");
    }

    // Actual execution happens here. This function will sleep thread whenever snapshot flag is set
    if (!wasm_runtime_call_wasm(wasm_da->exec_env, wasm_da->function_instance, param_cell_num, NULL)) {
        const char *exception = wasm_get_exception((WASMModuleInstance*)wasm_da->module_instance);
    
        // thrown if proc_exit is called, and is not a real exception
        const char *wasi_proc_exit = "Exception: wasi proc exit:";
        if (strncmp(exception, wasi_proc_exit, strlen(wasi_proc_exit)) == 0) {
            int32_t exitcode = (int32_t) ph_atol(exception + strlen(wasi_proc_exit));
            return_value = pvm_create_int_object(exitcode);
        } else {
            FAIL_SYSCALL("Wasm function call failed: %s", exception);
        }
    } else {
        return_value = pvm_create_null_object();
    }

    SYS_FREE_O(wasi_args_obj);

#undef FAIL_SYSCALL
destroy:
    wasm_da->function_instance = NULL;

    for (size_t i = 0; i < wasi_argc; i++) ph_free(wasi_argv[i]);

    if (return_value) SYSCALL_RETURN(return_value);
    
    SYSCALL_THROW_STRING(error_buf);
}

// void	wasiSetEnvVariables(var envs : .internal.string[]) [11] { }	
static int si_wasi_set_env_vars_wasm_11(pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args)
{
    DEBUG_INFO;
    CHECK_PARAM_COUNT(1);

    pvm_wasm_da_t wasm_da = pvm_data_area(me, wasm);
    pvm_object_t env_vars_obj = args[0];

    if (env_vars_obj->_class != pvm_get_array_class()) SYSCALL_THROW_STRING("`envs` should be an array");
    
    // delete previous env vars
    while (pvm_get_array_size(wasm_da->env_vars_array) > 0) {
        pvm_object_t obj = pvm_get_array_ofield(wasm_da->env_vars_array, 0);
        pvm_pop_array(wasm_da->env_vars_array, obj);
        ref_dec_o(obj);
    }

    // copy env vars into our internal array
    for (size_t i = 0; i < pvm_get_array_size(env_vars_obj); i++) {
        pvm_object_t item = pvm_get_array_ofield(env_vars_obj, i);
        if (item->_class != pvm_get_string_class()) SYSCALL_THROW_STRING("Expected string arguments");
        
        // create a copy and zero terminate it (will be useful later on)
        pvm_object_t copy = pvm_create_string_object_binary(pvm_get_str_data(item), pvm_get_str_len(item) + 1);
        pvm_get_str_data(copy)[pvm_get_str_len(item)] = '\0';
        pvm_append_array(wasm_da->env_vars_array, copy);
    }

    SYS_FREE_O(env_vars_obj);
    SYSCALL_RETURN_NOTHING;
}

void pvm_gc_iter_wasm(gc_iterator_call_t func, pvm_object_t self, void *arg) {
    pvm_wasm_da_t wasm_da = pvm_data_area(self, wasm);

    func(wasm_da->wasm_code_str, arg);
    func(wasm_da->env_vars_array, arg);

    if (wasm_da != master_instance) {
        func(pvm_da_to_object(master_instance), arg);
        return;
    }

    func(master_instance->wasm_global_objects_array, arg);
    func(master_instance->wasm_instance_objects_array, arg);
    func(master_instance->wasm_mutex_array, arg);
    func(master_instance->wasm_cond_array, arg);
}

syscall_func_t  syscall_table_4_wasm[16] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &invalid_syscall,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &si_load_module_wasm_8,             &si_invoke_wasm_wasm_9,
    &si_wasi_invoke_start_wasm_10,      &si_wasi_set_env_vars_wasm_11,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall
};

DECLARE_SIZE(wasm); // create variable holding size of syscall table

void pvm_restart_wasm(pvm_object_t o) {
    ph_printf("restarting wasm...\n");

    // we should only have a single wasm object in restart list
    assert(master_instance == NULL);
    r_assert(initialize_wasm(o)); // sets master_instance

    foreach_in_uarray(master_instance->wasm_mutex_array, entry) {
        if (*entry) {
            if (hal_mutex_init((hal_mutex_t*) *entry, NULL) < 0)
                SHOW_ERROR0(1, "Failed to restart mutex");
        }
    }

    foreach_in_uarray(master_instance->wasm_cond_array, entry) {
        if (*entry) {
            if (hal_cond_init((hal_cond_t*) *entry, NULL) < 0)
                SHOW_ERROR0(1, "Failed to restart cond");
        }
    }

    ph_printf("restarted wasm!\n");
}

// ######################################################
// #############     CALLBACKS FOR WAMR     #############
// ######################################################

void wamr_phantom_alloc_callback(pvm_object_t obj) {
    pvm_append_array(
        master_instance->wasm_instance_objects_array 
            ? master_instance->wasm_instance_objects_array 
            : master_instance->wasm_global_objects_array,
        obj
    );
}

void wamr_phantom_free_callback(pvm_object_t obj) {
    pvm_pop_array(master_instance->wasm_instance_objects_array, obj);
}

int wamr_phantom_create_mutex(hal_mutex_t *mutex, const char *name) {
    int ret = hal_mutex_init(mutex, name);
    if (ret < 0) return ret;

    append_uarray(&master_instance->wasm_mutex_array, mutex);

    return 0;
}

int wamr_phantom_destroy_mutex(hal_mutex_t *mutex) {
    int ret = hal_mutex_destroy(mutex);
    if (ret < 0) return ret;

    pop_uarray(master_instance->wasm_mutex_array, mutex);

    return 0;
}

int wamr_phantom_create_cond(hal_cond_t *mutex, const char *name) {
    int ret = hal_cond_init(mutex, name);
    if (ret < 0) return ret;

    append_uarray(&master_instance->wasm_cond_array, mutex);

    return 0;
}

int wamr_phantom_destroy_cond(hal_cond_t *mutex) {
    int ret = hal_cond_destroy(mutex);
    if (ret < 0) return ret;

    pop_uarray(master_instance->wasm_cond_array, mutex);

    return 0;
}
