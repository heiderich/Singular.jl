
function recursive_translate(x,R)
    if length(x) > 0 && x[1] isa Bool
        return convert_return_list(x,R)
    else
        return [ recursive_translate(i,R) for i in x]
    end
end

# function LIST_CMD_CASTER(x)
#     x_new = LIST_CMD_TRAVERSAL(x)
# end

casting_functions_pre = Dict(
    :NUMBER_CMD     => (libSingular.NUMBER_CMD_CASTER,     true  , ()),
    :RING_CMD       => (libSingular.RING_CMD_CASTER,       false , ()),
    :POLY_CMD       => (libSingular.POLY_CMD_CASTER,       true  , ()),
    :IDEAL_CMD      => (libSingular.IDEAL_CMD_CASTER,      true  , ()),
    :MODUL_CMD      => (libSingular.IDEAL_CMD_CASTER,      true  , (:module,) ),
    :VECTOR_CMD     => (libSingular.POLY_CMD_CASTER,       true  , (:vector,) ),
    :INT_CMD        => (libSingular.INT_CMD_CASTER,        false , ()),
    :STRING_CMD     => (libSingular.STRING_CMD_CASTER,     false , ()),
    :LIST_CMD       => (libSingular.LIST_CMD_TRAVERSAL,    false , ()),
    :INTVEC_CMD     => (libSingular.INTVEC_CMD_CASTER,     false , ()),
    :INTMAT_CMD     => (libSingular.INTMAT_CMD_CASTER,     false , ()),
    :BIGINT_CMD     => (libSingular.BIGINT_CMD_CASTER,     false , ()),
    :BIGINTMAT_CMD  => (libSingular.BIGINTMAT_CMD_CASTER,  false , ()),
    :MAP_CMD        => (libSingular.MAP_CMD_CASTER,        false , ()),
    :RESOLUTION_CMD => (libSingular.RESOLUTION_CMD_CASTER, true  , (:resolution,) )
)

casting_functions = nothing

function create_casting_functions()
    pair_array = Any[]
    for (sym,func) in casting_functions_pre
        push!(pair_array, mapping_types_reversed[sym] => func)
    end
    return Dict(pair_array...)
end

function convert_ring_content(value_list,rng)
    return_dict = Dict{Symbol,Any}()
    for i in value_list
        return_dict[i[2]] = convert_return_value([false,i[3],i[1]],rng)
    end
    return return_dict
end

function convert_return_value(single_value,rng = nothing)
    if single_value[1]
        error("recieved list instead of single value")
    end
    cast = casting_functions[single_value[3]][1](single_value[2])
    if cast isa Array{Any}
        return recursive_translate(cast,rng)
    elseif cast isa libSingular.ring
        new_ring = rng(cast)
        return [ new_ring, convert_ring_content(libSingular.get_ring_content(cast),new_ring) ]
    elseif casting_functions[single_value[3]][2]
        if length(casting_functions[single_value[3]][3]) > 0
            cast = rng(cast,Val(casting_functions[single_value[3]][3][1]))
        else
            cast = rng(cast)
        end
    end
    return cast
end


function convert_return_list(list_value, ring = nothing)
    if list_value[1]
        return map(i->convert_return_value(i,ring),list_value[2:end])
    end
    return convert_return_value(list_value,ring)
end

function get_ring(arg_list)
    ring = nothing
    for i in arg_list
        current_ptr = nothing
        try
            current_ptr = i.ptr
        catch
            continue
        end
        if current_ptr isa poly
            return parent(i)
        elseif current_ptr isa ideal
            return parent(i).base_ring
        end
    end
    return ring
end

function prepare_argument(x::Array{Int64,1})
    return Any[ mapping_types_reversed[:INTVEC_CMD], libSingular.jl_array_to_intvec(x) ], nothing
end

