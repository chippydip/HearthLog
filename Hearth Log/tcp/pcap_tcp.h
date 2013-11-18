#pragma once
/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <pcap.h>


#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#include <arpa/inet.h>
#endif

/*******************************************************************************************/
/* @(#) $Header: /tcpdump/master/tcpdump/ether.h,v 1.8 2002/12/11 07:13:51 guy Exp $ (LBL) */
/*******************************************************************************************/

#define	ETHERMTU	1500

/*
 * The number of bytes in an ethernet (MAC) address.
 */
#define	ETHER_ADDR_LEN		6

/*
 * Structure of a DEC/Intel/Xerox or 802.3 Ethernet header.
 */
struct	ether_header {
	u_int8_t	ether_dhost[ETHER_ADDR_LEN];
	u_int8_t	ether_shost[ETHER_ADDR_LEN];
	u_int16_t	ether_type;
};

/*
 * Length of a DEC/Intel/Xerox or 802.3 Ethernet header; note that some
 * compilers may pad "struct ether_header" to a multiple of 4 bytes,
 * for example, so "sizeof (struct ether_header)" may not give the right
 * answer.
 */
#define ETHER_HDRLEN		14

/********************************************************************************************/



#define ETHERTYPE_IP            0x0800  /* IP protocol */
#define ETHERTYPE_IPV6          0x86DD  /* IP protocol version 6 */


/********************************************************************************************/
/* @(#) $Header: /tcpdump/master/tcpdump/ip.h,v 1.11 2004/09/27 21:13:10 hannes Exp $ (LBL) */
/********************************************************************************************/

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */
#define	IPVERSION	4

/*
 * Structure of an internet header, naked of options.
 *
 * We declare ip_len and ip_off to be short, rather than u_short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */
struct ip {
	u_int8_t	ip_vhl;		/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
	u_int8_t	ip_tos;		/* type of service */
	u_int16_t	ip_len;		/* total length */
	u_int16_t	ip_id;		/* identification */
	u_int16_t	ip_off;		/* fragment offset field */
#define	IP_DF 0x4000		/* dont fragment flag */
#define	IP_MF 0x2000		/* more fragments flag */
#define	IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_int8_t	ip_ttl;		/* time to live */
	u_int8_t	ip_p;		/* protocol */
	u_int16_t	ip_sum;		/* checksum */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
};

#define	IP_MAXPACKET	65535	/* maximum packet size */

/*
 * Definitions for IP type of service (ip_tos)
 */
#define	IPTOS_LOWDELAY		0x10
#define	IPTOS_THROUGHPUT	0x08
#define	IPTOS_RELIABILITY	0x04

/*
 * Definitions for IP precedence (also in ip_tos) (hopefully unused)
 */
#define	IPTOS_PREC_NETCONTROL		0xe0
#define	IPTOS_PREC_INTERNETCONTROL	0xc0
#define	IPTOS_PREC_CRITIC_ECP		0xa0
#define	IPTOS_PREC_FLASHOVERRIDE	0x80
#define	IPTOS_PREC_FLASH			0x60
#define	IPTOS_PREC_IMMEDIATE		0x40
#define	IPTOS_PREC_PRIORITY			0x20
#define	IPTOS_PREC_ROUTINE			0x00

/*
 * Definitions for options.
 */
#define	IPOPT_COPIED(o)		((o)&0x80)
#define	IPOPT_CLASS(o)		((o)&0x60)
#define	IPOPT_NUMBER(o)		((o)&0x1f)

#define	IPOPT_CONTROL		0x00
#define	IPOPT_RESERVED1		0x20
#define	IPOPT_DEBMEAS		0x40
#define	IPOPT_RESERVED2		0x60

#define	IPOPT_EOL		  0		/* end of option list */
#define	IPOPT_NOP		  1		/* no operation */

#define	IPOPT_RR		  7		/* record packet route */
#define	IPOPT_TS		 68		/* timestamp */
#define	IPOPT_SECURITY	130		/* provide s,c,h,tcc */
#define	IPOPT_LSRR		131		/* loose source route */
#define	IPOPT_SATID		136		/* satnet id */
#define	IPOPT_SSRR		137		/* strict source route */
#define IPOPT_RA        148     /* router-alert, rfc2113 */

/*
 * Offsets to fields in options other than EOL and NOP.
 */
#define	IPOPT_OPTVAL	0		/* option ID */
#define	IPOPT_OLEN		1		/* option length */
#define IPOPT_OFFSET	2		/* offset within option */
#define	IPOPT_MINOFF	4		/* min value of above */

/*
 * Time stamp option structure.
 */
