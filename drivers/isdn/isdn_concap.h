/* $Id: isdn_concap.h,v 1.1 1999/02/08 06:20:41 linas Exp $
 */
extern struct concap_device_ops isdn_concap_reliable_dl_dops;
extern struct concap_device_ops isdn_concap_demand_dial_dops;
extern struct concap_proto * isdn_concap_new( int );


