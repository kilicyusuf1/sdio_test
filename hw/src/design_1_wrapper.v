//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
//Date        : Tue Jan 13 09:02:47 2026
//Host        : DESKTOP-RERM5SE running 64-bit major release  (build 9200)
//Command     : generate_target design_1_wrapper.bd
//Design      : design_1_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module design_1_wrapper
   (io_cmd_0,
    io_dat_0,
    o_ck_0,
    reset,
    sys_clock,
    usb_uart_rxd,
    usb_uart_txd,
    sd_reset);
  inout io_cmd_0;
  inout [3:0]io_dat_0;
  output o_ck_0;
  input reset;
  input sys_clock;
  input usb_uart_rxd;
  output usb_uart_txd;
  output wire sd_reset;
  
  wire io_cmd_0;
  wire [3:0]io_dat_0;
  wire o_ck_0;
  wire reset;
  wire sys_clock;
  wire usb_uart_rxd;
  wire usb_uart_txd;
  
  assign sd_reset = 1'b0;
  
  design_1 design_1_i
       (.io_cmd_0(io_cmd_0),
        .io_dat_0(io_dat_0),
        .o_ck_0(o_ck_0),
        .reset(reset),
        .sys_clock(sys_clock),
        .usb_uart_rxd(usb_uart_rxd),
        .usb_uart_txd(usb_uart_txd));
endmodule
