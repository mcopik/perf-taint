#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <sanitizer/dfsan_interface.h>

#include "runtime.h"

//extern int32_t __EXTRAP_INSTRUMENTATION_RESULTS[];
//extern int8_t * __EXTRAP_INSTRUMENTATION_FUNCS_NAMES[];
//extern int32_t __EXTRAP_INSTRUMENTATION_FUNCS_COUNT;
//extern int32_t __EXTRAP_INSTRUMENTATION_PARAMS_COUNT;
//
extern dfsan_label __EXTRAP_INSTRUMENTATION_LABELS[];


dependencies * __dfsw_EXTRAP_DEPS_FUNC(int func_idx)
{
    static dependencies * results = NULL;
    if(!results) {
        results = calloc(sizeof(dependencies), __EXTRAP_INSTRUMENTATION_FUNCS_COUNT);
    }
    return &results[func_idx];
}

int32_t __dfsw_EXTRAP_VAR_ID()
{
    static int32_t id = 0;
    return id++;
}

void __dfsw_EXTRAP_AT_EXIT()
{
    //int vars_count = __EXTRAP_INSTRUMENTATION_PARAMS_COUNT;
    //for(int i = 0; i < __EXTRAP_INSTRUMENTATION_FUNCS_COUNT; ++i) {
    //    printf("Function %s depends on: ", __EXTRAP_INSTRUMENTATION_FUNCS_NAMES[i]);
    //    for(int j = 0; j < vars_count; ++j) {
    //        if(__EXTRAP_INSTRUMENTATION_RESULTS[i*vars_count + j])
    //            printf("%d ", j);
    //    }
    //    printf("\n");
    //}
    //fflush(stdout);
    __dfsw_dump_json_output();
    //dependencies * deps = __dfsw_EXTRAP_DEPS_FUNC(0);
    dependencies * deps = __EXTRAP_LOOP_DEPENDENCIES;
    for(int i = 0; i < __EXTRAP_INSTRUMENTATION_FUNCS_COUNT; ++i)
        free(deps[i].deps);
    free(deps);
}

void __dfsw_EXTRAP_CHECK_CALLSITE(int8_t * addr, size_t size, int32_t function_idx, int32_t callsite_idx, int32_t arg_idx)
{
    //dfsan_label temp = dfsan_read_label(addr, size);
    //for(int i = 0; i < __EXTRAP_INSTRUMENTATION_PARAMS_COUNT; ++i) {
    //    if(__EXTRAP_INSTRUMENTATION_LABELS[i]) {
    //        printf("Call %d func at %d site for arg %d label %d result %d\n",
    //                function_idx, callsite_idx, arg_idx, i, dfsan_has_label(temp, __EXTRAP_INSTRUMENTATION_LABELS[i]));
    //        if(dfsan_has_label(temp, __EXTRAP_INSTRUMENTATION_LABELS[i])) {
    //            int offset = __EXTRAP_INSTRUMENTATION_CALLSITES_OFFSETS[function_idx];
    //            int arg_count = __EXTRAP_INSTRUMENTATION_FUNCS_ARGS[function_idx];
    //            offset += arg_count * callsite_idx;
    //            offset += arg_idx;
    //            __EXTRAP_INSTRUMENTATION_CALLSITES_RESULTS[offset] |= (1 << i);
    //        }
    //    }
    //}
}

bool __dfsw_is_power_of_two(uint16_t val)
{
    return val && !(val & (val - 1) );
}

void __dfsw_add_dep(uint16_t val, dependencies * deps)
{
    //TODO: more efficient with capacity
    for(size_t i = 0; i < deps->len; ++i) {
        if(deps->deps[i] == val)
            return;
        // existing val is subset -> replace
        if( (deps->deps[i] & val) == deps->deps[i]) {
            deps->deps[i] = val;
        }
        // new value is a subset -> ignore
        if( (deps->deps[i] & val) == val) {
            return;
        }
    }
    deps->deps = realloc(deps->deps, sizeof(dependencies) * (deps->len + 1));
    deps->deps[deps->len] = val;
    deps->len++;
}

