`timescale 1ns / 1ps

module axi_ram_sdio_wrapper #(
    // {{{
		// LGFIFO controls the size of the internal FIFOs (in bytes).
		// {{{
		// This also control the maximum amount of data that can be
		// sent or received in any one burst.
		parameter	LGFIFO = 12,
		// }}}
		// NUMIO : Controls the number of data pins available on the
		// {{{
		// interface.  eMMC cards can have up to 8 data pins.  SDIO
		// is limited to 4 data pins.  Both have modes that can support
		// a single data pin alone.  Set appropriately based upon your
		// ultimate hardware.
		parameter	NUMIO=4,
		// }}}
		localparam	MW=32,	// Bus width.  Do not change.
		// ADDRESS_WIDTH: Number of bits to the DMA's address lines,
		// {{{
		// as required to access octets of memory.  This is not the word
		// address width, but the octet/byte address width.
		parameter	ADDRESS_WIDTH=32,
		// }}}
		// Bus data width, DW: Bit-width of the bus, number of bits that
		// {{{
		// can be transferred across the bus (by the DMA) in any one
		// clock cycle.
		parameter	DW=32,
		// }}}
		// Stream data width, SW: Bit-width of the AXI stream inputs and
		// {{{
		// outputs of the IP (when used).  Only some bitwidths are
		// fully supported.  Set to 32 or DW for full support.
		parameter	SW=32,
		// }}}
		// OPT_DMA: Set to 1 to include the DMA in the design
		parameter [0:0]	OPT_DMA = 1'b1,
//`ifdef	SDIO_AXI
		// AXI_IW: ID width of the AXI interface
		parameter	AXI_IW= 1,
		parameter [AXI_IW-1:0]	AXI_WRITE_ID= 1'b0,
		parameter [AXI_IW-1:0]	AXI_READ_ID = 1'b0,
		parameter [0:0]	OPT_LITTLE_ENDIAN = 1'b1,
		parameter	AW = ADDRESS_WIDTH,
