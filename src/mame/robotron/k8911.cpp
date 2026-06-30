// license:BSD-3-Clause
// copyright-holders:stonedDiscord
/***************************************************************************

    Bildschirmterminal Robotron K8911

    The K8911 is a K1520-based screen terminal.  Two board sets are known.

    Old variant:
    K3820  012-7040  PFS  Programmierbarer Festwertspeicher
                         Max. 16 KiB ROM
    K7024  012-6820  ABS  Adapter fuer Bildschirm
                         Bildschirmkarte
    K2521  012-7100  ZRE  Zentrale Recheneinheit
                         CPU + 3 KiB ROM + 1 KiB RAM
    K7028  012-6710  ATS  Adapter fuer Tastatur und serielle Geraete
                         Tastatur, Drucker + Fernleitung

    New variant:
    K7024  012-6820  ABS  Adapter fuer Bildschirm
                         Bildschirmkarte
    ?      045-8786  ZRE  Zentrale Recheneinheit
                         CPU + 20 KiB ROM + 1 KiB RAM
    K7028  012-6710  ATS  Adapter fuer Tastatur und serielle Geraete
                         Tastatur, Drucker + Fernleitung

    TODO:
    - ABS/ATS board details
    - Confirm K1520 slot order and memory maps
    - Hook terminal screen and serial/keyboard devices

***************************************************************************/

#include "emu.h"

#include "bus/ddr/k1520.h"


namespace {

class k8911_pfs_device : public device_t, public device_k1520_card_interface
{
public:
	k8911_pfs_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	void set_slot(k1520_bus_device &bus, unsigned slot);

protected:
	virtual void device_start() override ATTR_COLD;
	virtual bool memory_r(offs_t offset, u8 &data) override;

private:
	required_region_ptr<u8> m_rom;
};

class k8911_state : public driver_device
{
public:
	k8911_state(machine_config const &mconfig, device_type type, char const *tag) :
		driver_device(mconfig, type, tag),
		m_k1520(*this, "k1520"),
		m_zre(*this, "zre"),
		m_pfs(*this, "pfs"),
		m_abs(*this, "abs"),
		m_ats(*this, "ats")
	{
	}

	void k8911(machine_config &config) ATTR_COLD;
	void k8911_8786(machine_config &config) ATTR_COLD;

private:
	required_device<k1520_bus_device> m_k1520;
	required_device<k1520_zre_device> m_zre;
	optional_device<k8911_pfs_device> m_pfs;
	required_device<k1520_placeholder_card_device> m_abs;
	required_device<k1520_placeholder_card_device> m_ats;
};


DEFINE_DEVICE_TYPE_PRIVATE(K8911_PFS, k8911_pfs_device, k8911_pfs_device, "k8911_pfs", "K8911 K3820 PFS Board")


k8911_pfs_device::k8911_pfs_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, K8911_PFS, tag, owner, clock),
	device_k1520_card_interface(mconfig, *this),
	m_rom(*this, "rom")
{
}

void k8911_pfs_device::set_slot(k1520_bus_device &bus, unsigned slot)
{
	set_bus(bus, slot);
}

void k8911_pfs_device::device_start()
{
}

bool k8911_pfs_device::memory_r(offs_t offset, u8 &data)
{
	if (offset < 0x1000 || offset > 0x4fff)
		return false;

	data = m_rom[offset - 0x1000];
	return true;
}


static INPUT_PORTS_START( k8911 )
INPUT_PORTS_END


void k8911_state::k8911(machine_config &config)
{
	K1520_BUS(config, m_k1520, 0);

	K8911_PFS(config, m_pfs, 0);          // K3820 / 012-7040 PFS
	m_pfs->set_slot(*m_k1520, 1);

	K1520_PLACEHOLDER_CARD(config, m_abs, 0); // K7024 / 012-6820 ABS
	m_abs->set_slot(*m_k1520, 2);

	K1520_ZRE(config, m_zre, 0);          // K2521 / 012-7100 ZRE
	m_zre->set_slot(*m_k1520, 3);

	K1520_PLACEHOLDER_CARD(config, m_ats, 0); // K7028 / 012-6710 ATS
	m_ats->set_slot(*m_k1520, 4);
}

