function test_nemo_fmpq()
    print("caller...")

    R, (x, y) = PolynomialRing(Singular.QQ, ["x", "y"])

    # Input tests
    @test Singular.SETS.isEqualInt(1,1)       == 1
    @test Singular.SETS.isEqualInt(R,x,x)     == 1
    @test Singular.SETS.isEqualInt("aa","aa") == 1
    @test Singular.SETS.isEqualInt(R,Singular.QQ(1),Singular.QQ(1)) == 1
    @test Singular.SETS.isEqualInt(R,Singular.ZZ(1),Singular.ZZ(1)) == 1
    @test Singular.SETS.isEqualInt([1,2,3],[1,2,3])                 == 1
    @test Singular.SETS.isEqualInt([1 2; 3 4] ,[1 2; 3 4])          == 1

    println("PASS")
end
