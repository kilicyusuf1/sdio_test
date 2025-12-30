//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
//Date        : Tue Dec 30 10:33:19 2025
//Host        : DESKTOP-RERM5SE running 64-bit major release  (build 9200)
//Command     : generate_target system_wrapper.bd
//Design      : system_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module system_wrapper
   (io_cmd_0,
    io_dat_0,
    o_ck_0,
    reset,
    sys_clock,
    usb_uart_rxd,
    usb_uart_txd);
  inout io_cmd_0;
  inout [3:0]io_dat_0;
  output o_ck_0;
  input reset;
  input sys_clock;
  input usb_uart_rxd;
  output usb_uart_txd;

  wire io_cmd_0;
  wire [3:0]io_dat_0;
  wire o_ck_0;
  wire reset;
  wire sys_clock;
  wire usb_uart_rxd;
  wire usb_uart_txd;

  system system_i
       (.io_cmd_0(io_cmd_0),
        .io_dat_0(io_dat_0),
        .o_ck_0(o_ck_0),
        .reset(reset),
        .sys_clock(sys_clock),
        .usb_uart_rxd(usb_uart_rxd),
        .usb_uart_txd(usb_uart_txd));
endmodule
