
// NES MMC5 mapper, currently only tailored for Castlevania 3 (U)

// Nes_Emu 0.5.4. http://www.slack.net/~ant/

#include "Nes_Mapper.h"

#include "Nes_Emu.h"
#include <string.h>

/* Copyright (C) 2004-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include BLARGG_SOURCE_BEGIN

struct mmc5_state_t
{
	enum { reg_count = 0x30 };
	byte regs [0x30];
	byte irq_enabled;
};
// to do: finalize state format
BOOST_STATIC_ASSERT( sizeof (mmc5_state_t) == 0x31 );

class Mapper_Mmc5 : public Nes_Mapper, mmc5_state_t {
	nes_time_t irq_time;
public:
	Mapper_Mmc5()
	{
		mmc5_state_t* state = this;
		register_state( state, sizeof *state );
	}
	
	void reset()
	{
		Nes_Mapper::reset();
		
		intercept_writes( 0x5100, 0x200 );
		memset( regs, 0, sizeof regs );
		
		irq_time = no_irq;
		irq_enabled = false;
		regs [0] = 2;
		regs [0x14] = 0x7f;
		regs [0x15] = 0x7f;
		regs [0x16] = 0x7f;
		regs [0x17] = 0x7f;
		apply_mapping();
	}
	
	void apply_mapping();
	
	nes_time_t next_irq( nes_time_t present )
	{
		if ( !(irq_enabled & 0x80) )
			return no_irq;
		
		return irq_time;
	}
	
	void write( nes_time_t time, nes_addr_t addr, int data )
	{
		int reg = addr - 0x5100;
		if ( (unsigned) reg < reg_count )
		{
			regs [reg] = data;
			apply_mapping();
			// to do: handle writes in a switch statement
		}
		else if ( addr == 0x5203 )
		{
			irq_time = no_irq;
			if ( data && data < 240 )
			{
				irq_time = (341 * 21 + 128 + (data * 341)) / 3;
				if ( irq_time < time )
					irq_time = no_irq;
			}
			irq_changed();
		}
		else if ( addr == 0x5204 )
		{
			irq_enabled = data;
			irq_changed();
		}
		else
		{
			Nes_Mapper::write( time, addr, data );
		}
	}
};

void Mapper_Mmc5::apply_mapping()
{
	mirror_manual( regs [0x05] & 3, (regs [0x05] >> 2) & 3,
			(regs [0x05] >> 4) & 3, (regs [0x05] >> 6) & 3 );
	
	check( (regs [0x00] & 3) == 2 );
	set_prg_bank( 0x8000, bank_16k, (regs [0x15] & 0x7f) >> 1 );
	set_prg_bank( 0xC000, bank_8k, regs [0x16] & 0x7f );
	set_prg_bank( 0xE000, bank_8k, regs [0x17] & 0x7f );
	
	check( (regs [0x01] & 3) == 3 );
	
	for ( int i = 0; i < 8; i++ )
		set_chr_bank( i * 0x400, bank_1k, regs [0x20 + i + (i & 4)] );
}

void register_mmc5_mapper();
void register_mmc5_mapper()
{
	register_mapper<Mapper_Mmc5>( 5 );
}

