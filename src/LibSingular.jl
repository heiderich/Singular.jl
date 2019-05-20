module libSingular

import Libdl
using CxxWrap
@wrapmodule(realpath(joinpath(@__DIR__, "..", "local", "lib", "libsingularwrap." * Libdl.dlext)))

mapping_types = nothing

function __init__()
   @initcxx
   global mapping_types, casting_functions
   mapping_types = get_type_mapper()
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
