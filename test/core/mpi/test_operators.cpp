// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2023 NeoFOAM authors

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include "NeoFOAM/core/mpi/operators.hpp"

using namespace NeoFOAM;
using namespace NeoFOAM::mpi;

TEST_CASE("getOp")
{
    REQUIRE(getOp(ReduceOp::Max) == MPI_MAX);
    REQUIRE(getOp(ReduceOp::Min) == MPI_MIN);
    REQUIRE(getOp(ReduceOp::Sum) == MPI_SUM);
    REQUIRE(getOp(ReduceOp::Prod) == MPI_PROD);
    REQUIRE(getOp(ReduceOp::Land) == MPI_LAND);
    REQUIRE(getOp(ReduceOp::Band) == MPI_BAND);
    REQUIRE(getOp(ReduceOp::Lor) == MPI_LOR);
    REQUIRE(getOp(ReduceOp::Bor) == MPI_BOR);
    REQUIRE(getOp(ReduceOp::Maxloc) == MPI_MAXLOC);
    REQUIRE(getOp(ReduceOp::Minloc) == MPI_MINLOC);
}

TEST_CASE("getType")
{
    REQUIRE(getType<char>() == MPI_CHAR);
    REQUIRE(getType<wchar_t>() == MPI_WCHAR);
    REQUIRE(getType<short>() == MPI_SHORT);
    REQUIRE(getType<int>() == MPI_INT);
    REQUIRE(getType<long>() == MPI_LONG);
    REQUIRE(getType<long long>() == MPI_LONG_LONG);
    REQUIRE(getType<unsigned>() == MPI_UNSIGNED);
    REQUIRE(getType<unsigned long>() == MPI_UNSIGNED_LONG);
    REQUIRE(getType<unsigned long long>() == MPI_UNSIGNED_LONG_LONG);
    REQUIRE(getType<float>() == MPI_FLOAT);
    REQUIRE(getType<double>() == MPI_DOUBLE);
    REQUIRE(getType<long double>() == MPI_LONG_DOUBLE);
    REQUIRE(getType<bool>() == MPI_CXX_BOOL);
    REQUIRE(getType<std::complex<float>>() == MPI_CXX_FLOAT_COMPLEX);
    REQUIRE(getType<std::complex<double>>() == MPI_CXX_DOUBLE_COMPLEX);
    REQUIRE(getType<std::complex<long double>>() == MPI_CXX_LONG_DOUBLE_COMPLEX);
}

TEST_CASE("reduceAllScalar")
{

    SECTION("Integer reduction with MPI_MAX")
    {
        int rank;
        int ranks;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &ranks);
        int value = rank;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Max, MPI_COMM_WORLD);
        REQUIRE(send_value == (ranks - 1));
    }

    SECTION("Integer reduction with MPI_MIN")
    {
        int rank;
        int ranks;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &ranks);
        int value = rank;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Min, MPI_COMM_WORLD);
        REQUIRE(send_value == 0);
    }

    SECTION("Integer reduction with MPI_SUM")
    {
        int ranks;
        MPI_Comm_size(MPI_COMM_WORLD, &ranks);
        int value = 2.5f;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Sum, MPI_COMM_WORLD);
        REQUIRE(send_value == value * ranks);
    }

    SECTION("Float reduction with MPI_PROD")
    {
        int ranks;
        MPI_Comm_size(MPI_COMM_WORLD, &ranks);
        float value = 2.5f;
        float send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Prod, MPI_COMM_WORLD);
        REQUIRE(send_value == std::pow(value, static_cast<float>(ranks)));
    }

    SECTION("Integer reduction with MPI_LAND")
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int value = (rank == 0) ? 1 : 0;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Land, MPI_COMM_WORLD);
        REQUIRE(send_value == 0);
    }

    SECTION("Integer reduction with MPI_LOR")
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int value = (rank == 0) ? 1 : 0;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Lor, MPI_COMM_WORLD);
        REQUIRE(send_value == 1);
    }

    SECTION("Integer reduction with MPI_BAND")
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int value = (rank == 0) ? 1 : 0;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Band, MPI_COMM_WORLD);
        REQUIRE(send_value == 0);
    }

    SECTION("Integer reduction with MPI_BOR")
    {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int value = (rank == 0) ? 1 : 0;
        int send_value = value;
        reduceAllScalar(&send_value, ReduceOp::Bor, MPI_COMM_WORLD);
        REQUIRE(send_value == 1);
    }
}

TEST_CASE("sendScalar recvScalar Test")
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int rank;
    MPI_Comm_rank(comm, &rank);

    const int size01 = 3;
    const int size10 = 2;
    int send_value01[size01] = {42, -1, 145}; // random values
    int send_value10[size10] = {-58, 234};    // random values
    const int tag = 0;
    MPI_Request requestSend;
    MPI_Request requestReceive;

    if (rank == 0)
    {
        int send_buffer[size01];
        int recv_buffer[size10];
        std::copy(std::begin(send_value01), std::end(send_value01), std::begin(send_buffer));
        sendScalar(send_buffer, size01, 1, tag, comm, &requestSend);
        recvScalar(recv_buffer, size10, 1, tag, comm, &requestReceive);

        while (!test(&requestSend))
        {
            // Busy wait
        }

        while (!test(&requestReceive))
        {
            // Busy wait
        }

        // Check the received value
        for (int i = 0; i < size10; i++)
            REQUIRE(recv_buffer[i] == send_value10[i]);
    }
    else if (rank == 1)
    {

        int send_buffer[size10];
        int recv_buffer[size01];
        std::copy(std::begin(send_value10), std::end(send_value10), std::begin(send_buffer));
        sendScalar(send_buffer, size10, 0, tag, comm, &requestSend);
        recvScalar(recv_buffer, size01, 0, tag, comm, &requestReceive);

        while (!test(&requestSend))
        {
            // Busy wait
        }

        while (!test(&requestReceive))
        {
            // Busy wait
        }

        // Check the received value
        for (int i = 0; i < size01; i++)
            REQUIRE(recv_buffer[i] == send_value01[i]);
    }
}