//`else
//		parameter [0:0]	OPT_LITTLE_ENDIAN = 1'b0,
//		localparam	AW = ADDRESS_WIDTH-$clog2(DW/8),
//`endif
		parameter	HWDELAY=0,
		// OPT_ISTREAM: Enable an incoming AXI stream to specify data
		// {{{
		// to the DMA, separate from any data that may be read from
		// memory.  This allows streaming applications to bypass the
		// memory interface and go directly to the SD card/EMMC chip
		// if desired.
		parameter [0:0]	OPT_ISTREAM=1'b0,
		// }}}
		// OPT_OSTREAM: Same as OPT_ISTREAM, but for outputs from the
		// {{{
		// SD card/eMMC chip.
		parameter [0:0]	OPT_OSTREAM=1'b0,
		// }}}
		// OPT_EMMC: Enables eMMC support.  There are subtle differences
		// {{{
		// between the eMMC protocol and the SDIO protocol.  This
		// enables the extra features required by eMMC.  These extra
		// features include self-replies to IRQ (interrupt) commands,
		// and data strobe support.
		parameter [0:0]	OPT_EMMC=0,
		// }}}
		// OPT_SERDES && OPT_DDR
		// {{{
		// Three front-end options are available:
		// OPT_DDR == OPT_SERDES == 0: Slowest front end, but requires
		//	no specialized hardware.  May only go up to the system
		//	clock frequency (nominally 100MHz) divided by four.
		// OPT_SERDES == 0, OPT_DDR = 1: A bit faster.  Transitions on
		//	both system clock edges.  May go up to the system
		//	clock frequency divided by two, or nominally 50 MHz.
		//	Will supports DDR data at this rate--assuming the device
		//	does.
		// OPT_SERDES == 1: Fastest clock frequency.  Depends upon
		//	I/OSERDES support.  Support may not be available in all
		//	FPGA vendors.  In this mode, the outgoing clock may
		//	get as high as twice the system clock rate, or nominally
		//	200MHz.  DDR data is supported at this rate.
		parameter [0:0]	OPT_SERDES=0,
		parameter [0:0]	OPT_DDR=1,
		// }}}
		// OPT_DS: Data strobe support
		// {{{
		// eMMC chips include a data strobe pin, which can be used to
		// clock return values.  Set this parameter true to support
		// sampling based upon this data strobe.  Beware that you may
		// need additional timing constraints to make this work.
		parameter [0:0]	OPT_DS=OPT_SERDES && OPT_EMMC,
		// }}}
		parameter [0:0]	OPT_CARD_DETECT=!OPT_EMMC,
		// OPT_CRCTOKEN : Look for a CRC token following every blk write
		// {{{
		// CRC tokens are returned by both eMMC and SD card devices
		// following block writes from the host to the card.  The token
		// tells the host whether or not the block was written validly
		// or not.
		//
		// At one time, I thought this these tokens were optional, then
		// that they were only on eMMC devices.  The parameter was built
		// so I could first have that optional support, then so that
		// support could be optionally configured in.  Now I understand
		// both eMMC and SD card devices use these toksn.  Therefore,
		// this parameter should be set and left.
		parameter [0:0]	OPT_CRCTOKEN=1'b1,
		// }}}
		// OPT_HWRESET
		// {{{
		// eMMC cards can have hardware resets.  SD Cards do not.  Set
		// OPT_HWRESET to include control logic to set the hardware
		// reset.
		parameter [0:0]	OPT_HWRESET = OPT_EMMC,
		// }}}
		// OPT_1P8V
		// {{{
		// Some protocols require switching voltages during the
		// handshaking process in order to switch to higher speeds.
		// OPT_1P8V enables a GPIO output which can be used to
		// request that external hardware (not part of this package)
		// switch the IO voltage to 1.8V from 3.3V.  No feedback is
		// provided, it simply controls a GPIO pin that is assumed to
		// command a 1.8V output.
		parameter [0:0]	OPT_1P8V=1,
		// }}}
		// OPT_COLLISION
		// {{{
		// eMMC is (supposed to) support IRQ (interrupts).  A specific
		// command tells the eMMC chip to wait for an internal interrupt
		// before responding to the command.  Hence, this interrupt
		// command is supposed to end with the device sending a command
		// reply.  However, according to protocol the host may pre-empt
		// this wait by replying to its own command.  If, however,
		// both attempt to send a command reply at the same time, there
		// is a possibility for a collision.  OPT_COLLISION adds logic
		// to detect such collisions and to get off the bus should any
		// such happen.  Detecting collisions requires a solid
		// knowledge internal to the front end about the delay through
		// the system, to avoid false alarms.
		//
		// NOTE: Collisions detection does not (currently) work with
		//   OPT_SERDES.
		parameter [0:0]	OPT_COLLISION=OPT_EMMC && !OPT_SERDES,
		// }}}
		// LGTIMEOUT
		// {{{
		// Controls how long to wait for a request from the device
		// before giving up with a timeout error.  LGTIMEOUT=23 will
		// wait for (approximately) 2^23 or 8-Million clock cycles.
		// If the device doesn't respond in this time, an internal
		// timeout will be generated, and the command will
		// fail--allowing software the opportunity to attempt to
		// resurrect the interface or give up on it at software's
		// choice.  Examples of what might cause such a timeout failure
		// include an unplugged card, a malfunctioning card, or a
		// bad board connection.
		parameter	LGTIMEOUT = 23,
		// }}}
		// }}}

        //AXI Dual Port RAM Parameters

        // Width of data bus in bits
        parameter DATA_WIDTH = 32,
        // Width of address bus in bits
        parameter ADDR_WIDTH = 16,
        // Width of wstrb (width of data bus in words)
        parameter STRB_WIDTH = (DATA_WIDTH/8),
        // Width of ID signal
        parameter ID_WIDTH = 8,
        // Extra pipeline register on output port A
        parameter A_PIPELINE_OUTPUT = 0,
        // Extra pipeline register on output port B
        parameter B_PIPELINE_OUTPUT = 0,
        // Interleave read and write burst cycles on port A
        parameter A_INTERLEAVE = 0,
        // Interleave read and write burst cycles on port B
        parameter B_INTERLEAVE = 0

)
    (
        input wire			i_clk, i_reset, i_hsclk,

        output wire debug_aw_hit,
        output wire debug_w_hit,
        output wire debug_ar_hit,
        output wire debug_r_hit,
        // (Optional) AXI-Lite interface
        // {{{
        input	wire		S_AXIL_AWVALID,
        output	wire		S_AXIL_AWREADY,
        input	wire	[4:0]	S_AXIL_AWADDR,
        input	wire	[2:0]	S_AXIL_AWPROT,
        //
        input	wire		S_AXIL_WVALID,
        output	wire		S_AXIL_WREADY,
        input	wire	[31:0]	S_AXIL_WDATA,
        input	wire	[3:0]	S_AXIL_WSTRB,
        //
        output	wire		S_AXIL_BVALID,
        input	wire		S_AXIL_BREADY,
        output	wire	[1:0]	S_AXIL_BRESP,
        //
        input	wire		S_AXIL_ARVALID,
        output	wire		S_AXIL_ARREADY,
        input	wire	[4:0]	S_AXIL_ARADDR,
        input	wire	[2:0]	S_AXIL_ARPROT,
        //
        output	wire		S_AXIL_RVALID,
        input	wire		S_AXIL_RREADY,
        output	wire	[31:0]	S_AXIL_RDATA,
        output	wire	[1:0]	S_AXIL_RRESP,
        // }}}

        // External DMA streaming interface
		// {{{
		input	wire			s_valid,
		output	wire			s_ready,
		input	wire	[SW-1:0]	s_data,
		//
		output	wire			m_valid,
		input	wire			m_ready,
		output	wire	[SW-1:0]	m_data,
		output	wire			m_last,
		// }}}

		// IO interface
		// {{{
		output	wire			o_ck,
		input	wire			i_ds,

		inout	wire			io_cmd,
		inout	wire	[NUMIO-1:0]	io_dat,

        // }}}
		input	wire		i_card_detect,
		output	wire		o_hwreset_n,
		output	wire		o_1p8v,
		input	wire		i_1p8v,
		output	wire		o_int,
		output	wire	[31:0]	o_debug,
		// }}}

        // AXI Dual Port RAM output for Microblaze (or processor) connection  
        input  wire [ID_WIDTH-1:0]    s_axi_b_awid,
        input  wire [ADDR_WIDTH-1:0]  s_axi_b_awaddr,
        input  wire [7:0]             s_axi_b_awlen,
        input  wire [2:0]             s_axi_b_awsize,
        input  wire [1:0]             s_axi_b_awburst,
        input  wire                   s_axi_b_awlock,
        input  wire [3:0]             s_axi_b_awcache,
        input  wire [2:0]             s_axi_b_awprot,
        input  wire                   s_axi_b_awvalid,
        output wire                   s_axi_b_awready,
        input  wire [DATA_WIDTH-1:0]  s_axi_b_wdata,
        input  wire [STRB_WIDTH-1:0]  s_axi_b_wstrb,
        input  wire                   s_axi_b_wlast,
        input  wire                   s_axi_b_wvalid,
        output wire                   s_axi_b_wready,
        output wire [ID_WIDTH-1:0]    s_axi_b_bid,
        output wire [1:0]             s_axi_b_bresp,
        output wire                   s_axi_b_bvalid,
        input  wire                   s_axi_b_bready,
        input  wire [ID_WIDTH-1:0]    s_axi_b_arid,
        input  wire [ADDR_WIDTH-1:0]  s_axi_b_araddr,
        input  wire [7:0]             s_axi_b_arlen,
        input  wire [2:0]             s_axi_b_arsize,
        input  wire [1:0]             s_axi_b_arburst,
        input  wire                   s_axi_b_arlock,
        input  wire [3:0]             s_axi_b_arcache,
        input  wire [2:0]             s_axi_b_arprot,
        input  wire                   s_axi_b_arvalid,
        output wire                   s_axi_b_arready,
        output wire [ID_WIDTH-1:0]    s_axi_b_rid,
        output wire [DATA_WIDTH-1:0]  s_axi_b_rdata,
        output wire [1:0]             s_axi_b_rresp,
        output wire                   s_axi_b_rlast,
        output wire                   s_axi_b_rvalid,
        input  wire                   s_axi_b_rready

    );

    wire			M_AXI_AWVALID;
    wire			M_AXI_AWREADY;
    wire [AXI_IW-1:0]	M_AXI_AWID;
    wire [AW-1:0]		M_AXI_AWADDR;
    wire [7:0]		M_AXI_AWLEN;
    wire [2:0]		M_AXI_AWSIZE;
    wire [1:0]		M_AXI_AWBURST;
    wire 			M_AXI_AWLOCK;
    wire [3:0]		M_AXI_AWCACHE;
    wire	[2:0]		M_AXI_AWPROT;
    wire [3:0]		M_AXI_AWQOS;
    wire			M_AXI_WVALID;
    wire			M_AXI_WREADY;
    wire [DW-1:0]		M_AXI_WDATA;
    wire [DW/8-1:0]		M_AXI_WSTRB;
    wire			M_AXI_WLAST;
    wire			M_AXI_BVALID;
    wire			M_AXI_BREADY;
    wire [AXI_IW-1:0]	M_AXI_BID;
    wire	[1:0]		M_AXI_BRESP;
    wire			M_AXI_ARVALID;
    wire			M_AXI_ARREADY;
    wire [AXI_IW-1:0]	M_AXI_ARID;
    wire [AW-1:0]		M_AXI_ARADDR;
    wire [7:0]		M_AXI_ARLEN;
    wire [2:0]		M_AXI_ARSIZE;
    wire [1:0]		M_AXI_ARBURST;
    wire 			M_AXI_ARLOCK;
    wire [3:0]		M_AXI_ARCACHE;
    wire	[2:0]		M_AXI_ARPROT;
    wire [3:0]		M_AXI_ARQOS;
    wire			M_AXI_RVALID;
    wire			M_AXI_RREADY;
    wire [AXI_IW-1:0]	M_AXI_RID;
    wire [DW-1:0]		M_AXI_RDATA;
    wire			M_AXI_RLAST;
    wire	[1:0]		M_AXI_RRESP;

    sdio_top #(

        .LGFIFO(LGFIFO), .NUMIO(NUMIO),
		.ADDRESS_WIDTH(ADDRESS_WIDTH),
		.DW(DW), .SW(SW),
		.OPT_DMA(OPT_DMA),
		.OPT_ISTREAM(OPT_ISTREAM), .OPT_OSTREAM(OPT_OSTREAM),
