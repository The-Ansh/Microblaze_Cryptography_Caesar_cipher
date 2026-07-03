`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 15.06.2026 17:49:20
// Design Name: 
// Module Name: crypto_core
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


`timescale 1ns / 1ps

module crypto_core(
    input  wire       mode_switch,  // 0 = Encrypt, 1 = Decrypt
    input  wire [7:0] data_in,      // Raw 8-bit character from UART
    output reg  [7:0] data_out      // Transformed character out
);

    always @(*) begin
        if (mode_switch == 1'b0) begin
            // ENCRYPT MODE: Simple Caesar Cipher (Shift values up by 3)
            data_out = data_in + 8'd3;
        end
        else begin
            // DECRYPT MODE: Reverse Shift (Shift values back down by 3)
            data_out = data_in - 8'd3;
        end
    end

endmodule