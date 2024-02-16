    #define CATCH_CONFIG_RUNNER // Define this before including catch.hpp to create a custom main
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <vector>
#include "NeoFOAM/blas/fields.hpp"


#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>

int main(int argc, char* argv[]) {
    // Initialize Catch2
    Kokkos::initialize(argc, argv);
    Catch::Session session;

    // Specify command line options
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) // Indicates a command line error
        return returnCode;

    int result = session.run();


    // Run benchmarks if there are any
    Kokkos::finalize();
    
    return result;
}


void serial_scalarField_addition(std::vector<double>& a, std::vector<double>& b, std::vector<double>& c)
{
    for (int i = 0; i < a.size(); ++i)
    {
        c[i] = a[i] + b[i];
    }
}

void GPU_scalarField_addition(NeoFOAM::scalarField& a, NeoFOAM::scalarField& b, NeoFOAM::scalarField& c)
{
    c.apply(KOKKOS_LAMBDA(int i) { return a(i) + b(i); });
    Kokkos::fence();
}

TEST_CASE("Vector addition [benchmark]") {

    auto N = GENERATE(8, 64, 512, 4096, 32768, 262144, 1048576, 1048576*4, 1048576*16, 1048576*64);

    CAPTURE(N);  // Capture the value of N
    
    // capture the value of N as section name
    DYNAMIC_SECTION( "" << N ) {
        {
            std::vector<double> CPUa(N, 1.0);
            std::vector<double> CPUb(N, 2.0);
            std::vector<double> CPUc(N, 0.0);
            
            BENCHMARK("Serial vector addition") {
                return serial_scalarField_addition(CPUa, CPUb, CPUc);
            };
        }

        {
            NeoFOAM::scalarField GPUa("a", N);
            Kokkos::parallel_for(
                N, KOKKOS_LAMBDA(const int i) {
                    GPUa(i) = 1;
                });
            NeoFOAM::scalarField GPUb("b", N);
            Kokkos::parallel_for(
                N, KOKKOS_LAMBDA(const int i) {
                    GPUb(i) = 2;
                });
            NeoFOAM::scalarField GPUc("c", N);

            BENCHMARK("GPU vector addition") {
                return GPU_scalarField_addition(GPUa, GPUb, GPUc);
            };
        }
    };
}