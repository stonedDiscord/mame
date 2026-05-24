// license:BSD-3-Clause
// copyright-holders:Roberto Fresca
/***************************************************************************

  MANN, OH-MANN
  199? - MERKUR

  Preliminary driver by Roberto Fresca.


  German board game similar to Ludo, derived from the Indian game Parchisi.
  Coin-operated machine for 1-4 players. No screen, just artwork and lamps.
  The machine was designed for pubs, etc...

  Field: 93 cm.
  High:  105 cm.

  1x keylock.
  Accept DM and Euro.


  It's all a challenge. Even once emulated, the game will need a lot of
  artwork and lamps work...

  Currently sits in a loop between 0x100000 and 0x600006 addresses r/w,
  the snippet is there:

  001BB8: move.b  (A2), D0
  001BBA: jsr     $6dc0.l
  001BC0: tst.b   D0
  001BC2: bne     $1bb8

  Passing this loop it checks the i/o stuff, including the sound addresses

****************************************************************************

  Hardware Notes...

  - XTAL1 = 8.000 MHz.
  - XTAL2 = 3.6864 MHz.

  1x MC68000P8        ; Motorola, 16-bits CPU.
  1x SAA1099P         ; Philips, 6-Voice Sound Generator.
  2x LC3664BL-10      ; Sanyo, 64K Static RAM.
  1x M62X42B          : OKI, Real Time Clock with built in crystal.
  1x MC68230P8        ; Motorola, Parallel Interface / Timer.
  1x SCN68681C1N40    ; Philips, Dual Asynchronous Receiver/Transmitter (DUART).
  1x MAX696CP         ; Maxim, Microprocessor Supervisory Circuits.


  PCB Layout:
  .------------------------------------------------------.
  | .-------------.    .-----.    .---------.            |
  | |:::::::::::::|    |:::::|    |:::::::::|            |
  | '-------------'    '-----'    '---------'            |
  |         .------------------------.  .-------.        |
  | .-.     |       MC68230P8        |  | L4962 |        |
  | |.|     |         1C10R          |  '-------'        |
  | |.|     |         WC9336         |                   |
  |R|.|     '------------------------'                   |
  |E|.|                                                  |
  |S|.|                                                  |
  |E|.| .---------.      .-----------.                   |
  |R|.| |74HC245N |      | POWER     |                   |
  |V|.| '---------'      |  MODULE   |                   |
  |E|.|                  |           |                   |
  | |.| .---------.      |  3 VOLTS  |    .--------.     |
  | |.| |74HC273B1|      |           |    |MAX696CP|     |
  | '-' '---------'      |           |    '--------'     |
  |                      |           |                   |
  |     .--------.       '-----------'                   |
  |     |74HC4094|                                       |
  |     '--------' .-------------.   .-------------.     |
  |                |    SANYO    |   |    SANYO    |     |
  |  .--------.    | LC3664BL-10 |   | LC3664BL-10 |     |
  |  |74HC04B1|    |             |   |             |     |
  |  '--------'    '-------------'   '-------------'     |
  |  .--------.                                          |
  |  |74HC164B|                                          |
  |  '--------'    .-------------.   .-------------.     |
  |                |Mann,oh-Mann |   |Mann,oh-Mann |     |
  |                |Austria      |   |Austria      |     |
  |   .---. XTAL1  |Vorserie II  |   |Vorserie I   |     |
  |                '-------------'   '-------------'     |
  |  .--------.      .---------.       .---------.       |
  |  |74HC04B1|      |74HC245N |       |74HC245N |       |
  |  '--------'      '---------'       '---------'       |
  |                   .........         .........        |
  |                    8x10K             8x10K           |
  |  .--------.                                          |
  |  |74HC139N|    .--------------------------------.    |
  |  '--------'    |                                |    |
  |                |           MC68000P8            |    |
  |                |                                |    |
  |  .--------.    |                                |    |
  |  |74HC30B1|    '--------------------------------'    |
  |  '--------'          .........  .........            |
  |                        8x10K      8x10K              |
  |  .--------.        .--------.  .----------.          |
  |  |74HC32N |        |74HC138B|  | 74HC245N |          |
  |  '--------'        '--------'  '----------'          |
  |  .--------.       .---------.                        |
  |  |74HC00B1|       | M62X42B |                        |
  |  '--------'       '---------'                        |
  |  .--------.       .---------.                        |
  |  |74HC74B1|       |SAA1099P |                        |
  |  '--------'       '---------'                        |
  |           .---. XTAL2                                |
  | . .------------------------.                         |
  |8. |                        |                         |
  |x. |     SCN68681C1N40      |                         |
  |1. |                        |                         |
  |0. '------------------------'                         |
  |K.     .........                                      |
  | .       8x10K                                        |
  |  .--------.  .--------.  .--------.  .----.  .--.    |
  |  |::::::::|  |::::::::|  |::::::::|  |::::|  |::|    |
  |  '--------'  '--------'  '--------'  '----'  '--'    |
  |   SERVICE                 SERIAL1   SERIAL2 SPEAKER  |
  '------------------------------------------------------'


****************************************************************************

  Memory Map:
  -----------

  000000-01FFFF   ROM Space.
  500000-503FFF   RAM.


***************************************************************************/

