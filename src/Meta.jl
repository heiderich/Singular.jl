include("libraryfuncdictionary.jl")

output_maniuplator_funcs = Dict(
    :dummy => Dict(
        # :dummy => i -> i[1]
    )
)

for (name,funcs) in libraryfunctiondictionary
    name_caps = Symbol(uppercase(string(name)))
    func_calls = Any[]
    name_string = string(name) * ".lib"
    for i in funcs
        if i[1] == "g"
            func_name = i[2]
            symb = Symbol(func_name)
            if haskey(output_maniuplator_funcs,name) && haskey(output_maniuplator_funcs[name],symb)
                push!(func_calls, :($symb(args...) = $(output_maniuplator_funcs[name][symb])(low_level_caller($(name_string),$func_name,args...)) ))
                push!(func_calls, :($symb(ring::PolyRing,args...) = $(output_maniuplator_funcs[name][symb])(low_level_caller_rng($(name_string),$func_name,ring,args...)) ))
            else
                push!(func_calls, :($symb(args...) = low_level_caller($(name_string),$func_name,args...)))
                push!(func_calls, :($symb(ring::PolyRing,args...) = low_level_caller_rng($(name_string),$func_name,ring,args...)))
            end
        end
    end
    eval(:(baremodule $name_caps
        import ..Singular: PolyRing, low_level_caller, low_level_caller_rng
        import Base: *
        $(func_calls...)
    end))
end
