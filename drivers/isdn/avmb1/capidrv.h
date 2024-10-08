/*
 * $Id: capidrv.h,v 1.1.1.1 1999/02/08 06:20:44 linas Exp $
 *
 * ISDN4Linux Driver, using capi20 interface (kernelcapi)
 *
 * Copyright 1997 by Carsten Paeth (calle@calle.in-berlin.de)
 *
 * $Log: capidrv.h,v $
 * Revision 1.1.1.1  1999/02/08 06:20:44  linas
 * stock linux 2.2.1 kernel
 *
 * Revision 1.1  1997/03/04 21:50:33  calle
 * Frirst version in isdn4linux
 *
 * Revision 2.2  1997/02/12 09:31:39  calle
 * new version
 *
 * Revision 1.1  1997/01/31 10:32:20  calle
 * Initial revision
 *
 */
#ifndef __CAPIDRV_H__
#define __CAPIDRV_H__

/*
 * LISTEN state machine
 */
#define ST_LISTEN_NONE			0	/* L-0 */
#define ST_LISTEN_WAIT_CONF		1	/* L-0.1 */
#define ST_LISTEN_ACTIVE		2	/* L-1 */
#define ST_LISTEN_ACTIVE_WAIT_CONF	3	/* L-1.1 */


#define EV_LISTEN_REQ			1	/* L-0 -> L-0.1
						   L-1 -> L-1.1 */
#define EV_LISTEN_CONF_ERROR		2	/* L-0.1 -> L-0
						   L-1.1 -> L-1 */
#define EV_LISTEN_CONF_EMPTY		3	/* L-0.1 -> L-0
						   L-1.1 -> L-0 */
#define EV_LISTEN_CONF_OK		4	/* L-0.1 -> L-1
						   L-1.1 -> L.1 */

/*
 * per plci state machine
 */
#define ST_PLCI_NONE			0	/* P-0 */
#define ST_PLCI_OUTGOING 		1	/* P-0.1 */
#define ST_PLCI_ALLOCATED		2	/* P-1 */
#define ST_PLCI_ACTIVE			3	/* P-ACT */
#define ST_PLCI_INCOMING		4	/* P-2 */
#define ST_PLCI_FACILITY_IND		5	/* P-3 */
#define ST_PLCI_ACCEPTING		6	/* P-4 */
#define ST_PLCI_DISCONNECTING		7	/* P-5 */
#define ST_PLCI_DISCONNECTED		8	/* P-6 */

#define EV_PLCI_CONNECT_REQ		1	/* P-0 -> P-0.1 */
#define EV_PLCI_CONNECT_CONF_ERROR	2	/* P-0.1 -> P-0 */
#define EV_PLCI_CONNECT_CONF_OK		3	/* P-0.1 -> P-1 */
#define EV_PLCI_FACILITY_IND_UP		4	/* P-0 -> P-1 */
#define EV_PLCI_CONNECT_IND		5	/* P-0 -> P-2 */
#define EV_PLCI_CONNECT_ACTIVE_IND	6	/* P-1 -> P-ACT */
#define EV_PLCI_CONNECT_REJECT		7	/* P-2 -> P-5
						   P-3 -> P-5 */
#define EV_PLCI_DISCONNECT_REQ		8	/* P-1 -> P-5
						   P-2 -> P-5
						   P-3 -> P-5
						   P-4 -> P-5
						   P-ACT -> P-5 */
#define EV_PLCI_DISCONNECT_IND		9	/* P-1 -> P-6
						   P-2 -> P-6
						   P-3 -> P-6
						   P-4 -> P-6
						   P-5 -> P-6
						   P-ACT -> P-6 */
#define EV_PLCI_FACILITY_IND_DOWN	10	/* P-0.1 -> P-5
						   P-1 -> P-5
						   P-ACT -> P-5
						   P-2 -> P-5
						   P-3 -> P-5
						   P-4 -> P-5 */
#define EV_PLCI_DISCONNECT_RESP		11	/* P-6 -> P-0 */
#define EV_PLCI_CONNECT_RESP		12	/* P-6 -> P-0 */

/*
 * per ncci state machine
 */
#define ST_NCCI_PREVIOUS			-1
#define ST_NCCI_NONE				0	/* N-0 */
#define ST_NCCI_OUTGOING			1	/* N-0.1 */
#define ST_NCCI_INCOMING			2	/* N-1 */
#define ST_NCCI_ALLOCATED			3	/* N-2 */
#define ST_NCCI_ACTIVE				4	/* N-ACT */
#define ST_NCCI_RESETING			5	/* N-3 */
#define ST_NCCI_DISCONNECTING			6	/* N-4 */
#define ST_NCCI_DISCONNECTED			7	/* N-5 */

#define EV_NCCI_CONNECT_B3_REQ			1	/* N-0 -> N-0.1 */
#define EV_NCCI_CONNECT_B3_IND			2	/* N-0 -> N.1 */
#define EV_NCCI_CONNECT_B3_CONF_OK		3	/* N-0.1 -> N.2 */
#define EV_NCCI_CONNECT_B3_CONF_ERROR		4	/* N-0.1 -> N.0 */
#define EV_NCCI_CONNECT_B3_REJECT		5	/* N-1 -> N-4 */
#define EV_NCCI_CONNECT_B3_RESP			6	/* N-1 -> N-2 */
#define EV_NCCI_CONNECT_B3_ACTIVE_IND		7	/* N-2 -> N-ACT */
#define EV_NCCI_RESET_B3_REQ			8	/* N-ACT -> N-3 */
#define EV_NCCI_RESET_B3_IND			9	/* N-3 -> N-ACT */
#define EV_NCCI_DISCONNECT_B3_IND		10	/* N-4 -> N.5 */
#define EV_NCCI_DISCONNECT_B3_CONF_ERROR	11	/* N-4 -> previous */
#define EV_NCCI_DISCONNECT_B3_REQ		12	/* N-1 -> N-4
							   N-2 -> N-4
							   N-3 -> N-4
							   N-ACT -> N-4 */
#define EV_NCCI_DISCONNECT_B3_RESP		13	/* N-5 -> N-0 */

#endif				/* __CAPIDRV_H__ */