#include "emu.h"

#include "bus/rs232/rs232.h"
#include "cpu/m68000/m68000.h"
#include "machine/68230pit.h"
#include "machine/mc68681.h"
#include "machine/msm6242.h"
#include "machine/nvram.h"
#include "machine/watchdog.h"
#include "sound/saa1099.h"
#include "emupal.h"
#include "screen.h"
#include "speaker.h"


namespace {

class manohman_state : public driver_device
{
public:
	manohman_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_duart(*this, "duart"),
    m_rs232(*this, "rs232"),
		m_pit(*this, "pit"),
    m_watchdog(*this, "watchdog"),
    m_screen(*this, "screen"),
    m_palette(*this, "palette"),
    m_workram(*this, "nvram"),
    m_sw1(*this, "SW1"),
    m_sw2(*this, "SW2")
	{ }

	void manohman(machine_config &config);

private:
	virtual void machine_start() override ATTR_COLD;
	void mem_map(address_map &map) ATTR_COLD;
	void cpu_space_map(address_map &map) ATTR_COLD;

	required_device<cpu_device> m_maincpu;
	required_device<mc68681_device> m_duart;
	required_device<rs232_port_device> m_rs232;
	required_device<pit68230_device> m_pit;
  required_device<watchdog_timer_device> m_watchdog;
  required_device<screen_device> m_screen;
  required_device<palette_device> m_palette;
  required_shared_ptr<uint16_t> m_workram;
  required_ioport m_sw1;
  required_ioport m_sw2;

  uint16_t m_data[8] = {0,0,0,0,0,0,0,0};
  uint8_t m_latch = 0;
  uint8_t m_zeilen = 0; // 0-7
  uint16_t m_spalten = 0; // 12 bits, 0-2 are & with en
  //uint8_t m_st = 0;
  //uint8_t m_din = 0;

  void serienplan_w(uint8_t data);
  void lamps_w(uint16_t data);
  void enable_w(uint8_t data);

  uint8_t pit_pa_r();
  void pit_pa_w(uint8_t data);
  uint8_t pit_pb_r();
  void pit_pb_w(uint8_t data);
  uint8_t pit_pc_r();
  void pit_pc_w(uint8_t data);

