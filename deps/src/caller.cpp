#include "caller.h"

#include <Singular/tok.h>
#include <Singular/grammar.h>
#include <Singular/ipshell.h>
#include <Singular/lists.h>

// #include <julia/julia.h>

static jl_value_t * get_type_mapper()
{
    std::vector<std::pair<int, std::string>> types = {
        std::pair<int, std::string>(NUMBER_CMD, "NUMBER_CMD"),
        std::pair<int, std::string>(RING_CMD, "RING_CMD"),
        std::pair<int, std::string>(POLY_CMD, "POLY_CMD"),
        std::pair<int, std::string>(IDEAL_CMD, "IDEAL_CMD"),
        std::pair<int, std::string>(INT_CMD, "INT_CMD"),
        std::pair<int, std::string>(STRING_CMD, "STRING_CMD"),
        std::pair<int, std::string>(LIST_CMD, "LIST_CMD")};

    jl_array_t * return_array =
        jl_alloc_array_1d(jl_array_any_type, types.size());

    for (int i = 0; i < types.size(); i++) {
        jl_array_t * current_return = jl_alloc_array_1d(jl_array_any_type, 2);
        jl_arrayset(current_return, jl_box_int64(types[i].first), 0);
        jl_arrayset(current_return,
                    reinterpret_cast<jl_value_t *>(
                        jl_symbol(types[i].second.c_str())),
                    1);
        jl_arrayset(return_array,
                    reinterpret_cast<jl_value_t *>(current_return), i);
    }
    return reinterpret_cast<jl_value_t *>(return_array);
}

static inline void * get_ptr_from_cxxwrap_obj(jl_value_t * obj)
{
    return *reinterpret_cast<void **>(obj);
}

// Safe way
// void* get_ptr_from_cxxwrap_obj(jl_value_t* obj){
//     return jl_unbox_voidpointer(jl_get_field(obj,"cpp_object"));
// }

bool translate_singular_type(
    jl_value_t * obj, void ** args, int * argtypes, int i, ring r)
{
    if (jl_isa(obj, jl_eval_string("Singular.libSingular.number"))) {
        argtypes[i] = NUMBER_CMD;
        args[i] = reinterpret_cast<void *>(n_Copy(
            reinterpret_cast<number>(get_ptr_from_cxxwrap_obj(obj)), r->cf));
        return true;
    }
    if (jl_isa(obj, jl_eval_string("Singular.libSingular.ring"))) {
        argtypes[i] = RING_CMD;
        void * rng = get_ptr_from_cxxwrap_obj(obj);
        reinterpret_cast<ring>(rng)->ref++;
        args[i] = rng;
        return true;
    }
    if (jl_isa(obj, jl_eval_string("Singular.libSingular.poly"))) {
        argtypes[i] = POLY_CMD;
        args[i] = reinterpret_cast<void *>(
            p_Copy(reinterpret_cast<poly>(get_ptr_from_cxxwrap_obj(obj)), r));
        return true;
    }
    if (jl_isa(obj, jl_eval_string("Singular.libSingular.ideal"))) {
        argtypes[i] = IDEAL_CMD;
        args[i] = reinterpret_cast<void *>(id_Copy(
            reinterpret_cast<ideal>(get_ptr_from_cxxwrap_obj(obj)), r));
        return true;
    }
    if (jl_isa(obj, jl_eval_string("Singular.libSingular.ip_smatrix"))) {
        argtypes[i] = MATRIX_CMD;
        args[i] = reinterpret_cast<void *>(mp_Copy(
            reinterpret_cast<matrix>(get_ptr_from_cxxwrap_obj(obj)), r));
        return true;
    }
    if (jl_is_int64(obj)) {
        argtypes[i] = INT_CMD;
        args[i] = reinterpret_cast<void *>(jl_unbox_int64(obj));
        return true;
    }
    if (jl_is_string(obj)) {
        argtypes[i] = STRING_CMD;
        int str_len = jl_string_len(obj);

        // char* new_string =
    }
    return false;
}

