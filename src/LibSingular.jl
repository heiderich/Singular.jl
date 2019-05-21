module libSingular

import Libdl

# We need the top-level eval for now
import ..Singular

using CxxWrap
@wrapmodule(realpath(joinpath(@__DIR__, "..", "local", "lib", "libsingularwrap." * Libdl.dlext)))

mapping_types = nothing
mapping_types_reversed = nothing

function __init__()
   @initcxx
   global mapping_types, mapping_types_reversed, casting_functions
   mapping_types = Dict( i[1] => i[2] for i in get_type_mapper() )
   mapping_types_reversed = Dict( j => i for (i,j) in mapping_types )
   casting_functions = create_casting_functions()
   initialize_jl_c_types(@__MODULE__)
end

include("libsingular/LibSingularTypes.jl")

include("libsingular/coeffs.jl")

include("libsingular/rings.jl")

include("libsingular/matrices.jl")

include("libsingular/ideals.jl")

include("libsingular/resolutions.jl")

include("libsingular/caller.jl")

end # module
