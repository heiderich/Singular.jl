function execute(cmd::Cmd)
    out = Pipe()
    err = Pipe()
  
    process = run(pipeline(ignorestatus(cmd), stdout = out, stderr = err))
    close(out.in)
    close(err.in)
  
    (stdout = String(read(out)), 
      stderr = String(read(err)),  
      code = process.exitcode)
end

libparsepath = abspath(joinpath(@__DIR__, "..", "local", "bin", "libparse"))

library_dir = ""

if haskey(ENV, "SINGULAR_LIBRARY_DIR")
    library_dir = ENV["SINGULAR_LIBRARY_DIR"]
else
    library_dir = abspath(joinpath(@__DIR__, "..", "local", "share", "singular", "LIB"))
end

filenames = filter(x -> endswith(x, ".lib"), readdir(library_dir))

output_filename = abspath(joinpath(@__DIR__, "..", "src", "libraryfuncdictionary.jl"))

open(output_filename, "w") do outputfile
    println(outputfile, "libraryfunctiondictionary = Dict(")
    for i in filenames
        full_path = joinpath(library_dir, i)
        libs = execute(`$libparsepath $full_path`)
        if libs.stderr != ""
            error("from libparse: $(libs.stderr)")
        end
        libs_splitted = split(libs.stdout,"\n")[4:end-1]
        libs_splitted = [ split(i," ") for i in libs_splitted ]
        libs_splitted = [ [ j for j in i if j != ""] for i in libs_splitted ]
        println(outputfile, ":$(i[1:end - 4]) => [")
        for j in libs_splitted
            println(outputfile, """[ "$(j[1])", "$(j[3])" ],""")
        end
        println(outputfile, "],\n")
    end
    println(outputfile, ")\n")
end