jl_value_t * get_julia_type_from_sleftv(leftv ret)
{
    jl_array_t * result = jl_alloc_array_1d(jl_array_any_type, 3);
    jl_arrayset(result, jl_false, 0);
    jl_arrayset(result, jl_box_voidpointer(ret->data), 1);
    jl_arrayset(result, jl_box_int64(ret->Typ()), 2);
    return reinterpret_cast<jl_value_t *>(result);
}

jl_value_t * call_singular_library_procedure(
    std::string s, ring r, jlcxx::ArrayRef<jl_value_t *> arguments)
{
    int    len = arguments.size();
    void * args[len];
    int    argtypes[len + 1];
    argtypes[len] = 0;
    for (int i = 0; i < len; i++) {
        bool result =
            translate_singular_type(arguments[i], args, argtypes, i, r);
        if (!result) {
            jl_error("Could not convert argument");
        }
    }
    BOOLEAN      err;
    jl_value_t * retObj;
    leftv        ret = ii_CallLibProcM(s.c_str(), args, argtypes, r, err);
    if (err) {
        jl_error("Could not call function");
    }
    if (ret->next != NULL) {
        int          len = ret->listLength();
        jl_array_t * list = jl_alloc_array_1d(jl_array_any_type, len + 1);
        jl_arrayset(list, jl_true, 0);
        for (int i = 0; i < len; ++i) {
            leftv next = ret->next;
            ret->next = 0;
            jl_arrayset(list, get_julia_type_from_sleftv(ret), i + 1);
            if (i > 0)
                omFreeBin(ret, sleftv_bin);
            ret = next;
        }
        retObj = reinterpret_cast<jl_value_t *>(list);
    }
    else {
        retObj = get_julia_type_from_sleftv(ret);
        omFreeBin(ret, sleftv_bin);
    }
    return retObj;
}

jl_value_t * call_singular_library_procedure_wo_ring(
    std::string name, jlcxx::ArrayRef<jl_value_t *> arguments)
{
    return call_singular_library_procedure(name, NULL, arguments);
}

jl_value_t * convert_nested_list(void* l_void){
    lists l = reinterpret_cast<lists>(l_void);
    int len = lSize(l) + 1;
    jl_array_t* result_array = jl_alloc_array_1d(jl_array_any_type,len);
    for(int i = 0; i < len; i++){
        leftv current = &(l->m[i]);
        if(current->Typ() == LIST_CMD){
            jl_arrayset(result_array,convert_nested_list(reinterpret_cast<void*>(current->data)), i);
        } else {
            jl_arrayset(result_array,get_julia_type_from_sleftv(current), i);
        }
    }
    return reinterpret_cast<jl_value_t*>(result_array);
}

void singular_define_caller(jlcxx::Module & Singular)
{
    Singular.method("load_library", [](std::string name) {
        char * plib = iiConvName(name.c_str());
        idhdl  h = ggetid(plib);
        omFree(plib);
        if (h == NULL) {
            BOOLEAN bo = iiLibCmd(omStrDup(name.c_str()), TRUE, TRUE, FALSE);
            if (bo)
                return jl_false;
        }
        return jl_true;
    });
    Singular.method("call_singular_library_procedure",
                    &call_singular_library_procedure);
    Singular.method("call_singular_library_procedure",
                    &call_singular_library_procedure_wo_ring);
    Singular.method("get_type_mapper", &get_type_mapper);


    Singular.method("NUMBER_CMD_CASTER",
                    [](void * obj) { return reinterpret_cast<number>(obj); });
    Singular.method("RING_CMD_CASTER",
                    [](void * obj) { return reinterpret_cast<ring>(obj); });
    Singular.method("POLY_CMD_CASTER",
                    [](void * obj) { return reinterpret_cast<poly>(obj); });
    Singular.method("IDEAL_CMD_CASTER",
                    [](void * obj) { return reinterpret_cast<ideal>(obj); });
    Singular.method("INT_CMD_CASTER", [](void * obj) {
        return jl_box_int64(reinterpret_cast<long>(obj));
    });
    Singular.method("STRING_CMD_CASTER", [](void * obj) {
        return std::string(reinterpret_cast<char *>(obj));
    });
    Singular.method("LIST_CMD_TRAVERSAL",&convert_nested_list);
}