  uint8_t duart_in_r();
  void duart_out_w(uint8_t data);
  uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


void manohman_state::machine_start()
{
  save_item(NAME(m_data));
  save_item(NAME(m_zeilen));
  save_item(NAME(m_spalten));
}


/*********************************************
*           Memory Map Definition            *
*********************************************/

void manohman_state::mem_map(address_map &map)
{
	map(0x000000, 0x01ffff).rom();
	map(0x100000, 0x10003f).rw(m_pit, FUNC(pit68230_device::read), FUNC(pit68230_device::write)).umask16(0x00ff);
	map(0x200000, 0x20001f).rw(m_duart, FUNC(mc68681_device::read), FUNC(mc68681_device::write)).umask16(0x00ff);
	map(0x300000, 0x300003).w("saa", FUNC(saa1099_device::write)).umask16(0x00ff).nopr();
	map(0x400000, 0x40001f).rw("rtc", FUNC(msm6242_device::read), FUNC(msm6242_device::write)).umask16(0x00ff);
	map(0x500000, 0x503fff).ram().share("nvram"); //work RAM
	map(0x600002, 0x600003).w(FUNC(manohman_state::serienplan_w)); // output through shift register?
	map(0x600004, 0x600005).nopr();
  map(0x600006, 0x600007).nopr();
	map(0x600006, 0x600007).w(FUNC(manohman_state::enable_w));
}

void manohman_state::cpu_space_map(address_map &map)
{
	map(0xfffff0, 0xffffff).m(m_maincpu, FUNC(m68000_base_device::autovectors_map));
	map(0xfffff4, 0xfffff5).r(m_pit, FUNC(pit68230_device::irq_tiack));
	map(0xfffff8, 0xfffff9).r(m_duart, FUNC(mc68681_device::get_irq_vector));
}

void manohman_state::serienplan_w(uint8_t data)
{
  m_data[0] = (m_data[0] << 1) | BIT(data,0);
  m_data[1] = (m_data[1] << 1) | BIT(data,1);
  m_data[2] = (m_data[2] << 1) | BIT(data,2);
  m_data[3] = (m_data[3] << 1) | BIT(data,3);
  m_data[4] = (m_data[4] << 1) | BIT(data,4);
  m_data[5] = (m_data[5] << 1) | BIT(data,5);
  m_data[6] = (m_data[6] << 1) | BIT(data,6);
  m_data[7] = (m_data[7] << 1) | BIT(data,7);
}

void manohman_state::enable_w(uint8_t data)
{
  lamps_w(m_data[0]);
}

void manohman_state::lamps_w(uint16_t data)
{
  m_zeilen = data & 7;
  m_spalten = (data >> 3) & 0xfff;
}

uint8_t manohman_state::pit_pa_r()
{
  //logerror("%06x: PIT PA read\n", m_maincpu->pc()); //buttons in?
  // NC ?
  return 0x00;
}

void manohman_state::pit_pa_w(uint8_t data)
{
  //logerror("%06x: PIT PA write %02x\n", m_maincpu->pc(), data); //lamps out?
  // NC ?
}

uint8_t manohman_state::pit_pb_r()
{
  if ((m_latch >> 4) == 4)
    return m_sw1->read();
  else if ((m_latch >> 4) == 5)
    return m_sw2->read();
  return 0x00;
}

void manohman_state::pit_pb_w(uint8_t data)
{
  logerror("%06x: PIT PB write %02x\n", m_maincpu->pc(), data); //reserved
}

uint8_t manohman_state::pit_pc_r()
{
  logerror("%06x: PIT PC read\n", m_maincpu->pc()); //coins
  return 0x00;
}

void manohman_state::pit_pc_w(uint8_t data)
{
  logerror("%06x: PIT PC write %02x\n", m_maincpu->pc(), data);
  if (BIT(data,6))
    m_watchdog->watchdog_reset();
}

uint8_t manohman_state::duart_in_r()
{
  //logerror("%06x: DUART read\n", m_maincpu->pc()); // service?
  return 0x00;
}

void manohman_state::duart_out_w(uint8_t data)
{
  //logerror("%06x: DUART write %02x\n", m_maincpu->pc(), data); // service?
}

uint32_t manohman_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0, cliprect);
	const int display_width = 120;
	const int display_height = 7;
	const offs_t base = 0x111a;

	for (int x = 0; x < display_width; x++)
	{
		offs_t addr = base + x;
		offs_t index = addr >> 1;
		uint16_t word = m_workram[index];
		uint8_t column = (addr & 1) ? (word & 0xff) : (word >> 8);
		for (int y = 0; y < display_height; y++)
		{
			if (column & (1 << y))
				bitmap.pix(y, x) = 1;
		}
	}

