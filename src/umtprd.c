/*
 * uMTP Responder
 * Copyright (c) 2018 - 2019 Viveris Technologies
 *
 * uMTP Responder is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * uMTP Responder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMTP Responder; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   umtprd.c
 * @brief  Main MTP Daemon program.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "mtp.h"


#include "usb_gadget.h"
#include "usb_gadget_fct.h"

#include "logs_out.h"

mtp_ctx * mtp_context;

void* io_thread(void* arg)
{
	usb_gadget * ctx;
	int ret;

	ctx = (usb_gadget *)arg;

	while (is_usb_up(ctx))
	{
		ret = mtp_incoming_packet(mtp_context);
		if(ret < 0)
		{
			ctx->stop = 1;
		}
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	usb_gadget * usb_ctx;
	int retcode = 0;
	int loop_continue = 0;

	mtp_context = 0;

	PRINT_MSG("uMTP Responder");
	PRINT_MSG("Version: %s compiled the %s@%s", APP_VERSION,
		  __DATE__, __TIME__);

	PRINT_MSG("(c) 2018 - 2019 Viveris Technologies");

	mtp_context = mtp_init_responder();
	if(!mtp_context)
	{
		exit(-1);
	}

	mtp_load_config_file(mtp_context);

	loop_continue = mtp_context->usb_cfg.loop_on_disconnect;

	do
	{
		usb_ctx = init_usb_mtp_gadget(mtp_context);
		if(usb_ctx)
		{
			mtp_set_usb_handle(mtp_context, usb_ctx, mtp_context->usb_cfg.usb_max_packet_size);
			if( mtp_context->usb_cfg.usb_functionfs_mode )
			{
				PRINT_DEBUG("uMTP Responder : FunctionFS Mode - entering handle_ffs_ep0");
				handle_ffs_ep0(usb_ctx);
			}
			else
			{
				PRINT_DEBUG("uMTP Responder : GadgetFS Mode - entering handle_ep0");
				handle_ep0(usb_ctx);
			}
			deinit_usb_mtp_gadget(usb_ctx);
		}
		else
		{
			PRINT_ERROR("USB Init failed !");
			retcode = -2;
			loop_continue = 0;
		}

		PRINT_MSG("uMTP Responder : Disconnected");

		if(mtp_context->fs_db)
		{
			deinit_fs_db(mtp_context->fs_db);
			mtp_context->fs_db = 0;
		}
	}while(loop_continue);

	mtp_deinit_responder(mtp_context);

	exit(retcode);
}
