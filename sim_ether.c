/* sim_ether.c: OS-dependent network routines
  ------------------------------------------------------------------------------
   Copyright (c) 2002-2007, David T. Hittner

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

  ------------------------------------------------------------------------------

  This ethernet simulation is based on the PCAP and WinPcap packages.

  PCAP/WinPcap was chosen as the basis for network code since it is the most
  "universal" of the various network packages available. Using this style has
  allowed rapid network development for the major SIMH platforms. Developing
  a network package specifically for SIMH was rejected due to the time required;
  the advantage would be a more easily compiled and integrated code set.

  There are various problems associated with use of ethernet networking, which
  would be true regardless of the network package used, since there are no
  universally accepted networking methods. The most serious of these is getting
  the proper networking package loaded onto the system, since most environments
  do not come with the network interface packages loaded.

  The second most serious network issue relates to security. The network
  simulation needs to simulate operating system level functionality (packet
  driving). However, the host network programming interfaces tend to operate at
  the user level of functionality, so getting to the full functionality of
  the network interface usually requires that the person executing the
  network code be a privileged user of the host system. See the PCAP/WinPcap
  documentation for the appropriate host platform if unprivileged use of
  networking is needed - there may be known workarounds.

  Define one of the two macros below to enable networking:
    USE_NETWORK     - Create statically linked network code
    USE_SHARED      - Create dynamically linked network code (_WIN32 only)

  ------------------------------------------------------------------------------

  Modification history:

  17-May-07  DTH  Fixed non-ethernet device removal loop (from Naoki Hamada)
  15-May-07  DTH  Added dynamic loading of wpcap.dll;
                  Corrected exceed max index bug in ethX lookup
  04-May-07  DTH  Corrected failure to look up ethernet device names in
                  the registry on Windows XP x64
  10-Jul-06  RMS  Fixed linux conditionalization (from Chaskiel Grundman)
  02-Jun-06  JDB  Fixed compiler warning for incompatible sscanf parameter
  15-Dec-05  DTH  Patched eth_host_devices [remove non-ethernet devices]
                  (from Mark Pizzolato and Galen Tackett, 08-Jun-05)
                  Patched eth_open [tun fix](from Antal Ritter, 06-Oct-05)
  30-Nov-05  DTH  Added option to regenerate CRC on received packets; some
                  ethernet devices need to pass it on to the simulation, and by
                  the time libpcap/winpcap gets the packet, the host OS network
                  layer has already stripped CRC out of the packet
  01-Dec-04  DTH  Added Windows user-defined adapter names (from Timothe Litt)
  25-Mar-04  MP   Revised comments and minor #defines to deal with updated
                  libpcap which now provides pcap_sendpacket on all platforms.
  04-Feb-04  MP   Returned success/fail status from eth_write to support
                  determining if the current libpcap connection can successfully 
                  write packets.
                  Added threaded approach to reading packets since
                  this works better on some platforms (solaris intel) than the 
                  inconsistently implemented non-blocking read approach.
  04-Feb-04  DTH  Converted ETH_DEBUG to sim_debug
  13-Jan-04  MP   tested and fixed on OpenBSD, NetBS and FreeBSD.
  09-Jan-04  MP   removed the BIOCSHDRCMPLT ioctl() for OS/X
  05-Jan-04  DTH  Added eth_mac_scan
  30-Dec-03  DTH  Cleaned up queue routines, added no network support message
  26-Dec-03  DTH  Added ethernet show and queue functions from pdp11_xq
  15-Dec-03  MP   polished generic libpcap support.
  05-Dec-03  DTH  Genericized eth_devices() and #ifdefs
  03-Dec-03  MP   Added Solaris support
  02-Dec-03  DTH  Corrected decnet fix to use reflection counting
  01-Dec-03  DTH  Added BPF source filtering and reflection counting
  28-Nov-03  DTH  Rewrote eth_devices using universal pcap_findalldevs()
  25-Nov-03  DTH  Verified DECNET_FIX, reversed ifdef to mainstream code
  19-Nov-03  MP   Fixed BPF functionality on Linux/BSD.
  17-Nov-03  DTH  Added xBSD simplification
  14-Nov-03  DTH  Added #ifdef DECNET_FIX for problematic duplicate detection code
  13-Nov-03  DTH  Merged in __FreeBSD__ support
  21-Oct-03  MP   Added enriched packet dumping for debugging
  20-Oct-03  MP   Added support for multiple ethernet devices on VMS
  20-Sep-03  Ankan Add VMS support (Alpha only)
  29-Sep-03  MP   Changed separator character in eth_fmt_mac to be ":" to
                  format ethernet addresses the way the BPF compile engine
                  wants to see them.
                  Added BPF support to filter packets
                  Added missing printf in eth_close
  07-Jun-03  MP   Added WIN32 support for DECNET duplicate address detection.
  06-Jun-03  MP   Fixed formatting of Ethernet Protocol Type in eth_packet_trace
  30-May-03  DTH  Changed WIN32 to _WIN32 for consistency
  07-Mar-03  MP   Fixed Linux implementation of PacketGetAdapterNames to also
                  work on Red Hat 6.2-sparc and Debian 3.0r1-sparc.
  03-Mar-03  MP   Changed logging to be consistent on stdout and sim_log
  01-Feb-03  MP   Changed type of local variables in eth_packet_trace to
                  conform to the interface needs of eth_mac_fmt wich produces
                  char data instead of unsigned char data.  Suggested by the
                  DECC compiler.
  15-Jan-03  DTH  Corrected PacketGetAdapterNames parameter2 datatype
  26-Dec-02  DTH  Merged Mark Pizzolato's enhancements with main source
                  Added networking documentation
                  Changed _DEBUG to ETH_DEBUG
  20-Dec-02  MP   Added display of packet CRC to the eth_packet_trace.
                  This helps distinguish packets with identical lengths
                  and protocols.
  05-Dec-02  MP   With the goal of draining the input buffer more rapidly
                  changed eth_read to call pcap_dispatch repeatedly until
                  either a timeout returns nothing or a packet allowed by
                  the filter is seen.  This more closely reflects how the
                  pcap layer will work when the filtering is actually done
                  by a bpf filter.
  31-Oct-02  DTH  Added USE_NETWORK conditional
                  Reworked not attached test
                  Added OpenBSD support (from Federico Schwindt)
                  Added ethX detection simplification (from Megan Gentry)
                  Removed sections of temporary code
                  Added parameter validation
  23-Oct-02  DTH  Beta 5 released
  22-Oct-02  DTH  Added all_multicast and promiscuous support
                  Fixed not attached behavior
  21-Oct-02  DTH  Added NetBSD support (from Jason Thorpe)
                  Patched buffer size to make sure entire packet is read in
                  Made 'ethX' check characters passed as well as length
                  Corrected copyright again
  16-Oct-02  DTH  Beta 4 released
                  Corrected copyright
  09-Oct-02  DTH  Beta 3 released
                  Added pdp11 write acceleration (from Patrick Caulfield)
  08-Oct-02  DTH  Beta 2 released
                  Integrated with 2.10-0p4
                  Added variable vector and copyrights
  04-Oct-02  DTH  Added linux support (from Patrick Caulfield)
  03-Oct-02  DTH  Beta version of xq/sim_ether released for SIMH 2.09-11
  24-Sep-02  DTH  Finished eth_devices, eth_getname
  18-Sep-02  DTH  Callbacks implemented
  13-Sep-02  DTH  Basic packet read/write written
  20-Aug-02  DTH  Created Sim_Ether for O/S independant ethernet implementation

  ------------------------------------------------------------------------------
*/

