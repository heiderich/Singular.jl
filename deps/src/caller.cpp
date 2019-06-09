#include "caller.h"

#include <Singular/tok.h>
#include <Singular/grammar.h>
#include <Singular/ipshell.h>
#include <Singular/lists.h>
#include <misc/intvec.h>

// #include <julia/julia.h>

static jl_value_t * jl_int64_vector_type;
static jl_value_t * jl_int64_matrix_type;
static jl_value_t * jl_singular_number_type;
static jl_value_t * jl_singular_poly_type;
static jl_value_t * jl_singular_ring_type;
static jl_value_t * jl_singular_ideal_type;
static jl_value_t * jl_singular_matrix_type;
static jl_value_t * jl_singular_bigint_type;
static jl_value_t * jl_singular_bigintmat_type;
static jl_value_t * jl_singular_map_type;
static jl_value_t * jl_singular_resolution_type;
static jl_value_t * jl_singular_vector_type;

static jl_value_t * get_type_mapper()
{
    std::vector<std::pair<int, std::string>> types = {
        std::pair<int, std::string>(BIGINT_CMD, "BIGINT_CMD"),
        std::pair<int, std::string>(NUMBER_CMD, "NUMBER_CMD"),
        std::pair<int, std::string>(RING_CMD, "RING_CMD"),
        std::pair<int, std::string>(POLY_CMD, "POLY_CMD"),
        std::pair<int, std::string>(IDEAL_CMD, "IDEAL_CMD"),
        std::pair<int, std::string>(INT_CMD, "INT_CMD"),
        std::pair<int, std::string>(STRING_CMD, "STRING_CMD"),
        std::pair<int, std::string>(LIST_CMD, "LIST_CMD"),
        std::pair<int, std::string>(INTMAT_CMD, "INTMAT_CMD"),
        std::pair<int, std::string>(BIGINTMAT_CMD, "BIGINTMAT_CMD"),
        std::pair<int, std::string>(MAP_CMD, "MAP_CMD"),
        std::pair<int, std::string>(RESOLUTION_CMD, "RESOLUTION_CMD"),
        std::pair<int, std::string>(MODUL_CMD, "MODUL_CMD"),
        std::pair<int, std::string>(VECTOR_CMD, "VECTOR_CMD"),
        std::pair<int, std::string>(INTVEC_CMD, "INTVEC_CMD")};

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

static void initialize_jl_c_types(jl_value_t * module_value)
{
    jl_module_t * module = reinterpret_cast<jl_module_t *>(module_value);
    jl_int64_vector_type =
        jl_apply_array_type((jl_value_t *)jl_int64_type, 1);
    jl_int64_matrix_type =
        jl_apply_array_type((jl_value_t *)jl_int64_type, 2);
    jl_singular_number_type = jl_get_global(module, jl_symbol("number"));
    jl_singular_poly_type = jl_get_global(module, jl_symbol("poly"));
    jl_singular_ring_type = jl_get_global(module, jl_symbol("ring"));
    jl_singular_ideal_type = jl_get_global(module, jl_symbol("ideal"));
    jl_singular_matrix_type = jl_get_global(module, jl_symbol("ip_smatrix"));
    jl_singular_bigint_type =
        jl_get_global(module, jl_symbol("__mpz_struct"));
    jl_singular_bigintmat_type =
        jl_get_global(module, jl_symbol("bigintmat"));
    jl_singular_map_type = jl_get_global(module, jl_symbol("sip_smap"));
    jl_singular_resolution_type =
        jl_get_global(module, jl_symbol("resolvente"));
}

static inline void * get_ptr_from_cxxwrap_obj(jl_value_t * obj)
{
    return *reinterpret_cast<void **>(obj);
}

// Safe way
// void* get_ptr_from_cxxwrap_obj(jl_value_t* obj){
//     return jl_unbox_voidpointer(jl_get_field(obj,"cpp_object"));
// }

jl_value_t * intvec_to_jl_array(intvec * v)
{
    int          size = v->length();
    jl_array_t * result = jl_alloc_array_1d(jl_int64_vector_type, size);
    int *        v_content = v->ivGetVec();
    for (int i = 0; i < size; i++) {
        jl_arrayset(result, jl_box_int64(static_cast<int64_t>(v_content[i])),
                    i);
    }
    return reinterpret_cast<jl_value_t *>(result);
}

jl_value_t * intmat_to_jl_array(intvec * v)
{
    int          rows = v->rows();
    int          cols = v->cols();
    jl_array_t * result = jl_alloc_array_2d(jl_int64_matrix_type, rows, cols);
    int64_t * result_ptr = reinterpret_cast<int64_t *> jl_array_data(result);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            result_ptr[j + (i * cols)] = IMATELEM(*v, i, j);
        }
    }
    return reinterpret_cast<jl_value_t *>(result);
}

