// license:GPL-2.0+
// copyright-holders: stonedDiscord

/***************************************************************************

  Netlist (modyssey) included from modyssey.cpp

***************************************************************************/

//#ifndef __PLIB_PREPROCESSOR__
	#define NL_PROHIBIT_BASEH_INCLUDE   1
	#include "netlist/devices/net_lib.h"
//#endif

#define FAST_HSYNC (1)
#define FAST_VSYNC (1)
#define CONTROLS (1)
#define WALL (1)
#define GAMELOGIC (1)
#define PLAYER (1)
#define FLIPFLOP (1)
#define MAIN (1)

NETLIST_START(modyssey)
{

	SOLVER(Solver, 100000)
	PARAM(Solver.ACCURACY, 1e-3)

	MAINCLOCK(clk, 100000)

	ANALOG_INPUT(V5, 5.6)
	ALIAS(VCC, V5)

	TTL_INPUT(high, 1)
	TTL_INPUT(low, 0)

	NET_MODEL("NTE5013A  D(BV=6.2 IBV=0.020 NBV=1)")
	NET_MODEL("NTE5014A  D(BV=6.8 IBV=0.020 NBV=1)")
	NET_MODEL("NTE106 PNP(VCEO=15 ICRATING=100m MFG=NTE)")
	// IGNORED SCR1: SCR1 __SCR1
	// IGNORED J20: J20 __J20
	// IGNORED TM2: TM2 __TM2
	// IGNORED TM1: TM1 __TM1


	#if (CONTROLS)
	//HAND CONTROL NO.2
	RES(R3_1, RES_K(47))
	RES(R3_10, RES_K(15))
	RES(R3_12, RES_K(6.8))
	RES(R3_3, RES_K(10))
	RES(R3_4, RES_K(10))
	RES(R3_5, 0)
	RES(R3_7, RES_K(18))
	RES(R3_9, RES_K(15))
	POT(RV3_61, 25000)
	POT(RV3_62, 100000)
	POT(RV3_8, 50000)
	SWITCH(SW3_1)
	SWITCH(SW3_2)
	SWITCH(SW3_3)
	NET_C(VCC, R3_10.1, R3_7.1, R3_1.2)
	NET_C(GND, R3_9.2, R3_12.2, SW3_1.1, SW3_2.1, SW3_3.1)
	NET_C(SW3_1.2, R3_1.1)
	NET_C(SW3_2.2, R3_1.1)
	NET_C(R3_12.1, RV3_61.1)
	NET_C(RV3_62.2, R3_5.2)
	NET_C(R3_3.2, RV3_62.1)
	NET_C(R3_4.2, RV3_62.3)
	NET_C(RV3_8.1, R3_7.2)
	NET_C(R3_10.2, RV3_61.3)
	NET_C(R3_9.1, RV3_8.3)

	//HAND CONTROL NO.1
	RES(R4_1, RES_K(47))
	RES(R4_10, RES_K(15))
	RES(R4_12, RES_K(6.8))
	RES(R4_3, RES_K(10))
	RES(R4_4, RES_K(10))
	RES(R4_5, 0)
	RES(R4_7, RES_K(18))
	RES(R4_9, RES_K(15))
	POT(RV4_61, 25000)
	POT(RV4_62, 100000)
	POT(RV4_8, 50000)
	SWITCH(SW4_1)
	SWITCH(SW4_2)
	SWITCH(SW4_3)
	NET_C(VCC, R4_1.2, R4_7.1, R4_10.1)
	NET_C(GND, R4_12.2, R4_9.2, SW4_1.1, SW4_2.1, SW4_3.1)
	NET_C(SW4_1.2, R4_1.1)
	NET_C(SW4_2.2, R4_1.1)
	NET_C(RV4_62.3, R4_4.2)
	NET_C(R4_3.2, RV4_62.1)
	NET_C(R4_7.2, RV4_8.1)
	NET_C(RV4_61.3, R4_10.2)
	NET_C(R4_9.1, RV4_8.3)
	NET_C(R4_12.1, RV4_61.1)
	#endif

	#if (PLAYER)
	//PLAYER NO.2 GEN
	DIODE(D5_1, "1N914")
	DIODE(D5_2, "1N914")
	QBJT_EB(Q5_1, "NPN")
	QBJT_EB(Q5_2, "NPN")
	QBJT_EB(Q5_3, "NPN")
	QBJT_EB(Q5_4, "NPN")
	CAP(C5_1, CAP_N(1.5))
	CAP(C5_2, CAP_P(120))
	CAP(C5_3, CAP_U(150))
	CAP(C5_4, CAP_U(100))
	CAP(C5_5, 10e-3)
	RES(R5_1, RES_K(10))
	RES(R5_2, RES_K(100))
	RES(R5_3, RES_K(10))
	RES(R5_4, RES_K(2.2))
	RES(R5_5, RES_K(10))
	RES(R5_6, RES_K(270))
	RES(R5_7, RES_K(10))
	RES(R5_8, RES_K(10))
	NET_C(VCC, R5_3.1, R5_7.2, R5_4.1, R5_6.2, R5_2.1)
	NET_C(GND, Q5_4.E, Q5_2.E, Q5_1.E, Q5_3.E)
	NET_C(horiz_in, D5_1.A)
	NET_C(C5_3.2, D5_2.K, C5_5.2, R5_8.1)
	NET_C(C5_1.2, R5_2.2, Q5_1.B)	
	NET_C(R5_6.1, C5_3.1, Q5_3.B)
	NET_C(C5_4.1, R5_7.1, Q5_3.C)
	NET_C(C5_2.1, R5_3.2, Q5_1.C)
	NET_C(C5_1.1, D5_1.K, R5_1.1)
	NET_C(R5_5.1, R5_4.2)

	//PLAYER NO.1 GEN
	DIODE(D6_1, "1N914")
	DIODE(D6_2, "1N914")
	QBJT_EB(Q6_1, "NPN")
	QBJT_EB(Q6_2, "NPN")
	QBJT_EB(Q6_3, "NPN")
	QBJT_EB(Q6_4, "NPN")
	CAP(C6_1, CAP_N(1.5))
	CAP(C6_2, CAP_P(120))
	CAP(C6_3, CAP_U(150))
	CAP(C6_4, CAP_U(100))
	CAP(C6_5, 10e-3)
	RES(R6_1, RES_K(10))
	RES(R6_2, RES_K(100))
	RES(R6_3, RES_K(10))
	RES(R6_4, RES_K(2.2))
	RES(R6_5, RES_K(10))
	RES(R6_6, RES_K(270))
	RES(R6_7, RES_K(10))
	RES(R6_8, RES_K(10))
	NET_C(VCC, R6_6.2, R6_7.2, R6_3.1, R6_2.1, R6_4.1)
	NET_C(GND, Q6_4.E, Q6_3.E, Q6_1.E, Q6_2.E)
	NET_C(horiz_in, D6_1.A)
	NET_C(R6_8.1, C6_5.2, C6_3.2, D6_2.K)
	NET_C(R6_3.2, Q6_1.C, C6_2.1)
	NET_C(D6_1.K, C6_1.1, R6_1.1)
	NET_C(R6_2.2, Q6_1.B, C6_1.2)
	NET_C(R6_6.1, C6_3.1, Q6_3.B)	
	NET_C(C6_4.1, Q6_3.C, R6_7.1)
	NET_C(R6_4.2, R6_5.1)
	#endif

	#if (WALL)
	//WALL GENERATOR
	DIODE(D7_1, "1N914")
	DIODE(D7_2, "1N914")
	QBJT_EB(Q7_1, "NPN")
	QBJT_EB(Q7_2, "NPN")
	QBJT_EB(Q7_3, "NPN")
	QBJT_EB(Q7_4, "NPN")
	CAP(C7_1, CAP_N(1.5))
	CAP(C7_2, CAP_P(120))
	CAP(C7_3, CAP_U(150))
	CAP(C7_4, CAP_U(100))
	CAP(C7_5, 10e-3)
	RES(R7_1, RES_K(10))
	RES(R7_2, RES_K(100))
	RES(R7_3, RES_K(10))
	RES(R7_4, RES_K(2.2))
	RES(R7_5, RES_K(10))
	RES(R7_6, RES_K(270))
	RES(R7_7, RES_K(10))
	RES(R7_8, RES_K(10))
	NET_C(VCC, R7_2.1, R7_6.2, R7_7.2, R7_3.1, R7_4.1)
	NET_C(GND, Q7_1.E, Q7_3.E, Q7_4.E, Q7_2.E)
	NET_C(C7_1.2, R7_2.2, Q7_1.B)
	NET_C(C7_3.2, D7_2.K, R7_8.1, C7_5.2)
	NET_C(R7_7.1, C7_4.1, Q7_3.C)
	NET_C(R7_4.2, R7_5.1)
	NET_C(R7_6.1, C7_3.1, Q7_3.B)
	NET_C(R7_3.2, Q7_1.C, C7_2.1)
	NET_C(C7_1.1, D7_1.K, R7_1.1)

	//BALL GENERATOR
	DIODE(D8_1, "1N914")
	DIODE(D8_2, "1N914")
	QBJT_EB(Q8_1, "NPN")
	QBJT_EB(Q8_2, "NPN")
	QBJT_EB(Q8_3, "NPN")
	QBJT_EB(Q8_4, "NPN")
	CAP(C8_1, CAP_N(1.5))
	CAP(C8_2, CAP_P(120))
	CAP(C8_3, CAP_U(150))
	CAP(C8_4, CAP_U(100))
	CAP(C8_5, 10e-3)
	RES(R8_1, RES_K(10))
	RES(R8_2, RES_K(100))
	RES(R8_3, RES_K(10))
	RES(R8_4, RES_K(2.2))
	RES(R8_5, RES_K(10))
	RES(R8_6, RES_K(270))
	RES(R8_7, RES_K(10))
	RES(R8_8, RES_K(10))
	NET_C(VCC, R8_3.1, R8_7.2, R8_4.1, R8_6.2, R8_2.1)
	NET_C(GND, Q8_1.E, Q8_3.E, Q8_4.E, Q8_2.E)
	NET_C(D8_1.K, R8_1.1, C8_1.1)
	NET_C(R8_7.1, C8_4.1, Q8_3.C)
	NET_C(R8_3.2, Q8_1.C, C8_2.1)
	NET_C(Q8_3.B, C8_3.1, R8_6.1)
	NET_C(C8_5.2, R8_8.1, C8_3.2, D8_2.K)
	NET_C(R8_4.2, R8_5.1)
	#endif

	//vert_in
	#if (FAST_VSYNC)
	CLOCK(vert_in_CLOCK, 60)
	NET_C(vert_in_CLOCK.GND, GND)
	NET_C(vert_in_CLOCK.VCC, VCC)
	ALIAS(vert_in, vert_in_CLOCK.Q)
	#else
	CAP(C6, CAP_P(100))
	CAP(C9_1, CAP_U(100))
	CAP(C9_2, CAP_U(100))
	QBJT_EB(Q9_1, "NPN")
	QBJT_EB(Q9_2, "PNP")
	QBJT_EB(Q9_3, "NPN")
	RES(R24, 100)
	RES(R9_1, RES_K(2.2))
	RES(R9_2, RES_K(180))
	RES(R9_3, 470)
	RES(R9_4, 820)
	RES(R9_5, RES_K(300))
	RES(R9_6, RES_K(2.2))
	RES(R9_7, 470)
	RES(R23, RES_K(200))
	POT(RV39, 100000)
	NET_C(RV39.2, RV39.3, R23.1)
	NET_C(R24.1, C6.1, RV39.1)
	NET_C(VCC, R9_1.2, Q9_2.E, Q9_3.C, R24.2)
	NET_C(GND, Q9_1.E, R9_6.1, R9_5.1, R9_7.1, C6.2)
	NET_C(vert_in, Q9_3.E, R9_7.1)
	NET_C(C9_1.2, R9_4.2)
	NET_C(R9_1.1, Q9_1.C, R9_3.2)
	NET_C(R9_2.2, Q9_1.B, C9_1.1)
	NET_C(R23.2, R9_2.1)
	NET_C(Q9_2.B, C9_2.2, R9_5.2)
	NET_C(R9_3.1, C9_2.1)
	NET_C(Q9_2.C, Q9_3.B, R9_4.1, R9_6.2)
	#endif

	//horiz_in
	#if (FAST_HSYNC)
	CLOCK(horiz_in_CLOCK, 15.734)
	NET_C(horiz_in_CLOCK.GND, GND)
	NET_C(horiz_in_CLOCK.VCC, VCC)
	ALIAS(horiz_in, horiz_in_CLOCK.Q)
	#else
	CAP(C10_1, CAP_P(470))
	CAP(C10_2, CAP_P(680))
	CAP(C10_3, CAP_P(10))
	CAP(C10_4, CAP_P(10))
	IND(L10_1, 0.00062)
	QBJT_EB(Q10_1, "NPN")
	QBJT_EB(Q10_2, "PNP")
	QBJT_EB(Q10_3, "NPN")
	RES(R10_1, RES_K(1))
	RES(R10_2, RES_K(150))
	RES(R10_3, 820)
	RES(R10_4, RES_K(3.3))
	RES(R10_5, RES_K(300))
	RES(R10_6, RES_K(1))
	RES(R10_7, 470)
	RES(R22, RES_K(3.9))
	RES(R37, 0)
	POT(RV38, 50000)
	NET_C(GND, C10_4.2, C10_3.2, Q10_1.E, R10_6.1, R10_5.1, R10_7.1)
	NET_C(Q10_1.C, R10_1.1, R10_3.2)//, D12_7.K)
	NET_C(VCC, RV38.1, L10_1.1, C10_3.1)
	NET_C(R10_4.2, C10_1.2)
	NET_C(R10_3.1, C10_2.1)
	NET_C(Q10_1.B, R10_2.2, C10_1.1)
	NET_C(R22.2, R10_2.1)
	NET_C(R22.1, R37.1)
	NET_C(R37.2, RV38.2, RV38.3)
	NET_C(horiz_in, R10_7.2, Q10_3.E)
	NET_C(L10_1.2, C10_4.1, R10_1.2, Q10_2.E, Q10_3.C)
	NET_C(R10_6.2, R10_4.1, Q10_2.C, Q10_3.B)
	NET_C(R10_5.2, C10_2.2, Q10_2.B)
	#endif

	#if (GAMELOGIC)
	//GATE MATRIX
	DIODE(D11_1, "1N34A")
	DIODE(D11_2, "1N34A")
	DIODE(D11_3, "1N914")
	DIODE(D11_4, "1N914")
	DIODE(D11_5, "1N34A")
	DIODE(D11_6, "1N34A")
	DIODE(D11_7, "1N34A")
	DIODE(D11_9, "1N34A")
	RES(R11_1, RES_K(10))
	RES(R11_2, RES_K(10))
	RES(R11_3, RES_K(10))
	RES(R11_4, RES_K(10))
	RES(R11_5, RES_K(4.7))
	RES(R11_6, RES_K(4.7))
	RES(R11_7, RES_K(4.7))
	NET_C(VCC, R11_6.2, R11_5.2, R11_7.2)
	NET_C(R11_4.1, D11_4.K)
	NET_C(R11_1.1, D11_1.K)
	NET_C(D11_3.K, R11_3.1)
	NET_C(R11_4.2, R11_3.2)
	NET_C(R11_7.1, D11_7.A)
	NET_C(R11_2.1, D11_2.K)

	//SUMMER
	DIODE(D12_1, "1N914")
	DIODE(D12_2, "1N34A")
	DIODE(D12_3, "1N914")
	DIODE(D12_4, "1N34A")
	DIODE(D12_5, "1N914")
	DIODE(D12_6, "1N914")
	DIODE(D12_7, "1N914")
	DIODE(D12_8, "1N914")
	RES(R12_1, 820)
	RES(R12_2, RES_K(27))
	RES(R12_3, RES_K(18))
	RES(R12_4, 0)
	RES(R12_5, 0)
	RES(R12_6, 0)
	RES(R12_7, 0)
	NET_C(R12_1.1, R12_3.1, R12_2.1)
	NET_C(D12_5.K, D12_6.K, D12_8.A, D12_1.K, R12_1.2, D12_3.K, D12_7.A)
	NET_C(R12_7.1, D12_6.A)
	NET_C(R12_6.1, D12_5.A)
	NET_C(R12_4.1, D12_1.A)
	NET_C(R12_5.1, D12_3.A)
	NET_C(D12_7.K, R12_3.2)
	#endif

	#if (FLIPFLOP)
	//FLIP-FLOP/ENGLISH
	CAP(C15_1, CAP_U(100))
	CAP(C15_2, CAP_U(100))
	CAP(C15_3, CAP_P(100))
	CAP(C15_4, CAP_P(100))
	QBJT_EB(Q15_1, "PNP")
	QBJT_EB(Q15_2, "PNP")
	QBJT_EB(Q15_3, "NPN")
	QBJT_EB(Q15_4, "NPN")
	RES(R15_1, RES_K(47))
	RES(R15_2, RES_K(47))
	RES(R15_3, RES_K(2.2))
	RES(R15_4, RES_K(47))
	RES(R15_5, RES_K(47))
	RES(R15_6, RES_K(1))
	RES(R15_7, RES_K(1))
	NET_C(VCC, Q15_1.C, Q15_2.C)
	NET_C(GND, Q15_3.E, Q15_4.E)
	NET_C(C15_1.2, R15_1.1, Q15_2.B)
	NET_C(Q15_1.B, R15_2.2, C15_2.2)

	//FLIP-FLOP/BALL
	CAP(C16_1, CAP_U(100))
	CAP(C16_2, CAP_U(100))
	CAP(C16_3, CAP_P(100))
	CAP(C16_4, CAP_P(100))
	QBJT_EB(Q16_1, "NTE106")
	QBJT_EB(Q16_2, "PNP")
	QBJT_EB(Q16_3, "NPN")
	QBJT_EB(Q16_4, "NPN")
	RES(R16_1, RES_K(47))
	RES(R16_2, RES_K(47))
	RES(R16_3, RES_K(2.2))
	RES(R16_4, RES_K(47))
	RES(R16_5, RES_K(47))
	RES(R16_6, RES_K(1))
	RES(R16_7, RES_K(1))
	NET_C(VCC, Q16_2.C, Q16_1.C)
	NET_C(GND, Q16_4.E, Q16_3.E)
	NET_C(R16_7.1, R16_6.1)
	NET_C(R16_6.2, C16_2.1, R16_2.1, Q16_2.E)
	NET_C(R16_3.2, C16_1.1, R16_1.2, Q16_1.E)
	NET_C(Q16_2.B, C16_1.2, R16_1.1)
	NET_C(C16_2.2, R16_2.2, Q16_1.B)
	NET_C(C16_3.2, Q16_4.B, R16_5.1)
	NET_C(Q16_4.C, R16_7.2, R16_4.1, C16_4.1)
	NET_C(R16_4.2, C16_4.2, Q16_3.B)
	#endif

	#if (MAIN)
	//MAIN BOARD
	CAP(C1, CAP_P(220))
	CAP(C3, CAP_P(10))
	CAP(C4, CAP_P(47))
	CAP(C5, CAP_P(10))
	CAP(C7, CAP_P(10))
	CAP(C8, CAP_P(47))
	CAP(C9, CAP_P(10))
	CAP(C10, CAP_P(10))
	CAP(C11, 1e-3)
	CAP(C13, CAP_U(10))
	CAP(C14, CAP_P(470))
	CAP(C15, CAP_P(470))
	CAP(C16, CAP_P(330))
	CAP(C20, CAP_P(330))
	DIODE(D2, "1N914")
	DIODE(D3, "1N914")
	DIODE(D5, "1N34A")
	DIODE(D6, "1N34A")
	DIODE(D7, "1N34A")
	DIODE(D8, "1N34A")
	DIODE(Z1, "NTE5013A")
	DIODE(Z2, "NTE5014A")
	QBJT_EB(Q1, "NPN")
	QBJT_EB(Q2, "NPN")
	RES(R5, RES_K(10))
	RES(R7, RES_K(27))
	RES(R11, RES_K(15))
	RES(R13, RES_K(33))
	RES(R15, RES_K(47))
	RES(R16, RES_K(10))
	RES(R18, RES_K(27))
	RES(R19, RES_K(130))
	RES(R2, RES_K(2.7))
	RES(R20, RES_K(820))
	RES(R21, RES_K(120))
	RES(R27, RES_K(27))
	RES(R29, RES_K(10))
	RES(R30, RES_K(10))
	RES(R33, RES_K(27))
	RES(R44, RES_K(22))
	RES(R45, RES_K(1.5))
	RES(R46, RES_K(10))
	RES(R9, RES_K(8.2))
	POT(RV12, 25000)
	POT(RV17, 50000)
	POT(RV26, 50000)
	POT(RV28, 15000)
	POT(RV3, 9000)
	POT(RV31, 15000)
	POT(RV32, 50000)
	POT(RV4, 15000)
	POT(RV6, 50000)
	NET_C(VCC, C14.1, Q1.E, R30.2, R33.2, R7.2, R5.2, R21.2, R15.2, R20.1, R18.2, R29.2, R27.2)
	NET_C(GND, Z1.A, C14.2, C10.2, R13.1, C1.2, C16.2, C11.2, Q2.E, R46.1, Z2.A, C20.2, C3.2, C4.2, R19.2, RV12.2, RV12.3, C13.2, C15.2, C5.2, C9.2, C8.2, C7.2)
	NET_C(C9.1, R5_1.2, RV3_61.2)
	NET_C(D6.K, R4_4.1)
	NET_C(D6.A, C15_1.1, R15_1.2, Q15_1.E, R15_3.2)
	NET_C(D5.K, R3_3.1)
	NET_C(R21.1, C7_4.2, Q7_4.B, C7_5.1)
	NET_C(D2.A, R4_1.1)
	NET_C(R29.1, RV28.1)
	NET_C(R16.1, R46.2, R44.1)
	NET_C(RV4.1, R5.1)
	NET_C(R9.1, R4_5.1, R3_5.1)
	NET_C(R18.1, RV17.1)
	NET_C(D8.K, R3_4.1)
	NET_C(C16_3.1, R16_5.2, R16_3.1, Q16_3.C)
	NET_C(D5.A, C15_3.1, R15_5.2, Q15_3.C, R15_3.1)
	NET_C(R20.2, RV17.2, RV17.3, C7_2.2, Q7_2.B)
	NET_C(D7.A, R15_4.1, C15_4.1, R15_7.2, Q15_4.C)
	NET_C(C10.1, R5_8.2, RV3_8.2)
	NET_C(R45.1, Q1.C, C13.1, C15.1)
	NET_C(R11.2, C3.1, R15.1, R7_1.2)
	NET_C(RV31.2, RV31.3, Q5_4.B, C5_4.2, C5_5.1)
	NET_C(Z1.K, R45.2, Q1.B)
	NET_C(RV4.2, RV4.3, C8_4.2, C8_5.1, Q8_4.B)
	NET_C(RV26.1, R27.1)
	NET_C(D8.A, R15_2.1, C15_2.1, R15_6.2, Q15_2.E)
	NET_C(R11_1.2, Q15_3.B, R15_4.2, C15_4.2)
	NET_C(R11.1, RV12.1)
	NET_C(R19.1, C5.1, R7_8.2)
	NET_C(R30.1, RV31.1)
	NET_C(R33.1, RV32.1)
	NET_C(D3.A, R3_1.1)
	NET_C(SW4_3.2, SW3_3.2)
	NET_C(RV4_62.2, R4_5.2)
	NET_C(RV32.2, RV32.3, C5_2.2, Q5_2.B)
	NET_C(R2.1, R15_7.1, R15_6.1)
	NET_C(R9.2, C4.1, R8_8.2)
	NET_C(Q8_4.C, R8_5.2, Q8_2.C, R12_6.2, D11_6.K, D11_5.K, D11_7.K)
	NET_C(C11.1, Q2.C)
	NET_C(RV6.2, RV6.3, C8_2.2, Q8_2.B)
	NET_C(C16.1, R16.2)
	NET_C(R44.2, Z2.K, C20.1)
	NET_C(D12_4.A, D11_9.A, R11_5.1, D11_5.A, D11_3.A, D11_1.A)
	NET_C(RV28.2, RV28.3, Q6_4.B, C6_4.2, C6_5.1)
	NET_C(RV6.1, R7.1)
	NET_C(D12_8.K, R12_2.2)
	NET_C(C8.1, R6_8.2, RV4_8.2)
	NET_C(Q8_1.B, C8_1.2, R8_2.2)
	NET_C(RV26.2, RV26.3, C6_2.2, Q6_2.B)
	NET_C(D7.K, R4_3.1)
	NET_C(vert_in, D8_2.A, D7_2.A, D6_2.A, D5_2.A)
	NET_C(horiz_in, D8_1.A, D7_1.A)
	NET_C(R12_7.2, D11_9.K, R7_5.2, Q7_4.C, Q7_2.C)
	NET_C(R12_4.2, D12_2.K, Q6_4.C, R6_5.2, Q6_2.C)
	NET_C(D12_2.A, D11_4.A, R11_6.1, D11_6.A, D11_2.A)
	NET_C(D3.K, D2.K, Q2.B)
	NET_C(C7.1, R6_1.2, RV4_61.2)
	NET_C(R13.2, RV3.2, RV3.3, C1.1, R8_1.2)
	NET_C(RV3.1, R2.2)
	NET_C(R11_2.2, C15_3.2, R15_5.1, Q15_4.B)
	NET_C(D12_4.K, R12_5.2, Q5_4.C, R5_5.2, Q5_2.C)
	#endif

	ALIAS(videomix, R12_1.1)

	// ----------------------------------------------------------------------------------------
	// power terminals
	// ----------------------------------------------------------------------------------------

	NET_C(VCC, high.VCC, low.VCC)
	NET_C(GND, high.GND, low.GND)
	
}
