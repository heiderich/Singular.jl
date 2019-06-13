function test_caller()
    print("caller...")

    R, (x, y) = PolynomialRing(Singular.ZZ, ["x", "y"])
    @test Singular.SETS.isEqualInt(R, Singular.ZZ(1), Singular.ZZ(1)) == 1

    R, (x, y) = PolynomialRing(Singular.QQ, ["x", "y"])

    # Input tests
    @test Singular.SETS.isEqualInt(R, 1, 1)                           == 1
    @test Singular.SETS.isEqualInt(R, x, x)                           == 1
    @test Singular.SETS.isEqualInt(R, "aa", "aa")                     == 1
    @test Singular.SETS.isEqualInt(R, Singular.QQ(1), Singular.QQ(1)) == 1
    @test Singular.SETS.isEqualInt(R, [1,2,3], [1,2,3])               == 1
    @test Singular.SETS.isEqualInt(R, [1 2; 3 4], [1 2; 3 4])         == 1

    R, (x,y,z) = PolynomialRing(Singular.QQ, ["x", "y", "z"])
    
    i1 = Singular.POLY.cyclic(R, 3)
    i2 = Ideal( R, x+y+z, x*y+x*z+y*z, x*y*z-1 )
    @test equal(i1, i2)

    vec = FreeModule(R,2)([x,y])
    mod = Singular.Module(R, vec)
    i1 = Singular.POLY.mod2id(R,mod,[1,2])
    i2 = Ideal(R, x^2, x*y, y^2, x^2 )
    @test equal(i1, i2)

    i1 = Ideal(R, x, y)
    i2 = Ideal(R, x^2, x*y, y^2, x, y)
    mod = Singular.POLY.id2mod(R, i1, [1,2])
    i1 = Singular.POLY.mod2id(R, mod, [1,2])
    @test equal(i1,i2)
    @test Singular.POLY.content(R,vec) == 1
    @test Singular.POLY.lcm(R,x) == x

    # i1 = Ideal(R, x*z, y*z, x^3-y^3)
    # @test Singular.STANDARD.res(R,i1,0) isa Singular.sresolution
    # i1 = Ideal(R, x*z, y*z, x^3-y^3)
    # @test Singular.PRIMDEC.primdecGTZ(R,i1) isa Array

    # i1 = Ideal(R, x*z, y*z, x^3-y^3)
    # @test Singular.NORMAL.normal(i1, "withDelta", "prim") isa Array

    println("PASS")
end
