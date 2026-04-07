#include <iostream>
#include <verilated.h>
#include "Vspi_slave_bfm.h" // Παράγεται αυτόματα από τον Verilator

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vspi_slave_bfm* dut = new Vspi_slave_bfm;

    std::cout << "[SIM] Starting SPI Slave BFM Simulation..." << std::endl;

    // Απλό clock cycle simulation
    dut->clk = 0;
    dut->ncs = 1; // Inactive
    
    for (int i = 0; i < 20; i++) {
        dut->clk = !dut->clk;
        dut->eval();
    }

    std::cout << "[SIM] Simulation finished successfully!" << std::endl;
    delete dut;
    return 0;
}