#include <ctype.h>
#include "sim_ether.h"
#include "sim_sock.h"

extern FILE *sim_log;

/*============================================================================*/
/*                  OS-independant ethernet routines                          */
/*============================================================================*/

t_stat eth_mac_scan (ETH_MAC* mac, char* strmac)
{
  int i, j;
  short unsigned int num;
  char cptr[18];
  int len = strlen(strmac);
  const ETH_MAC zeros = {0,0,0,0,0,0};
  const ETH_MAC ones  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  ETH_MAC newmac;

  /* format of string must be 6 double-digit hex bytes with valid separators
     ideally, this mac scanner could allow more flexible formatting later */
  if (len != 17) return SCPE_ARG;

  /* copy string to local storage for mangling */
  strcpy(cptr, strmac);

  /* make sure byte separators are OK */
  for (i=2; i<len; i=i+3) {
    if ((cptr[i] != '-') && 
        (cptr[i] != '.') &&
        (cptr[i] != ':')) return SCPE_ARG;
    cptr[i] = '\0';
  }

  /* get and set address bytes */
  for (i=0, j=0; i<len; i=i+3, j++) {
    int valid = strspn(&cptr[i], "0123456789abcdefABCDEF");
    if (valid < 2) return SCPE_ARG;
    sscanf(&cptr[i], "%hx", &num);
    newmac[j] = (unsigned char) num;
  }

  /* final check - mac cannot be broadcast or multicast address */
  if (!memcmp(newmac, zeros, sizeof(ETH_MAC)) ||  /* broadcast */
      !memcmp(newmac, ones,  sizeof(ETH_MAC)) ||  /* broadcast */
      (newmac[0] & 0x01)                          /* multicast */
     )
    return SCPE_ARG;

  /* new mac is OK, copy into passed mac */
  memcpy (*mac, newmac, sizeof(ETH_MAC));
  return SCPE_OK;
}