void __dfsw_EXTRAP_CHECK_LABEL(uint16_t temp, int32_t loop_idx,
        int32_t depth, int32_t function_idx)
{
    // First, we track all of found labels.
    // If there is more then one label, we write down dynamically a tuple.
    // If there is exactly one label, we write it statically.
    // Otherwise we don't write any results.
    //int offset = function_idx*__EXTRAP_INSTRUMENTATION_PARAMS_COUNT;
    //uint16_t found_params = 0;
    //for(int i = 0; i < __EXTRAP_INSTRUMENTATION_PARAMS_COUNT; ++i)
    //    if(__EXTRAP_INSTRUMENTATION_LABELS[i]) {
    //        bool has_label = dfsan_has_label(temp, __EXTRAP_INSTRUMENTATION_LABELS[i]);
    //        found_params |= (has_label << i);
    //        //printf("%d %d %d\n", function_idx, has_label, found_params);
    //    }
    ////printf("%d %d \n", function_idx, found_params);
    //// write down only a combination
    //if(found_params && !__dfsw_is_power_of_two(found_params)) {
    //    __dfsw_add_dep(found_params, __dfsw_EXTRAP_DEPS_FUNC(function_idx));
    //}
    //// write down a parameter
    //else if(found_params) {
    //    for(int i = 0; i < __EXTRAP_INSTRUMENTATION_PARAMS_COUNT; ++i) {
    //        if(found_params & (1 << i)) {
    //            __EXTRAP_INSTRUMENTATION_RESULTS[offset + i] = true;
    //            break;
    //        }
    //    }
    //}

    int32_t depths_offset = __EXTRAP_LOOPS_DEPTHS_FUNC_OFFSETS[function_idx];
    //Position of `dependencies` object for this loop is composed from
    //a) beginning offset of this function
    //b) sum of depths for all previous loops in this function
    //c) depth level for this loop
    int offset = __EXTRAP_LOOPS_DEPS_OFFSETS[function_idx];
    for(int i = 0; i < loop_idx; ++i)
        offset += __EXTRAP_LOOPS_DEPTHS_PER_FUNC[depths_offset + i];
    offset += depth;
    uint16_t found_params = 0;
    for(int i = 0; i < __EXTRAP_INSTRUMENTATION_PARAMS_COUNT; ++i)
        if(__EXTRAP_INSTRUMENTATION_LABELS[i]) {
            bool has_label = dfsan_has_label(temp, __EXTRAP_INSTRUMENTATION_LABELS[i]);
            found_params |= (has_label << i);
            //printf("%d %d %d\n", function_idx, has_label, found_params);
        }
    if(found_params)
        __dfsw_add_dep(found_params, &__EXTRAP_LOOP_DEPENDENCIES[offset]);
}

void __dfsw_EXTRAP_CHECK_LOAD(int8_t * addr, size_t size,
        int32_t loop_idx, int32_t depth, int32_t func_idx)
{
    if(!addr)
        return;
    dfsan_label temp = dfsan_read_label(addr, size);
    __dfsw_EXTRAP_CHECK_LABEL(temp, loop_idx, depth, func_idx);
}

void __dfsw_EXTRAP_STORE_LABEL(int8_t * addr, size_t size, int32_t param_idx, const char * name)
{
    dfsan_label lab = dfsan_create_label(name, NULL);
    __EXTRAP_INSTRUMENTATION_LABELS[param_idx] = lab;
    __EXTRAP_INSTRUMENTATION_PARAMS_NAMES[param_idx] = name;
    dfsan_set_label(lab, addr, size);
    //printf("Create label %d for %d at %d %p %s\n", lab, param_idx, size, addr, name);
    //printf("Set label %d\n", dfsan_read_label(addr, size));
}

void __dfsw_EXTRAP_INIT()
{
    int deps_count = __EXTRAP_LOOPS_DEPS_OFFSETS[
                __EXTRAP_INSTRUMENTATION_FUNCS_COUNT
            ];
    __EXTRAP_LOOP_DEPENDENCIES = malloc(sizeof(dependencies) * deps_count);
    __dfsw_json_initialize();
}

//void __dfsw_EXTRAP_CHECK_CALLSITE(int function_idx, int callsite_idx,
//        int arg_idx, int8_t * addr, size_t size)
//{
//    dfsan_label temp = dfsan_read_label(addr, size);
//    bool found_params[__EXTRAP_INSTRUMENTATION_PARAMS_COUNT] = {0};
//    for(int i = 0; i < __EXTRAP_INSTRUMENTATION_PARAMS_COUNT; ++i)
//        if(__EXTRAP_INSTRUMENTATION_LABELS[i]) {
//            dfound_params[i] =
//                dfsan_has_label(temp, __EXTRAP_INSTRUMENTATION_LABELS[i]);
//    __dfsw_json_callsite(function_idx, callsite_idx, arg_idx, found_params);
//}