struct	ip_timestamp {
	u_int8_t	ipt_code;	/* IPOPT_TS */
	u_int8_t	ipt_len;	/* size of structure (variable) */
	u_int8_t	ipt_ptr;	/* index of current entry */
	u_int8_t	ipt_oflwflg;	/* flags, overflow counter */
#define IPTS_OFLW(ip)	(((ipt)->ipt_oflwflg & 0xf0) >> 4)
#define IPTS_FLG(ip)	((ipt)->ipt_oflwflg & 0x0f)
	union ipt_timestamp {
		u_int32_t ipt_time[1];
		struct	ipt_ta {
			struct in_addr ipt_addr;
			u_int32_t ipt_time;
		} ipt_ta[1];
	} ipt_timestamp;
};

/* flag bits for ipt_flg */
#define	IPOPT_TS_TSONLY		0		/* timestamps only */
#define	IPOPT_TS_TSANDADDR	1		/* timestamps and addresses */
#define	IPOPT_TS_PRESPEC	3		/* specified modules only */

/* bits for security (not byte swapped) */
#define	IPOPT_SECUR_UNCLASS	0x0000
#define	IPOPT_SECUR_CONFID	0xf135
#define	IPOPT_SECUR_EFTO	0x789a
#define	IPOPT_SECUR_MMMM	0xbc4d
#define	IPOPT_SECUR_RESTR	0xaf13
#define	IPOPT_SECUR_SECRET	0xd788
#define	IPOPT_SECUR_TOPSECRET	0x6bc5

/*
 * Internet implementation parameters.
 */
#define	MAXTTL		255		/* maximum time to live (seconds) */
#define	IPDEFTTL	64		/* default ttl, from RFC 1340 */
#define	IPFRAGTTL	60		/* time to live for frags, slowhz */
#define	IPTTLDEC	1		/* subtracted when forwarding */

#define	IP_MSS		576		/* default maximum segment size */

/* in print-ip.c */
extern u_int32_t ip_finddst(const struct ip *);

/*************************************************************************************************/



#define IPPROTO_TCP             6               /* tcp */



/*************************************************************************************************/
/* @(#) $Header: /tcpdump/master/tcpdump/tcp.h,v 1.11.2.1 2005/11/29 09:09:26 hannes Exp $ (LBL) */
/*************************************************************************************************/

typedef	u_int32_t	tcp_seq;
/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct tcphdr {
	u_int16_t	th_sport;		/* source port */
	u_int16_t	th_dport;		/* destination port */
	tcp_seq		th_seq;			/* sequence number */
	tcp_seq		th_ack;			/* acknowledgement number */
	u_int8_t	th_offx2;		/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
	u_int8_t	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
#define TH_ECNECHO	0x40	/* ECN Echo */
#define TH_CWR		0x80	/* ECN Cwnd Reduced */
	u_int16_t	th_win;			/* window */
	u_int16_t	th_sum;			/* checksum */
	u_int16_t	th_urp;			/* urgent pointer */
};

#define	TCPOPT_EOL			0
#define	TCPOPT_NOP			1
#define	TCPOPT_MAXSEG		2
#define		TCPOLEN_MAXSEG		4
#define	TCPOPT_WSCALE		3	/* window scale factor (rfc1323) */
#define	TCPOPT_SACKOK		4	/* selective ack ok (rfc2018) */
#define	TCPOPT_SACK			5	/* selective ack (rfc2018) */
#define	TCPOPT_ECHO			6	/* echo (rfc1072) */
#define	TCPOPT_ECHOREPLY	7	/* echo (rfc1072) */
#define TCPOPT_TIMESTAMP	8	/* timestamp (rfc1323) */
#define		TCPOLEN_TIMESTAMP	10
#define		TCPOLEN_TSTAMP_APPA	(TCPOLEN_TIMESTAMP+2) /* appendix A */
#define TCPOPT_CC			11	/* T/TCP CC options (rfc1644) */
#define TCPOPT_CCNEW		12	/* T/TCP CC options (rfc1644) */
#define TCPOPT_CCECHO		13	/* T/TCP CC options (rfc1644) */
#define TCPOPT_SIGNATURE	19	/* Keyed MD5 (rfc2385) */
#define		TCPOLEN_SIGNATURE	18
#define TCP_SIGLEN			16	/* length of an option 19 digest */
#define TCPOPT_AUTH         20  /* Enhanced AUTH option */

#define TCPOPT_TSTAMP_HDR	\
	(TCPOPT_NOP<<24|TCPOPT_NOP<<16|TCPOPT_TIMESTAMP<<8|TCPOLEN_TIMESTAMP)
