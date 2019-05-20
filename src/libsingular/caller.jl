
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
    :NUMBER_CMD    => (NUMBER_CMD_CASTER,    true  ),
    :RING_CMD      => (RING_CMD_CASTER,      false ),
    :POLY_CMD      => (POLY_CMD_CASTER,      true  ),
    :IDEAL_CMD     => (IDEAL_CMD_CASTER,     true  ),
    :INT_CMD       => (INT_CMD_CASTER,       false ),
    :STRING_CMD    => (STRING_CMD_CASTER,    false ),
    :LIST_CMD      => (LIST_CMD_TRAVERSAL,   false ),
    :INTVEC_CMD    => (INTVEC_CMD_CASTER,    false ),
    :INTMAT_CMD    => (INTMAT_CMD_CASTER,    false ),
    :BIGINT_CMD    => (BIGINT_CMD_CASTER,    false ),
    :BIGINTMAT_CMD => (BIGINTMAT_CMD_CASTER, false ),
)

casting_functions = nothing

function create_casting_functions()
    pair_array = Any[]
    for (sym,func) in casting_functions_pre
        pos = findfirst(i->(i[2] == sym),mapping_types)
        push!(pair_array, mapping_types[pos][1] => func)
    end
    return Dict(pair_array...)
end

function convert_return_value(single_value,ring = nothing)
    if single_value[1]
        error("recieved list instead of single value")
    end
    cast = casting_functions[single_value[3]][1](single_value[2])
    if cast isa Array{Any}
        return recursive_translate(cast,ring)
    end
    if casting_functions[single_value[3]][2]
        cast = ring(cast)
    end
    return cast
end


function convert_return_list(list_value, ring = nothing)
    if list_value[1]
        return map(i->convert_return_value(i,ring),list_value[2:end])
    end
    return convert_return_value(list_value,ring)
end

function prepare_argument(argument)
    return argument.ptr
end
prepare_argument(x::T) where T <: AbstractString = x
prepare_argument(x::Int64) = x
prepare_argument(x::Array{Int64,1}) = x
prepare_argument(x::Array{Int64,2}) = x

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


function low_level_caller(lib::String,name::String,args...)
    load_library(lib)
    arguments = Any[prepare_argument(i) for i in args]
    ring = get_ring(args)
    return_values = nothing
    if ring == nothing
        return_value = call_singular_library_procedure(name,arguments)
    else
        return_value = call_singular_library_procedure(name,ring.ptr,arguments)
    end
    return convert_return_list(return_value,ring)
end
