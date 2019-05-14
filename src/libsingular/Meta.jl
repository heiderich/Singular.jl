include("libraryfuncdictionary.jl")

for (name,funcs) in libraryfunctiondictionary
    name_caps = Symbol(uppercase(string(name)))
    func_calls = Any[]
    name_string = string(name)
    for i in funcs
        if i[1] == "g"
            func_name = i[2]
            symb = Symbol(func_name)
            # push!(func_calls, name = :($name))
            push!(func_calls, :($symb(args...) = libSingular.low_level_caller($(name_string) * ".lib",$func_name,args...)))
        end
    end
    eval(:(baremodule $name_caps
        import ..libSingular
        import Base: *
        $(func_calls...)
    end))
end