void * jl_array_to_intvec(jl_value_t * array_val)
{
    jl_array_t * array = reinterpret_cast<jl_array_t *>(array_val);
    int          size = jl_array_len(array);
    intvec *     result = new intvec(size);
    int *        result_content = result->ivGetVec();
    for (int i = 0; i < size; i++) {
        result_content[i] =
            static_cast<int>(jl_unbox_int64(jl_arrayref(array, i)));
    }
    return reinterpret_cast<void *>(result);
}

void * jl_array_to_intmat(jl_value_t * array_val)
{
    jl_array_t * array = reinterpret_cast<jl_array_t *>(array_val);
    int          rows = jl_array_dim(array, 0);
    int          cols = jl_array_dim(array, 1);
    intvec *     result = new intvec(rows, cols, 0);
    int64_t * array_data = reinterpret_cast<int64_t *>(jl_array_data(array));
    int *     vec_data = result->ivGetVec();
    for (int i = 0; i < cols; i++) {
        for (int j = 0; j < rows; j++) {
            IMATELEM(*result, i - 1, j - 1) =
                static_cast<int>(array_data[j + (i * rows)]);
        }
    }
    return reinterpret_cast<void *>(result);
}

static bool check_vector(poly p, ring r)
{
    if (p == 0) {
        return false;
    }
    return p_GetComp(p, r) != 0;
}

static void * get_ring_ref(ring r)
{
    r->ref++;
    return reinterpret_cast<void *>(r);
}

static jl_value_t * get_poly_ptr(poly p, ring r)
{
    poly         p_copy = p_Copy(p, r);
    jl_array_t * result = jl_alloc_array_1d(jl_array_any_type, 2);
    jl_arrayset(result, jl_box_voidpointer(reinterpret_cast<void *>(p_copy)),
                1);
    if (check_vector(p_copy, r)) {
        jl_arrayset(result, jl_box_int64(VECTOR_CMD), 0);
    }
    else {
        jl_arrayset(result, jl_box_int64(POLY_CMD), 0);
    }
    return reinterpret_cast<jl_value_t *>(result);
}

static jl_value_t * get_ideal_ptr(ideal i, ring r)
{
    ideal        i_copy = id_Copy(i, r);
    jl_array_t * result = jl_alloc_array_1d(jl_array_any_type, 2);
    jl_arrayset(result, jl_box_voidpointer(reinterpret_cast<void *>(i_copy)),
                1);
    jl_arrayset(result, jl_box_int64(IDEAL_CMD), 0);
    if (i_copy->rank != 1) {
        jl_arrayset(result, jl_box_int64(MODUL_CMD), 0);
    }
    else {
        int nr_elems = IDELEMS(i_copy);
        for (int i = 0; i < nr_elems; i++) {
            if (check_vector(i_copy->m[i], r)) {
                jl_arrayset(result, jl_box_int64(MODUL_CMD), 1);
            }
        }
    }
    return reinterpret_cast<jl_value_t *>(result);
}

static void * safe_singular_string(std::string s)
{
    return reinterpret_cast<void *>(omStrDup(s.c_str()));
}