function prepare_argument(x::Array{Int64,2})
    return Any[ mapping_types_reversed[:INTMAT_CMD], libSingular.jl_array_to_intmat(x) ], nothing
end

function prepare_argument(x::Int64)
    return Any[ mapping_types_reversed[:INT_CMD], Ptr{Cvoid}(x) ], nothing
end

function prepare_argument(x::String)
    return Any[ mapping_types_reversed[:STRING_CMD], libSingular.safe_singular_string(x) ], nothing
end

function prepare_argument(x::PolyRing)
    new_ptr = libSingular.get_ring_ref(x.ptr)
    return Any[ mapping_types_reversed[:RING_CMD], new_ptr ], x
end

function prepare_argument(x::spoly)
    rng = parent(x)
    rng_ptr = rng.ptr
    return Any[ mapping_types_reversed[:POLY_CMD], libSingular.get_poly_ptr(x.ptr,rng_ptr) ], rng
end

function prepare_argument(x::svector)
    rng = parent(x).base_ring
    rng_ptr = rng.ptr
    return Any[ mapping_types_reversed[:VECTOR_CMD], libSingular.get_poly_ptr(x.ptr,rng_ptr) ], rng
end

function prepare_argument(x::sideal)
    rng = parent(x).base_ring
    rng_ptr = rng.ptr
    return Any[ mapping_types_reversed[:IDEAL_CMD], libSingular.get_ideal_ptr(x.ptr,rng_ptr)], rng
end

function prepare_argument(x::smodule)
    rng = parent(x).base_ring
    rng_ptr = rng.ptr
    return Any[ mapping_types_reversed[:MODUL_CMD], libSingular.get_ideal_ptr(x.ptr,rng_ptr)], rng
end

function prepare_argument(x::sresolution)
    rng = base_ring(x)
    res = Any[ mapping_types_reversed[:RESOLUTION_CMD], libSingular.create_syStrategy_data(x.ptr,rng.ptr) ]
    return res,rng
end

function prepare_argument(x::Any)
    if x.ptr isa libSingular.number
        ptr = x.ptr
        rng = parent(x)
        new_ptr = libSingular.n_Copy(ptr,rng.ptr)
        return Any[ mapping_types_reversed[:NUMBER_CMD], new_ptr.cpp_object ], nothing
    elseif x.ptr isa libSingular.ip_smatrix
        rng = parent(x)
        return Any[ mapping_types_reversed[:MATRIX_CMD], libSingular.mpCopy(x.ptr,rng.ptr).cpp_object ], rng
    elseif x.ptr isa libSingular.__mpz_struct
        return Any[ mapping_types_reversed[:BIGINT_CMD], x.ptr.cpp_object ], nothing
    elseif x.ptr isa libSingular.sip_smap
        return Any[ mapping_types_reversed[:MAP_CMD], x.ptr.cpp_object ], nothing
    elseif x.ptr isa libSingular.bigintmat
        return Any[ mapping_types_reversed[:BIGINTMAT_CMD], x.ptr.cpp_object ], nothing
    end

end

function low_level_caller_rng(lib::String,name::String,ring,args...)
    libSingular.load_library(lib)
    arguments = [prepare_argument(i) for i in args]
    arguments = Any[ i for (i,j) in arguments ]
    return_value = libSingular.call_singular_library_procedure(name,ring.ptr,arguments)
    return convert_return_list(return_value,ring)
end

function low_level_caller(lib::String,name::String,args...)
    libSingular.load_library(lib)
    arguments = [prepare_argument(i) for i in args]
    rng = nothing
    for (i,j) in arguments
        if j != nothing
            rng = j
        end
    end
    arguments = Any[ i for (i,j) in arguments ]
    return_values = nothing
    if rng == nothing
        return_value = libSingular.call_singular_library_procedure(name,arguments)
    else
        return_value = libSingular.call_singular_library_procedure(name,rng.ptr,arguments)
    end
    return convert_return_list(return_value,rng)
end