void eth_mac_fmt(ETH_MAC* mac, char* buff)
{
  uint8* m = (uint8*) mac;
  sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return;
}

static const uint32 crcTable[256] = {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
  0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
  0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
  0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
  0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
  0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
  0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
  0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
  0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
  0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
  0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
  0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
  0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
  0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
  0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
  0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
  0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
  0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
  0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
  0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
  0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
  0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32 eth_crc32(uint32 crc, const void* vbuf, size_t len)
{
  const uint32 mask = 0xFFFFFFFF;
  const unsigned char* buf = (const unsigned char*)vbuf;

  crc ^= mask;
  while (0 != len--)
    crc = (crc >> 8) ^ crcTable[ (crc ^ (*buf++)) & 0xFF ];
  return(crc ^ mask);
}

void eth_add_crc32(ETH_PACK* packet)
{
  if (packet->len <= ETH_MAX_PACKET) {
    uint32 crc = eth_crc32(0, packet->msg, packet->len);  /* calculate CRC */
    uint32 ncrc = htonl(crc);                             /* CRC in network order */
    int size = sizeof(ncrc);                              /* size of crc field */
    memcpy(&packet->msg[packet->len], &ncrc, size);       /* append crc to packet */
    packet->crc_len = packet->len + size;                 /* set packet crc length */
  } else {
    packet->crc_len = 0;                                  /* appending crc would destroy packet */
  }
}

void eth_setcrc(ETH_DEV* dev, int need_crc)
{
  dev->need_crc = need_crc;
}

void eth_packet_trace_ex(ETH_DEV* dev, const uint8 *msg, int len, char* txt, int dmp)
{
  if (dev->dptr->dctrl & dev->dbit) {
    char src[20];
    char dst[20];
    unsigned short* proto = (unsigned short*) &msg[12];
    uint32 crc = eth_crc32(0, msg, len);
    eth_mac_fmt((ETH_MAC*)&msg[0], dst);
    eth_mac_fmt((ETH_MAC*)&msg[6], src);
    sim_debug(dev->dbit, dev->dptr, "%s  dst: %s  src: %s  proto: 0x%04X  len: %d  crc: %X\n",
          txt, dst, src, ntohs(*proto), len, crc);
    if (dmp) {
      int i, same, group, sidx, oidx;
      char outbuf[80], strbuf[18];
      static char hex[] = "0123456789ABCDEF";
      for (i=same=0; i<len; i += 16) {
        if ((i > 0) && (0 == memcmp(&msg[i], &msg[i-16], 16))) {
          ++same;
          continue;
        }
        if (same > 0) {
          sim_debug(dev->dbit, dev->dptr, "%04X thru %04X same as above\r\n", i-(16*same), i-1);
          same = 0;
        }
        group = (((len - i) > 16) ? 16 : (len - i));
        for (sidx=oidx=0; sidx<group; ++sidx) {
          outbuf[oidx++] = ' ';
          outbuf[oidx++] = hex[(msg[i+sidx]>>4)&0xf];
          outbuf[oidx++] = hex[msg[i+sidx]&0xf];
          if (isprint(msg[i+sidx]))
            strbuf[sidx] = msg[i+sidx];
          else
            strbuf[sidx] = '.';
        }
        outbuf[oidx] = '\0';
        strbuf[sidx] = '\0';
        sim_debug(dev->dbit, dev->dptr, "%04X%-48s %s\r\n", i, outbuf, strbuf);
      }
      if (same > 0)
        sim_debug(dev->dbit, dev->dptr, "%04X thru %04X same as above\r\n", i-(16*same), len-1);
    }
  }
}

void eth_packet_trace(ETH_DEV* dev, const uint8 *msg, int len, char* txt)
{
  eth_packet_trace_ex(dev, msg, len, txt, 1/*len > ETH_MAX_PACKET*/);
}


t_stat ethq_init(ETH_QUE* que, int max)
{
  /* create dynamic queue if it does not exist */
  if (!que->item) {
    size_t size = sizeof(struct eth_item) * max;
    que->max = max;
    que->item = (struct eth_item *) malloc(size);
    if (que->item) {
      /* init dynamic memory */
      memset(que->item, 0, size);
    } else {
      /* failed to allocate memory */
      char* msg = "EthQ: failed to allocate dynamic queue[%d]\r\n";
      printf(msg, max);
      if (sim_log) fprintf(sim_log, msg, max);
      return SCPE_MEM;
    };
  };
  return SCPE_OK;
}

void ethq_clear(ETH_QUE* que)
{
  /* clear packet array */
  memset(que->item, 0, sizeof(struct eth_item) * que->max);
  /* clear rest of structure */
  que->count = que->head = que->tail = que->loss = que->high = 0;
}

void ethq_remove(ETH_QUE* que)
{
  struct eth_item* item = &que->item[que->head];

  if (que->count) {
    memset(item, 0, sizeof(struct eth_item));
    if (++que->head == que->max)
      que->head = 0;
    que->count--;
  }
}

void ethq_insert(ETH_QUE* que, int32 type, ETH_PACK* pack, int32 status)
{
  struct eth_item* item;

  /* if queue empty, set pointers to beginning */
  if (!que->count) {
    que->head = 0;
    que->tail = -1;
  }

  /* find new tail of the circular queue */
  if (++que->tail == que->max)
    que->tail = 0;
  if (++que->count > que->max) {
    que->count = que->max;
    /* lose oldest packet */
    if (++que->head == que->max)
      que->head = 0;
    que->loss++;
    }
  if (que->count > que->high)
    que->high = que->count;

  /* set information in (new) tail item */
  item = &que->item[que->tail];
  item->type = type;
  item->packet.len = pack->len;
  item->packet.used = 0;
  item->packet.crc_len = pack->crc_len;
  memcpy(item->packet.msg, pack->msg, ((pack->len > pack->crc_len) ? pack->len : pack->crc_len));
  item->packet.status = status;
}

/*============================================================================*/
/*                        Non-implemented versions                            */
/*============================================================================*/

#if !defined (USE_NETWORK) && !defined(USE_SHARED)
t_stat eth_open(ETH_DEV* dev, char* name, DEVICE* dptr, uint32 dbit)
  {return SCPE_NOFNC;}
t_stat eth_close (ETH_DEV* dev)
  {return SCPE_NOFNC;}
t_stat eth_write (ETH_DEV* dev, ETH_PACK* packet, ETH_PCALLBACK routine)
  {return SCPE_NOFNC;}
t_stat eth_read (ETH_DEV* dev, ETH_PACK* packet, ETH_PCALLBACK routine)
  {return SCPE_NOFNC;}
t_stat eth_filter (ETH_DEV* dev, int addr_count, ETH_MAC* addresses,
                   ETH_BOOL all_multicast, ETH_BOOL promiscuous)
  {return SCPE_NOFNC;}
int eth_devices (int max, ETH_LIST* dev)
  {return -1;}
#else       /* endif unimplemented */

#endif /* USE_NETWORK */
