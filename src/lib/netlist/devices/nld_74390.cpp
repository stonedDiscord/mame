// license:BSD-3-Clause

/*
 * nld_74390.cpp
 *
 *  DM74390: Dual 4-Stage Binary Counter
 *
 *          +--------------+
 *       /A |1     ++    14| VCC
 *       MR |2           13| /A
 *       Q0 |3           12| MR
 *       Q1 |4    74390  11| Q0
 *       Q2 |5           10| Q1
 *       Q3 |6            9| Q2
 *      GND |7            8| Q3
 *          +--------------+
 *
 *  Naming conventions follow Motorola datasheet
 *
 */

#include "nl_base.h"

namespace netlist::devices {

	static constexpr const unsigned MAXCNT = 9;

	NETLIB_OBJECT(74390)
	{
		NETLIB_CONSTRUCTOR(74390)
		, m_A(*this, "A", NETLIB_DELEGATE(a))
        , m_B(*this, "B", NETLIB_DELEGATE(b))
		, m_MR(*this, "MR", NETLIB_DELEGATE(mr))
		, m_Q(*this, {"Q0", "Q1", "Q2", "Q3"})
		, m_cnt(*this, "m_cnt", 0)
		, m_a(*this, "m_a", 0)
		, m_b(*this, "m_b", 0)
		, m_mr(*this, "m_mr", 0)
		, m_power_pins(*this)
		{
		}

	private:
		NETLIB_RESETI()
		{
			m_A.set_state(logic_t::STATE_INP_HL);
            m_B.set_state(logic_t::STATE_INP_HL);
			m_cnt = 0;
		}

		NETLIB_HANDLERI(mr)
		{
			if (!m_MR())
			{
				m_A.activate_hl();
                m_B.activate_hl();
			}
			else
			{
				m_A.inactivate();
                m_B.inactivate();
				if (m_cnt != 0)
				{
					m_cnt = 0;
					m_Q.push(0, NLTIME_FROM_NS(24));
				}
			}
		}

		NETLIB_HANDLERI(a)
		{
			auto cnt = (m_cnt + 1) & MAXCNT;
			m_cnt = cnt;
			m_Q[0].push((cnt >> 0) & 1, NLTIME_FROM_NS(13));
			m_Q[1].push((cnt >> 1) & 1, NLTIME_FROM_NS(22));
			m_Q[2].push((cnt >> 2) & 1, NLTIME_FROM_NS(31));
			m_Q[3].push((cnt >> 3) & 1, NLTIME_FROM_NS(40));
		}

        NETLIB_HANDLERI(b)
		{
			auto cnt = (m_cnt + 1) & MAXCNT;
			m_cnt = cnt;
			m_Q[0].push((cnt >> 0) & 1, NLTIME_FROM_NS(13));
			m_Q[1].push((cnt >> 1) & 1, NLTIME_FROM_NS(22));
			m_Q[2].push((cnt >> 2) & 1, NLTIME_FROM_NS(31));
			m_Q[3].push((cnt >> 3) & 1, NLTIME_FROM_NS(40));
		}

		logic_input_t m_A;
        logic_input_t m_B;
		logic_input_t m_MR;
		object_array_t<logic_output_t, 4> m_Q;

		state_var<unsigned> m_cnt;
		state_var_sig m_a;
        state_var_sig m_b;
		state_var_sig m_mr;

		nld_power_pins m_power_pins;
	};

	NETLIB_DEVICE_IMPL(74390,     "TTL_74390", "+A,+B,+MR,@VCC,@GND")

} // namespace netlist::devices
