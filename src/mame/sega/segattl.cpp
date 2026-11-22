// license:BSD-3-Clause
// copyright-holders: Fabio Priuli, Scott Stone

/***************************************************************************

 known Sega Discrete Hardware Games


 Game Name(s)                                                        Part #'s       Data      PROM/ROM Chip Numbers
 ------------------------------------------------------------------+--------------+---------+---------------------------------

    3 Way Block
    Balloon Gun
    Bomber
    Break Open
    Bullet Mark                                                      94359X-P                  PRO-1.106 PRO-3.144
    Cartoon Gun
    Castling
    Crash Course
    Double Block
    Duck Shoot
    Erase
    Fonz                                                             94789-P                   IC86.86 IC87.87 PR-09.49 PR-08.50
    Galaxy War
    Goal Kick
    Heavyweight Champ
    Hockey TV
    Last Inning
    Man TT
    Mini Hockey
    Monaco GP (see monacogp.cpp)
    Moto-Cross
    Pong-Tron
    Pong-Tron II
    Pro Monaco GP
    Pro Racer
    Road Race                                                        94540Y-P
    Rock'n Bark
    Secret Base
    Secret War
    Senkan Yamato
    Space Fighter
    Sparkling Corner
    Squadron
    Super Break Open
    Table Baseball
    Table Hockey
    Top Runner
    Tracer
    Twin Course T.T.
    Upset Block
    Wild Wood
    World Cup
    Zigzag Block


***************************************************************************/


#include "emu.h"

#include "machine/netlist.h"
#include "netlist/devices/net_lib.h"
#include "video/fixfreq.h"

#include "nl_secretbase.h"

namespace {

// copied by Pong, not accurate for this driver!
// start
#define MASTER_CLOCK    7159000
#define V_TOTAL         (0x105+1)       // 262
#define H_TOTAL         (0x1c6+1)       // 454

#define HBSTART                 (H_TOTAL)
#define HBEND                   (80)
#define VBSTART                 (V_TOTAL)
#define VBEND                   (16)

#define HRES_MULT                   (1)
// end


class segattl_state : public driver_device
{
public:
	segattl_state(const machine_config &mconfig, device_type type, const char *tag)
	: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_video(*this, "fixfreq")
	{
	}

	void segattl(machine_config &config) ATTR_COLD;
	void sbase(machine_config &config) ATTR_COLD;

private:
	// devices
	required_device<netlist_mame_cpu_device> m_maincpu;
	required_device<fixedfreq_device> m_video;
};


static NETLIST_START(segattl)
{
	SOLVER(Solver, 48000)
//  PARAM(Solver.FREQ, 48000)
	PARAM(Solver.ACCURACY, 1e-4) // works and is sufficient

	// schematics
	//...

	//  NETDEV_ANALOG_CALLBACK(sound_cb, sound, exidyttl_state, sound_cb, "")
	//  NETDEV_ANALOG_CALLBACK(video_cb, videomix, fixedfreq_device, update_vid, "fixfreq")
}


void segattl_state::segattl(machine_config &config)
{
	// basic machine hardware
	NETLIST_CPU(config, m_maincpu, netlist::config::DEFAULT_CLOCK()).set_source(netlist_segattl);

	// video hardware
	SCREEN(config, "screen");
	FIXFREQ(config, m_video).set_screen("screen");
	m_video->set_monitor_clock(MASTER_CLOCK);
	m_video->set_horz_params(H_TOTAL-67,H_TOTAL-40,H_TOTAL-8,H_TOTAL);
	m_video->set_vert_params(V_TOTAL-22,V_TOTAL-19,V_TOTAL-12,V_TOTAL);
	m_video->set_fieldcount(1);
	m_video->set_threshold(0.30);
}

void segattl_state::sbase(machine_config &config)
{
	segattl(config);
	m_maincpu->set_source(netlist_secretbase);
	// Sample before the coupling capacitor to retain the DC reference for sync.
	NETLIST_ANALOG_OUTPUT(config, "maincpu:video").set_params("VIDEO_MIX", "fixfreq", FUNC(fixedfreq_device::update_composite_monochrome));
	m_video->set_monitor_clock(14'305'000);
	m_video->set_horz_params(720, 812, 876, 909);
	m_video->set_vert_params(240, 247, 251, 261);
	m_video->set_threshold(1.46);
	m_video->set_gain(3.0);
}

/***************************************************************************

 Game driver(s)

 ***************************************************************************/

/*
IC86.86     MMI 6306 (dumped as AMD 27S13)  0390
IC87.87     MMI 6306 (dumped as AMD 27S13)  050D

PR-09.49    TI 74S287
PR-08.50    TI 74S287
*/

ROM_START( bmark )
	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "pro-1.106", 0x000, 0x100, CRC(395fbea5) SHA1(975478fed331594f21aa72e047f2da69991f7795) )
	ROM_LOAD( "pro-3.144", 0x100, 0x100, CRC(5815894e) SHA1(aa879764c4f1775f0ac3b2e75124a483637487ed) )
ROM_END

ROM_START( fonz )
	ROM_REGION( 0x0600, "proms", 0 )
	ROM_LOAD( "ic86.86",  0x000, 0x200, CRC(627a7795) SHA1(88304451ffbd7231d397ada632c7bf6f8cbe7598) )
	ROM_LOAD( "ic87.87",  0x200, 0x200, CRC(d7dd14c7) SHA1(2f112197d7caafb4cb7e2658ea8b298e15aba6ac) )
	ROM_LOAD( "pr-09.49", 0x400, 0x100, CRC(2e293727) SHA1(a03eb4aa726a2e2c872f40dfd1628b00d4edfa33) )
	ROM_LOAD( "pr-08.50", 0x500, 0x100, CRC(6c763af7) SHA1(fdfa310f54b88610c1f59345f5fc72b3b90641ad) )
ROM_END

ROM_START( sbase )
	ROM_REGION( 0x0800, "maincpu:u51", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-108.51", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x0800, "maincpu:u57", ROMREGION_ERASE00 )
	ROM_LOAD( "ic57.57", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x0100, "maincpu:u60", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-18.60", 0x0000, 0x0100, NO_DUMP )

	ROM_REGION( 0x0100, "maincpu:u61", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-19.61", 0x0000, 0x0100, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u82", ROMREGION_ERASE00 )
	ROM_LOAD( "u82", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u131", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-31.131", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u159", ROMREGION_ERASE00 )
	ROM_LOAD( "u159", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u163", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-26.163", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0800, "maincpu:u187", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-107.187", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u193", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-27.193", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0020, "maincpu:u194", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-30.194", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x0800, "maincpu:u207", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-108.207", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x0100, "maincpu:u214", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-16.214", 0x0000, 0x0100, NO_DUMP )

	ROM_REGION( 0x0100, "maincpu:u217", ROMREGION_ERASE00 )
	ROM_LOAD( "pr-17.217", 0x0000, 0x0100, NO_DUMP )
ROM_END

} // anonymous namespace


GAME( 1975, bmark, 0, segattl, 0, segattl_state, empty_init, ROT0, "Sega", "Bullet Mark", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
GAME( 1976, fonz,  0, segattl, 0, segattl_state, empty_init, ROT0, "Sega", "Fonz",        MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
GAME( 1978, sbase, 0, sbase, 0, segattl_state, empty_init, ROT0, "Sega", "Secret Base", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
