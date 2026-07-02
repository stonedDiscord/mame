DECLARE_DEVICE_TYPE(K1520_ZRE, k1520_zre_k2521_device)

class k1520_zre_k2521_device : public device_t, public device_k1520_card_interface
{
public:
	k1520_zre_k2521_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	void irq_line_w(int state);
	void nmi_line_w(int state);

protected:
	k1520_zre_k2521_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock);

	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;
	virtual void device_start() override ATTR_COLD;
	virtual bool memory_r(offs_t offset, u8 &data) override;
	virtual bool memory_w(offs_t offset, u8 data) override;

private:
	void mem_map(address_map &map) ATTR_COLD;
	void io_map(address_map &map) ATTR_COLD;
	u8 bus_memory_r(offs_t offset);
	void bus_memory_w(offs_t offset, u8 data);
	u8 bus_io_r(offs_t offset);
	void bus_io_w(offs_t offset, u8 data);

	required_device<z80_device> m_maincpu;
	required_device<z80ctc_device> m_ctc;
	required_device<z80pio_device> m_pio;
	required_region_ptr<u8> m_rom;
	std::array<u8, 0x400> m_ram;
};
