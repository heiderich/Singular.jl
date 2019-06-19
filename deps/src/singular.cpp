#include "jlcxx/jlcxx.hpp"
#include "includes.h"
#include "coeffs.h"
#include "rings.h"
#include "ideals.h"
#include "matrices.h"
#include "caller.h"


JLCXX_MODULE define_julia_module(jlcxx::Module & Singular)
{
    Singular.add_type<n_Procs_s>("coeffs");
    Singular.add_bits<n_coeffType>("n_coeffType");
    Singular.set_const("n_Z", n_Z);
    Singular.set_const("n_Q", n_Q);
    Singular.set_const("n_Zn", n_Zn);
    Singular.set_const("n_Zp", n_Zp);
    Singular.set_const("n_GF", n_GF);
    Singular.set_const("n_unknown", n_unknown);
    Singular.add_type<snumber>("number");
    Singular.add_type<__mpz_struct>("__mpz_struct");
    Singular.add_type<ip_sring>("ring");
    Singular.add_type<spolyrec>("poly");
    // Singular.add_type<nMapFunc>("nMapFunc");
    // Singular.add_type<spolyrec>("vector");
    Singular.add_bits<rRingOrder_t>("rRingOrder_t");
    Singular.add_type<sip_sideal>("ideal");
    Singular.add_type<ip_smatrix>("ip_smatrix");
    Singular.add_type<ssyStrategy>("syStrategy");
    Singular.add_type<sip_smap>("sip_smap");
    Singular.add_type<bigintmat>("bigintmat");

    /* monomial orderings */
    Singular.set_const("ringorder_no", ringorder_no);
    Singular.set_const("ringorder_lp", ringorder_lp);
    Singular.set_const("ringorder_rp", ringorder_rp);
    Singular.set_const("ringorder_dp", ringorder_dp);
    Singular.set_const("ringorder_Dp", ringorder_Dp);
    Singular.set_const("ringorder_ls", ringorder_ls);
    Singular.set_const("ringorder_rs", ringorder_rs);
    Singular.set_const("ringorder_ds", ringorder_ds);
    Singular.set_const("ringorder_Ds", ringorder_Ds);
    Singular.set_const("ringorder_c", ringorder_c);
    Singular.set_const("ringorder_C", ringorder_C);

    Singular.method("siInit", [](const char * path) {
        siInit(const_cast<char *>(path));
    });
    Singular.method("versionString", []() {
        return const_cast<const char *>(versionString());
    });

    singular_define_coeffs(Singular);
    singular_define_rings(Singular);
    singular_define_ideals(Singular);
    singular_define_matrices(Singular);
    singular_define_caller(Singular);

    /****************************
     ** from resolutions.jl
     ***************************/

    Singular.method("res_Delete_helper",
                    [](syStrategy ra, ring o) { syKillComputation(ra, o); });

    Singular.method("res_Copy", [](syStrategy ra, ring o) {
        const ring origin = currRing;
        rChangeCurrRing(o);
        syStrategy temp = syCopy(ra);
        rChangeCurrRing(origin);
        return temp;
    });

    Singular.method("getindex_internal",
                    [](syStrategy ra, int64_t k, bool minimal) {
                        if (minimal) {
                            return ra->minres[k];
                        }
                        return (ideal)ra->fullres[k];
                    });

    Singular.method("syMinimize", [](syStrategy ra, ring o) {
        const ring origin = currRing;
        rChangeCurrRing(o);
        syStrategy temp = syCopy(ra);
        syMinimize(temp);
        rChangeCurrRing(origin);
        return reinterpret_cast<void *>(temp);
    });

    Singular.method("get_minimal_res", [](syStrategy ra) {
        return reinterpret_cast<void *>(ra->minres);
    });

    Singular.method("get_full_res", [](syStrategy ra) {
        return reinterpret_cast<void *>(ra->fullres);
    });

    Singular.method("get_sySize", [](syStrategy ra) {
        return static_cast<int64_t>(sySize(ra));
    });

    Singular.method("create_SyStrategy", [](void * res_void, int64_t len,
                                            ring r) {
        resolvente res = reinterpret_cast<resolvente>(res_void);
        syStrategy result = (syStrategy)omAlloc0(sizeof(ssyStrategy));
        result->list_length = static_cast<short>(len);
        result->length = static_cast<int>(len);
        resolvente res_cp = (resolvente)omAlloc0((len + 1) * sizeof(ideal));
        for (int i = 0; i <= len; i++) {
            if (res[i] != NULL) {
                res_cp[i] = id_Copy(res[i], r);
            }
        }
        result->fullres = res_cp;
        result->syRing = r;
        return result;
    });

    Singular.method("syBetti_internal", [](void * ra, int len, ring o) {
        const ring origin = currRing;
        rChangeCurrRing(o);
        int      dummy;
        intvec * iv = syBetti(reinterpret_cast<resolvente>(ra), len, &dummy,
                              NULL, FALSE, NULL);
        rChangeCurrRing(origin);
        int  nrows = iv->rows();
        int  ncols = iv->cols();
        auto betti = (int *)malloc(ncols * nrows * sizeof(int));
        for (int i = 0; i < ncols; i++) {
            for (int j = 0; j < nrows; j++) {
                betti[i * nrows + j] = IMATELEM(*iv, j + 1, i + 1);
            }
        }
        delete (iv);
        return std::make_tuple(betti, nrows, ncols);
    });
}
