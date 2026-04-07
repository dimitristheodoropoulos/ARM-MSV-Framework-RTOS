module spi_slave_bfm (
    /* verilator lint_off UNUSEDSIGNAL */
    input clk,
    /* verilator lint_on UNUSEDSIGNAL */
    input ncs, sclk, mosi,
    output reg miso
);
    reg [7:0] shift = 0;
    
    always @(posedge sclk or posedge ncs) begin
        if (ncs) begin
            shift <= 0;
        end else begin
            shift <= {shift[6:0], mosi};
            miso <= shift[7];
        end
    end
endmodule