	return 0;
}

/*

  RW

  100000 ; R      \
  100000 ; W 0000  | Constant after RAM test... Seems for the MAX696's watchdog.
  100000 ; W 00FF /

  500000-503FF9 ; R
  500000-503FF9 ; W FFFF \
  500000-503FF9 ; W AAAA  | Seems bit patterns for testing RAM...
  500000-503FF9 ; W 5555  |
  500000-503FF9 ; W 0000 /

  503FFA - 503FFF RW

  500300 ; R
  500302 ; R

  600006 ; R
  600006 ; W FFFF \
  600006 ; W AAAA  | These bit patterns are for 500000-503ff8 comparison.
  600006 ; W 5555  |
  600006 ; W 0000 /


  BP at 0x1880 to point to the end of RAM test.

*/

/*********************************************
*          Input Ports Definitions           *
*********************************************/

static INPUT_PORTS_START( backgamn )
	PORT_START("SW1")
	PORT_DIPNAME(0x01, 0x00, "Game Variant")       // Spielvariante
	PORT_DIPSETTING(   0x00, "Per move")           // pro Zug
	PORT_DIPSETTING(   0x01, "Per game")           // pro Spiel
	PORT_DIPNAME(0x02, 0x00, "Game Price")         // Spielpreis
	PORT_DIPSETTING(   0x02, "Half price")         // halber Preis
	PORT_DIPSETTING(   0x00, "Full price")         // voller Preis
	PORT_DIPNAME(0x04, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x04, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x00, "Clear Highscores")   // Highscore löschen
	PORT_DIPSETTING(   0x08, DEF_STR(Off))
	PORT_DIPSETTING(   0x00, "After 30 days")      // nach 30 Tg.
	PORT_DIPNAME(0x10, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x10, DEF_STR(On))
	PORT_DIPNAME(0x20, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x20, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x40, DEF_STR(On))
	PORT_DIPNAME(0x80, 0x00, DEF_STR(Language))    // Sprache
	PORT_DIPSETTING(   0x00, DEF_STR(German))      // deutsch
	PORT_DIPSETTING(   0x80, DEF_STR(English))     // englisch

	PORT_START("SW2")
	PORT_DIPNAME(0x01, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x01, DEF_STR(On))
	PORT_DIPNAME(0x02, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x02, DEF_STR(On))
	PORT_DIPNAME(0x04, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x04, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x08, DEF_STR(Off))
	PORT_DIPNAME(0x10, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x10, DEF_STR(On))
	PORT_DIPNAME(0x20, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x20, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x40, DEF_STR(On))
	PORT_DIPNAME(0x80, 0x00, DEF_STR(Unused))
	PORT_DIPSETTING(   0x00, DEF_STR(Off))
	PORT_DIPSETTING(   0x80, DEF_STR(On))
INPUT_PORTS_END

static INPUT_PORTS_START( manohman )
  PORT_INCLUDE(backgamn)
INPUT_PORTS_END



/*********************************************
*               Machine Config               *
*********************************************/