//`ifdef	SDIO_AXI
		.AXI_IW(AXI_IW),
		.AXI_WRITE_ID(AXI_WRITE_ID),
		.AXI_READ_ID(AXI_READ_ID),
        .AW(AW),
        .HWDELAY(HWDELAY),
//`endif
		.OPT_LITTLE_ENDIAN(OPT_LITTLE_ENDIAN),
		.OPT_DDR(OPT_DDR), .OPT_SERDES(OPT_SERDES),
		.OPT_DS(OPT_DS),
		.OPT_CARD_DETECT(OPT_CARD_DETECT),
		.OPT_EMMC(OPT_EMMC),
		.OPT_CRCTOKEN(OPT_CRCTOKEN),
		.OPT_HWRESET(OPT_HWRESET),
		.OPT_1P8V(OPT_1P8V),
		.LGTIMEOUT(LGTIMEOUT),
        .OPT_COLLISION(OPT_COLLISION) 

    ) inst_sdio_top (

        .i_clk(i_clk),
        .i_reset(i_reset),
        .i_hsclk(i_hsclk),

        .S_AXIL_AWVALID(S_AXIL_AWVALID),
        .S_AXIL_AWREADY(S_AXIL_AWREADY),
        .S_AXIL_AWADDR(S_AXIL_AWADDR),
        .S_AXIL_AWPROT(S_AXIL_AWPROT),

        .S_AXIL_WVALID(S_AXIL_WVALID),
        .S_AXIL_WREADY(S_AXIL_WREADY),
        .S_AXIL_WDATA(S_AXIL_WDATA),
        .S_AXIL_WSTRB(S_AXIL_WSTRB),

        .S_AXIL_BVALID(S_AXIL_BVALID),
        .S_AXIL_BREADY(S_AXIL_BREADY),
        .S_AXIL_BRESP(S_AXIL_BRESP),

        .S_AXIL_ARVALID(S_AXIL_ARVALID),
        .S_AXIL_ARREADY(S_AXIL_ARREADY),
        .S_AXIL_ARADDR(S_AXIL_ARADDR),
        .S_AXIL_ARPROT(S_AXIL_ARPROT),

        .S_AXIL_RVALID(S_AXIL_RVALID),
        .S_AXIL_RREADY(S_AXIL_RREADY),
        .S_AXIL_RDATA(S_AXIL_RDATA),
        .S_AXIL_RRESP(S_AXIL_RRESP),

        .s_valid(s_valid),
		.s_ready(s_ready),
	    .s_data(s_data),

		.m_valid(m_valid),
		.m_ready(m_ready),
    	.m_data(m_data),
		.m_last(m_last),

        .o_ck(o_ck),
        .i_ds(i_ds),

        .io_cmd(io_cmd),
        .io_dat(io_dat),

        .i_card_detect(i_card_detect),
        .o_hwreset_n(o_hwreset_n),
        .o_1p8v(o_1p8v),
        .i_1p8v(i_1p8v),
        .o_int(o_int),
        .o_debug(o_debug),

        //DMA
        .M_AXI_AWVALID(M_AXI_AWVALID),
        .M_AXI_AWREADY(M_AXI_AWREADY),
        .M_AXI_AWID(M_AXI_AWID),
        .M_AXI_AWADDR(M_AXI_AWADDR),
        .M_AXI_AWLEN(M_AXI_AWLEN),
        .M_AXI_AWSIZE(M_AXI_AWSIZE),
        .M_AXI_AWBURST(M_AXI_AWBURST),
        .M_AXI_AWLOCK(M_AXI_AWLOCK),
        .M_AXI_AWCACHE(M_AXI_AWCACHE),
        .M_AXI_AWPROT(M_AXI_AWPROT),
        .M_AXI_AWQOS(M_AXI_AWQOS),

        .M_AXI_WVALID(M_AXI_WVALID),
        .M_AXI_WREADY(M_AXI_WREADY),
        .M_AXI_WDATA(M_AXI_WDATA),
        .M_AXI_WSTRB(M_AXI_WSTRB),
        .M_AXI_WLAST(M_AXI_WLAST),

        .M_AXI_BVALID(M_AXI_BVALID),
        .M_AXI_BREADY(M_AXI_BREADY),
        .M_AXI_BID(M_AXI_BID),
        .M_AXI_BRESP(M_AXI_BRESP),

        .M_AXI_ARVALID(M_AXI_ARVALID),
        .M_AXI_ARREADY(M_AXI_ARREADY),
        .M_AXI_ARID(M_AXI_ARID),
        .M_AXI_ARADDR(M_AXI_ARADDR),
        .M_AXI_ARLEN(M_AXI_ARLEN),
        .M_AXI_ARSIZE(M_AXI_ARSIZE),
        .M_AXI_ARBURST(M_AXI_ARBURST),
        .M_AXI_ARLOCK(M_AXI_ARLOCK),
        .M_AXI_ARCACHE(M_AXI_ARCACHE),
        .M_AXI_ARPROT(M_AXI_ARPROT),
        .M_AXI_ARQOS(M_AXI_ARQOS),

        .M_AXI_RVALID(M_AXI_RVALID),
        .M_AXI_RREADY(M_AXI_RREADY),
    	.M_AXI_RID(M_AXI_RID),
        .M_AXI_RDATA(M_AXI_RDATA),
        .M_AXI_RLAST(M_AXI_RLAST),
        .M_AXI_RRESP(M_AXI_RRESP)

    );

    axi_dp_ram #(

        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH),
        .STRB_WIDTH(STRB_WIDTH),
        .ID_WIDTH(AXI_IW),
        .A_PIPELINE_OUTPUT(A_PIPELINE_OUTPUT),
        .B_PIPELINE_OUTPUT(B_PIPELINE_OUTPUT),
        .A_INTERLEAVE(A_INTERLEAVE),
        .B_INTERLEAVE(B_INTERLEAVE)

    ) inst_axi_dp_ram(

        .a_clk(i_clk),
        .a_rst(i_reset),

        .b_clk(i_clk),
        .b_rst(i_reset),

        //DMA connections
        .s_axi_a_awid(M_AXI_AWID),
        .s_axi_a_awaddr(M_AXI_AWADDR[ADDR_WIDTH-1:0]),
        .s_axi_a_awlen(M_AXI_AWLEN),
        .s_axi_a_awsize(M_AXI_AWSIZE),
        .s_axi_a_awburst(M_AXI_AWBURST),
        .s_axi_a_awlock(M_AXI_AWLOCK),
        .s_axi_a_awcache(M_AXI_AWCACHE),
        .s_axi_a_awprot(M_AXI_AWPROT),
        .s_axi_a_awvalid(M_AXI_AWVALID),
        .s_axi_a_awready(M_AXI_AWREADY),
        .s_axi_a_wdata(M_AXI_WDATA),
        .s_axi_a_wstrb(M_AXI_WSTRB),
        .s_axi_a_wlast(M_AXI_WLAST),
        .s_axi_a_wvalid(M_AXI_WVALID),
        .s_axi_a_wready(M_AXI_WREADY),
        .s_axi_a_bid(M_AXI_BID),
        .s_axi_a_bresp(M_AXI_BRESP),
        .s_axi_a_bvalid(M_AXI_BVALID),
        .s_axi_a_bready(M_AXI_BREADY),
        .s_axi_a_arid(M_AXI_ARID),
        .s_axi_a_araddr(M_AXI_ARADDR[ADDR_WIDTH-1:0]),
        .s_axi_a_arlen(M_AXI_ARLEN),
        .s_axi_a_arsize(M_AXI_ARSIZE),
        .s_axi_a_arburst(M_AXI_ARBURST),
        .s_axi_a_arlock(M_AXI_ARLOCK),
        .s_axi_a_arcache(M_AXI_ARCACHE),
        .s_axi_a_arprot(M_AXI_ARPROT),
        .s_axi_a_arvalid(M_AXI_ARVALID),
        .s_axi_a_arready(M_AXI_ARREADY),
        .s_axi_a_rid(M_AXI_RID),
        .s_axi_a_rdata(M_AXI_RDATA),
        .s_axi_a_rresp(M_AXI_RRESP),
        .s_axi_a_rlast(M_AXI_RLAST),
        .s_axi_a_rvalid(M_AXI_RVALID),
        .s_axi_a_rready(M_AXI_RREADY),

        .s_axi_b_awid(s_axi_b_awid),
        .s_axi_b_awaddr(s_axi_b_awaddr),
        .s_axi_b_awlen(s_axi_b_awlen),
        .s_axi_b_awsize(s_axi_b_awsize),
        .s_axi_b_awburst(s_axi_b_awburst),
        .s_axi_b_awlock(s_axi_b_awlock),
        .s_axi_b_awcache(s_axi_b_awcache),
        .s_axi_b_awprot(s_axi_b_awprot),
        .s_axi_b_awvalid(s_axi_b_awvalid),
        .s_axi_b_awready(s_axi_b_awready),
        .s_axi_b_wdata(s_axi_b_wdata),
        .s_axi_b_wstrb(s_axi_b_wstrb),
        .s_axi_b_wlast(s_axi_b_wlast),
        .s_axi_b_wvalid(s_axi_b_wvalid),
        .s_axi_b_wready(s_axi_b_wready),
        .s_axi_b_bid(s_axi_b_bid),
        .s_axi_b_bresp(s_axi_b_bresp),
        .s_axi_b_bvalid(s_axi_b_bvalid),
        .s_axi_b_bready(s_axi_b_bready),
        .s_axi_b_arid(s_axi_b_arid),
        .s_axi_b_araddr(s_axi_b_araddr),
        .s_axi_b_arlen(s_axi_b_arlen),
        .s_axi_b_arsize(s_axi_b_arsize),
        .s_axi_b_arburst(s_axi_b_arburst),
        .s_axi_b_arlock(s_axi_b_arlock),
        .s_axi_b_arcache(s_axi_b_arcache),
        .s_axi_b_arprot(s_axi_b_arprot),
        .s_axi_b_arvalid(s_axi_b_arvalid),
        .s_axi_b_arready(s_axi_b_arready),
        .s_axi_b_rid(s_axi_b_rid),
        .s_axi_b_rdata(s_axi_b_rdata),
        .s_axi_b_rresp(s_axi_b_rresp),
        .s_axi_b_rlast(s_axi_b_rlast),
        .s_axi_b_rvalid(s_axi_b_rvalid),
        .s_axi_b_rready(s_axi_b_rready)

    );