bool translate_singular_type(jl_value_t * obj,
                             void **      args,
                             int *        argtypes,
                             int          i)
{
    jl_array_t * array = reinterpret_cast<jl_array_t *>(obj);
    int    cmd = static_cast<int>(jl_unbox_int64(jl_arrayref(array, 0)));
    void * arg = jl_unbox_voidpointer(jl_arrayref(array, 1));
    args[i] = arg;
    argtypes[i] = cmd;
    return true;
}

jl_value_t * get_julia_type_from_sleftv(leftv ret)
{
    jl_array_t * result = jl_alloc_array_1d(jl_array_any_type, 3);
    jl_arrayset(result, jl_false, 0);
    jl_arrayset(result, jl_box_voidpointer(ret->data), 1);
    jl_arrayset(result, jl_box_int64(ret->Typ()), 2);
    return reinterpret_cast<jl_value_t *>(result);
}

jl_value_t * get_ring_content(ring r)
{
    // count elements
    idhdl h = r->idroot;
    int   nr = 0;
    while (h != NULL) {
        nr++;
        h = IDNEXT(h);
    }
    jl_array_t * result = jl_alloc_array_1d(jl_array_any_type, nr);
    h = r->idroot;
    nr = 0;
    while (h != NULL) {
        jl_array_t * current = jl_alloc_array_1d(jl_array_any_type, 3);
        jl_arrayset(current, jl_box_int64(IDTYP(h)), 0);
        jl_arrayset(current,
                    reinterpret_cast<jl_value_t *>(jl_symbol(IDID(h))), 1);
        jl_arrayset(current, jl_box_voidpointer(IDDATA(h)), 2);
        jl_arrayset(result, reinterpret_cast<jl_value_t *>(current), nr);
        h = IDNEXT(h);
        nr++;
    }
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
            translate_singular_type(arguments[i], args, argtypes, i);
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

jl_value_t * convert_nested_list(void * l_void)
{
    lists        l = reinterpret_cast<lists>(l_void);
    int          len = lSize(l) + 1;
    jl_array_t * result_array = jl_alloc_array_1d(jl_array_any_type, len);
    for (int i = 0; i < len; i++) {
        leftv current = &(l->m[i]);
        if (current->Typ() == LIST_CMD) {
            jl_arrayset(
                result_array,
                convert_nested_list(reinterpret_cast<void *>(current->data)),
                i);
        }
        else {
            jl_arrayset(result_array, get_julia_type_from_sleftv(current), i);
        }
    }
    return reinterpret_cast<jl_value_t *>(result_array);
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
    Singular.method("initialize_jl_c_types", &initialize_jl_c_types);


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
    Singular.method("INTVEC_CMD_CASTER", [](void * obj) {
        return intvec_to_jl_array(reinterpret_cast<intvec *>(obj));
    });
    Singular.method("INTMAT_CMD_CASTER", [](void * obj) {
        return intmat_to_jl_array(reinterpret_cast<intvec *>(obj));
    });
    Singular.method("BIGINT_CMD_CASTER", [](void * obj) {
        return reinterpret_cast<__mpz_struct *>(obj);
    });
    Singular.method("BIGINTMAT_CMD_CASTER", [](void * obj) {
        return reinterpret_cast<bigintmat *>(obj);
    });
    Singular.method("MAP_CMD_CASTER", [](void * obj) {
        return reinterpret_cast<sip_smap *>(obj);
    });
    Singular.method("LIST_CMD_TRAVERSAL", &convert_nested_list);
    Singular.method("get_ring_content", &get_ring_content);
    Singular.method("get_ring_ref", &get_ring_ref);
    Singular.method("get_poly_ptr", &get_poly_ptr);
    Singular.method("get_ideal_ptr", &get_ideal_ptr);
    Singular.method("jl_array_to_intvec", &jl_array_to_intvec);
    Singular.method("jl_array_to_intmat", &jl_array_to_intmat);
    Singular.method("safe_singular_string", &safe_singular_string);
}