void manohman_state::manohman(machine_config &config)
{
	M68000(config, m_maincpu, XTAL(8'000'000)); // MC68000P8
	m_maincpu->set_addrmap(AS_PROGRAM, &manohman_state::mem_map);
	m_maincpu->set_addrmap(m68000_base_device::AS_CPU_SPACE, &manohman_state::cpu_space_map);

	PIT68230(config, m_pit, XTAL(8'000'000)); // MC68230P8
	m_pit->timer_irq_callback().set_inputline("maincpu", M68K_IRQ_2);

  m_pit->pa_in_callback().set(FUNC(manohman_state::pit_pa_r)); // buttons
  m_pit->pa_out_callback().set(FUNC(manohman_state::pit_pa_w)); // lamps
  
  m_pit->pb_in_callback().set(FUNC(manohman_state::pit_pb_r)); // reserved
  m_pit->pb_out_callback().set(FUNC(manohman_state::pit_pb_w)); // reserved

  m_pit->pc_in_callback().set(FUNC(manohman_state::pit_pc_r)); // coins
  m_pit->pc_out_callback().set(FUNC(manohman_state::pit_pc_w)); // coins

	MC68681(config, m_duart, XTAL(3'686'400));
	m_duart->irq_cb().set_inputline(m_maincpu, M68K_IRQ_4);
  m_duart->inport_cb().set(FUNC(manohman_state::duart_in_r)); // coins
  m_duart->outport_cb().set(FUNC(manohman_state::duart_out_w)); // coins
  m_duart->a_tx_cb().set(m_rs232, FUNC(rs232_port_device::write_txd));

  RS232_PORT(config, m_rs232, default_rs232_devices, nullptr);
  m_rs232->rxd_handler().set(m_duart, FUNC(mc68681_device::rx_a_w));

	MSM6242(config, "rtc", XTAL(32'768)); // M62X42B

  WATCHDOG_TIMER(config, m_watchdog).set_time(attotime::from_msec(160000));   // MAX696

	NVRAM(config, "nvram", nvram_device::DEFAULT_NONE); // KM6264BL-10 x2 + MAX696CFL + battery
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_refresh_hz(50);
	screen.set_size(120, 7);
	screen.set_visarea_full();
	screen.set_color(rgb_t::green());
	screen.set_palette("palette");
	screen.set_screen_update(FUNC(manohman_state::screen_update));

	PALETTE(config, "palette", palette_device::MONOCHROME_INVERTED);
	SPEAKER(config, "mono").front_center();
	SAA1099(config, "saa", XTAL(8'000'000) / 2).add_route(ALL_OUTPUTS, "mono", 0.10); // clock not verified
}


/*********************************************
*                  Rom Load                  *
*********************************************/

ROM_START( manohman )
	ROM_REGION( 0x20000, "maincpu", 0 )
  ROM_LOAD16_BYTE( "mom_austria_vorserie_i.bin",  0x00001, 0x10000, CRC(3c9507f9) SHA1(489a6aadfb7d61be0873bf48d428e9d915268f95) )
	ROM_LOAD16_BYTE( "mom_austria_vorserie_ii.bin", 0x00000, 0x10000, CRC(4b57409c) SHA1(0438f5d52f4de2ece8fb684cf2d82bdea0eacf0b) )	
ROM_END

ROM_START( backgamn )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "b_f2_i.bin",  0x00000, 0x10000, CRC(9e42937c) SHA1(85d462a560b85b03ee9d341e18815b7c396118ac) )
	ROM_LOAD16_BYTE( "b_f2_ii.bin", 0x00001, 0x10000, CRC(8e0ee50c) SHA1(2a05c337db1131b873646aa4109593636ebaa356) )
ROM_END

ROM_START( backgamnw )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "backgammon_wand_f2_i.u19",  0x00001, 0x10000, CRC(45e280d2) SHA1(65dff9d4884af2d37138f79d71bddd223433b13c) )
	ROM_LOAD16_BYTE( "backgammon_wand_f2_ii.u20", 0x00000, 0x10000, CRC(ccc536c3) SHA1(82d9243d6f6e05d4ce83d1080c6c038852d2398c) )
ROM_END

} // anonymous namespace


/*********************************************
*                Game Drivers                *
*********************************************/

//    YEAR  NAME       PARENT    MACHINE   INPUT     STATE           INIT        ROT   COMPANY   FULLNAME                FLAGS
GAME( 199?, manohman,  0,        manohman, manohman, manohman_state, empty_init, ROT0, "Merkur", "Mann, oh-Mann",        MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME( 1990, backgamn,  0,        manohman, backgamn, manohman_state, empty_init, ROT0, "Merkur", "Backgammon",           MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
GAME( 1990, backgamnw, backgamn, manohman, backgamn, manohman_state, empty_init, ROT0, "Merkur", "Backgammon (Wand)",    MACHINE_NOT_WORKING | MACHINE_NO_SOUND | MACHINE_REQUIRES_ARTWORK )