// --- DEBUG LATCH LOGIC ---
reg debug_aw_hit_reg = 1'b0;
reg debug_w_hit_reg  = 1'b0;
reg debug_ar_hit_reg = 1'b0;
reg debug_r_hit_reg  = 1'b0;

always @(posedge i_clk) begin
    if (i_reset) begin
        debug_aw_hit_reg <= 1'b0;
        debug_w_hit_reg  <= 1'b0;
        debug_ar_hit_reg <= 1'b0;
        debug_r_hit_reg  <= 1'b0;
    end else begin
        // Yazma Adres hs (AW)
        if (M_AXI_AWVALID && M_AXI_AWREADY) debug_aw_hit_reg <= 1'b1;
        
        // Yazma Veri hs (W)
        if (M_AXI_WVALID && M_AXI_WREADY)   debug_w_hit_reg  <= 1'b1;
        
        // Okuma Adres hs (AR)
        if (M_AXI_ARVALID && M_AXI_ARREADY) debug_ar_hit_reg <= 1'b1;
        
        // Okuma Veri hs (R)
        if (M_AXI_RVALID && M_AXI_RREADY)   debug_r_hit_reg  <= 1'b1;
    end
end

assign debug_aw_hit = debug_aw_hit_reg;
assign debug_w_hit  = debug_w_hit_reg;
assign debug_ar_hit = debug_ar_hit_reg;
assign debug_r_hit  = debug_r_hit_reg;

endmodule