void k8911_state::k8911_8786(machine_config &config)
{
	K1520_BUS(config, m_k1520, 0);

	K1520_PLACEHOLDER_CARD(config, m_abs, 0); // K7024 / 012-6820 ABS
	m_abs->set_slot(*m_k1520, 1);

	K1520_ZRE_8786(config, m_zre, 0);     // 045-8786 ZRE
	m_zre->set_slot(*m_k1520, 2);

	K1520_PLACEHOLDER_CARD(config, m_ats, 0); // K7028 / 012-6710 ATS
	m_ats->set_slot(*m_k1520, 3);
}


ROM_START( k8911 )
	ROM_REGION( 0x0c00, "zre:rom", ROMREGION_ERASEFF )
	ROM_LOAD( "y221",  0x0000, 0x0400, CRC(20664820) SHA1(04126771312c86d4cce2b5529364893ebd798342) )
	ROM_LOAD( "y2221", 0x0400, 0x0400, CRC(84cb0ac7) SHA1(38a457734606ae58a655163554a1d7e7d4ac9628) )
	ROM_LOAD( "y223",  0x0800, 0x0400, CRC(a509f2dc) SHA1(55f55ff88c71a6aaf6ddda5dea5b0aa30385d080) )

	ROM_REGION( 0x4000, "pfs:rom", ROMREGION_ERASEFF )
	ROM_LOAD( "y224", 0x0000, 0x0400, CRC(e894eba0) SHA1(7cfe84d12136ac42f76d44102f669e01231e9d18) )
	ROM_LOAD( "y225", 0x0400, 0x0400, CRC(610a85e3) SHA1(acb58d1796ed3b03354483b9efb37e988fb63f83) )
	ROM_LOAD( "y226", 0x0800, 0x0400, CRC(4b90a984) SHA1(499f5d2a322843330b8264de2b7931bef28d4a1a) )
	ROM_LOAD( "y227", 0x0c00, 0x0400, CRC(84ffecce) SHA1(977fa7d4269a5e73cdb069595f27becfa4c9979f) )
	ROM_LOAD( "y228", 0x1000, 0x0400, CRC(73034d6b) SHA1(cb7eb81ca37628e25094b219bc40bfe1bdad5340) )
	ROM_LOAD( "y229", 0x1400, 0x0400, CRC(a219d2ce) SHA1(777c38889f54742b49fabbfb4ae8f716f30d5f64) )
	ROM_LOAD( "y230", 0x1800, 0x0400, CRC(9e3dc3d4) SHA1(47d3d3b526f2b41ee55bc0e8a4e9cffa4ac47720) )
	ROM_LOAD( "y231", 0x1c00, 0x0400, CRC(8a0afac2) SHA1(60263508a4eff4b623c6de7e5a210b717aa39328) )
	ROM_LOAD( "y232", 0x2000, 0x0400, CRC(c229091c) SHA1(c40c2bed388997fe50d3f98cb20de9f4a3a33813) )
	ROM_LOAD( "y233", 0x2400, 0x0400, CRC(baf3189d) SHA1(4cbbf9a06800f6ddc91a0e3dd9306aa3deba4ea3) )
ROM_END

ROM_START( k8911_8786 )
	ROM_REGION( 0x5000, "zre:rom", ROMREGION_ERASEFF )
	ROM_LOAD( "k8911_zre_045-8786.bin", 0x0000, 0x5000, NO_DUMP )
ROM_END

} // anonymous namespace


//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT  CLASS        INIT        COMPANY         FULLNAME                                FLAGS
COMP( 198?, k8911,      0,      0,      k8911,      k8911, k8911_state, empty_init, "VEB Robotron", "K8911 Bildschirmterminal",            MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
COMP( 198?, k8911_8786, k8911,  0,      k8911_8786, k8911, k8911_state, empty_init, "VEB Robotron", "K8911 Bildschirmterminal (045-8786)", MACHINE_NO_SOUND | MACHINE_NOT_WORKING )
