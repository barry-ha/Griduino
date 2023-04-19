# AutoGridCL 
#   A command-line program to update the grid square in WSJT-X from Griduino in real time.
#   Barry Hansen K7BWH, barry@k7bwh.com
#   
#  This work is based on https://github.com/bmo/py-wsjtx, by Brian Moran N9ADG

# ---------- Start: manual edit ---------------------------------------
# Windows communications port for Griduino or GPS hardware
GPS_PORT = "COM53"
GPS_RATE = 115200

# WSJT-X UDP multicast settings
MULTICAST_ADDR = "224.0.0.1"
MULTICAST_PORT = 2237

# ---------- End: manual edit -----------------------------------------

#  Licensed under the GNU General Public License v3.0
#
#  Permissions of this strong copyleft license are conditioned on making available
#  complete source code of licensed works and modifications, which include larger
#  works using a licensed work, under the same license. Copyright and license
#  notices must be preserved. Contributors provide an express grant of patent rights.
#
#  You may obtain a copy of the License at
#      https://www.gnu.org/licenses/gpl-3.0.en.html
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# ---------------------------------------------------------------------
#
#  Inserted inline from: 
#    \Documents\Arduino\libraries-python\py-wsjtx\pywsjtx\extra\latlong_to_grid_square.py
#
# ---------------------------------------------------------------------

class GPSException(Exception):
    def __init__(self,*args):
        super(GPSException, self).__init__(*args)

# From K6WRU via stackexchange : see https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square/244#244
# Convert latitude and longitude to Maidenhead grid locators.
#
# Arguments are in signed decimal latitude and longitude. For example,
# the location of my QTH Palo Alto, CA is: 37.429167, -122.138056 or
# in degrees, minutes, and seconds: 37° 24' 49" N 122° 6' 26" W
class LatLongToGridSquare(object):
    upper = 'ABCDEFGHIJKLMNOPQRSTUVWX'
    lower = 'abcdefghijklmnopqrstuvwx'

    @classmethod
    def to_grid(cls,dec_lat, dec_lon):

        if not (-180<=dec_lon<180):
            raise GPSException('longitude must be -180<=lon<180, given %f\n'%dec_lon)
        if not (-90<=dec_lat<90):
            raise GPSException('latitude must be -90<=lat<90, given %f\n'%dec_lat)

        adj_lat = dec_lat + 90.0
        adj_lon = dec_lon + 180.0

        grid_lat_sq = LatLongToGridSquare.upper[int(adj_lat/10)]
        grid_lon_sq = LatLongToGridSquare.upper[int(adj_lon/20)]

        grid_lat_field = str(int(adj_lat%10))
        grid_lon_field = str(int((adj_lon/2)%10))

        adj_lat_remainder = (adj_lat - int(adj_lat)) * 60
        adj_lon_remainder = ((adj_lon) - int(adj_lon/2)*2) * 60

        grid_lat_subsq = LatLongToGridSquare.lower[int(adj_lat_remainder/2.5)]
        grid_lon_subsq = LatLongToGridSquare.lower[int(adj_lon_remainder/5)]

        return grid_lon_sq + grid_lat_sq + grid_lon_field + grid_lat_field + grid_lon_subsq + grid_lat_subsq

    # GPS sentences are encoded
    @classmethod
    def convert_to_degrees(cls, gps_value, direction):
        if direction not in ['N','S','E','W']:
            raise GPSException("Invalid direction specifier for lat/long: {}".format(direction))

        dir_mult = 1
        if direction in ['S','W']:
            dir_mult = -1

        if len(gps_value) < 3:
            raise GPSException("Invalid Value for lat/long: {}".format(gps_value))

        dot_posn = gps_value.index('.')

        if dot_posn < 0:
            raise GPSException("Invalid Format for lat/long: {}".format(gps_value))

        degrees = gps_value[0:dot_posn-2]
        mins = gps_value[dot_posn-2:]

        f_degrees = dir_mult * (float(degrees) + (float(mins) / 60.0))
        return f_degrees

    @classmethod
    def NMEA_to_grid(cls, nmea_text):
        grid = ""
        if nmea_text.startswith('$GPGLL'):
            grid = LatLongToGridSquare.GPGLL_to_grid(nmea_text)
        if nmea_text.startswith('$GPGGA'):
            grid = LatLongToGridSquare.GPGGA_to_grid(nmea_text)
        return grid

    @classmethod
    def GPGLL_to_grid(cls, GPSLLText):
       # example: $GPGLL,4740.99254,N,12212.31179,W,223311.00,A,A*70\r\n
       try:
           components = GPSLLText.split(",")
           if components[0]=='$GPGLL':
               del components[0]
           if components[5] != 'A':
               raise GPSException("Not a valid GPS fix")
           lat = LatLongToGridSquare.convert_to_degrees(components[0], components[1])
           long = LatLongToGridSquare.convert_to_degrees(components[2], components [3])
           grid = LatLongToGridSquare.to_grid(lat, long)
       except GPSException:
           grid = ""
       return grid
       
    @classmethod
    def GPGGA_to_grid(cls, GPGGAText):
       # example: $GPGGA,042845.000,4745.1848,N,12217.0699,W,1,04,1.86,-0.1,M,-17.2,M,,*79\r\n
       try:
           components = GPGGAText.split(",")
           if components[0]=='$GPGGA':
               del components[0]
           if components[5] != '1':
               raise GPSException("Not a valid GPS fix")
           lat = LatLongToGridSquare.convert_to_degrees(components[1], components[2])
           long = LatLongToGridSquare.convert_to_degrees(components[3], components [4])
           grid = LatLongToGridSquare.to_grid(lat, long)
       except GPSException:
           grid = ""
       return grid

# ---------------------------------------------------------------------
#
#  Inserted inline from: 
#    \Documents\Arduino\libraries-python\py-wsjtx\pywsjtx\extra\wsjtx_packets.py
#
# ---------------------------------------------------------------------

import struct
import datetime

class PacketUtil:
    @classmethod
    # this hexdump brought to you by Stack Overflow
    def hexdump(cls, src, length=16):
        FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
        lines = []
        for c in range(0, len(src), length):
            chars = src[c:c + length]
            hex = ' '.join(["%02x" % x for x in chars])
            printable = ''.join(["%s" % ((x <= 127 and FILTER[x]) or '.') for x in chars])
            lines.append("%04x  %-*s  %s\n" % (c, length * 3, hex, printable))
        return ''.join(lines)

    # timezone tomfoolery
    @classmethod
    def midnight_utc(cls):
        utcnow = datetime.utcnow()
        utcmidnight = datetime(utcnow.year, utcnow.month, utcnow.day, 0, 0)
        return utcmidnight


class PacketWriter(object):
    def __init__(self ):
        self.ptr_pos = 0
        self.packet = bytearray()
        # self.max_ptr_pos
        self.write_header()

    def write_header(self):
        self.write_QUInt32(GenericWSJTXPacket.MAGIC_NUMBER)
        self.write_QInt32(GenericWSJTXPacket.SCHEMA_VERSION)

    def write_QInt8(self, val):
        self.packet.extend(struct.pack('>b', val))

    def write_QUInt8(self, val):
        self.packet.extend(struct.pack('>B', val))

    def write_QBool(self, val):
        self.packet.extend(struct.pack('>?', val))

    def write_QInt16(self, val):
        self.packet.extend(struct.pack('>h', val))

    def write_QUInt16(self, val):
        self.packet.extend(struct.pack('>H', val))

    def write_QInt32(self, val):
        self.packet.extend(struct.pack('>l',val))

    def write_QUInt32(self, val):
        self.packet.extend(struct.pack('>L', val))

    def write_QInt64(self, val):
        self.packet.extend(struct.pack('>q',val))

    def write_QFloat(self, val):
        self.packet.extend(struct.pack('>d', val))

    def write_QString(self, str_val):

        b_values = str_val
        if type(str_val) != bytes:
            b_values = str_val.encode()
        length = len(b_values)
        self.write_QInt32(length)
        self.packet.extend(b_values)

class PacketReader(object):
    def __init__(self, packet):
        self.ptr_pos = 0
        self.packet = packet
        self.max_ptr_pos = len(packet)-1
        self.skip_header()

    def at_eof(self):
        return self.ptr_pos > self.max_ptr_pos

    def skip_header(self):
        if self.max_ptr_pos < 8:
            raise Exception('Not enough data to skip header')
        self.ptr_pos = 8

    def check_ptr_bound(self,field_type, length):
        if self.ptr_pos + length > self.max_ptr_pos+1:
            raise Exception('Not enough data to extract {}'.format(field_type))

    ## grab data from the packet, incrementing the ptr_pos on the basis of the data we've gleaned
    def QInt32(self):
        self.check_ptr_bound('QInt32', 4)   # sure we could inspect that, but that is slow.
        (the_int32,) = struct.unpack('>l',self.packet[self.ptr_pos:self.ptr_pos+4])
        self.ptr_pos += 4
        return the_int32


    def QInt8(self):
        self.check_ptr_bound('QInt8', 1)
        (the_int8,) = struct.unpack('>b', self.packet[self.ptr_pos:self.ptr_pos+1])
        self.ptr_pos += 1
        return the_int8

    def QInt64(self):
        self.check_ptr_bound('QInt64', 8)
        (the_int64,) = struct.unpack('>q', self.packet[self.ptr_pos:self.ptr_pos+8])
        self.ptr_pos += 8
        return the_int64

    def QFloat(self):
        self.check_ptr_bound('QFloat', 8)
        (the_double,) = struct.unpack('>d', self.packet[self.ptr_pos:self.ptr_pos+8])
        self.ptr_pos += 8
        return the_double

    def QString(self):
        str_len = self.QInt32()
        if str_len == -1:
            return None
        self.check_ptr_bound('QString[{}]'.format(str_len),str_len)
        (str,) = struct.unpack('{}s'.format(str_len), self.packet[self.ptr_pos:self.ptr_pos + str_len])
        self.ptr_pos += str_len
        return str.decode('utf-8')

class GenericWSJTXPacket(object):
    SCHEMA_VERSION = 3
    MINIMUM_SCHEMA_SUPPORTED = 2
    MAXIMUM_SCHEMA_SUPPORTED = 3
    MINIMUM_NETWORK_MESSAGE_SIZE = 8
    MAXIMUM_NETWORK_MESSAGE_SIZE = 2048
    MAGIC_NUMBER = 0xadbccbda

    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        self.addr_port = addr_port
        self.magic = magic
        self.schema = schema
        self.pkt_type = pkt_type
        self.id = id
        self.pkt = pkt

class InvalidPacket(GenericWSJTXPacket):
    TYPE_VALUE = -1
    def __init__(self, addr_port, packet,  message):
        self.packet = packet
        self.message = message
        self.addr_port = addr_port

    def __repr__(self):
        return 'Invalid Packet: %s from %s:%s\n%s' % (self.message, self.addr_port[0], self.addr_port[1], PacketUtil.hexdump(self.packet))

class HeartBeatPacket(GenericWSJTXPacket):
    TYPE_VALUE = 0

    def __init__(self, addr_port: object, magic: object, schema: object, pkt_type: object, id: object, pkt: object) -> object:
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        ps = PacketReader(pkt)
        the_type = ps.QInt32()
        self.wsjtx_id = ps.QString()
        self.max_schema = ps.QInt32()
        self.version = ps.QInt8()
        self.revision = ps.QInt8()

    def __repr__(self):
        return 'HeartBeatPacket: from {}:{}\n\twsjtx id:{}\tmax_schema:{}\tschema:{}\tversion:{}\trevision:{}' .format(self.addr_port[0], self.addr_port[1],
                                                                                                      self.wsjtx_id, self.max_schema, self.schema, self.version, self.revision)
    @classmethod
    # make a heartbeat packet (a byte array) we can send to a 'client'. This should be it's own class.
    def Builder(cls,wsjtx_id='pywsjtx', max_schema=2, version=1, revision=1):
        # build the packet to send
        pkt = PacketWriter()
        pkt.write_QInt32(HeartBeatPacket.TYPE_VALUE)
        pkt.write_QString(wsjtx_id)
        pkt.write_QInt32(max_schema)
        pkt.write_QInt32(version)
        pkt.write_QInt32(revision)
        return pkt.packet

class StatusPacket(GenericWSJTXPacket):
    TYPE_VALUE = 1
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        ps = PacketReader(pkt)
        the_type = ps.QInt32()
        self.wsjtx_id = ps.QString()
        self.dial_frequency = ps.QInt64()

        self.mode = ps.QString()
        self.dx_call = ps.QString()

        self.report = ps.QString()
        self.tx_mode = ps.QString()

        self.tx_enabled = ps.QInt8()
        self.transmitting = ps.QInt8()
        self.decoding = ps.QInt8()
        self.rx_df = ps.QInt32()
        self.tx_df = ps.QInt32()


        self.de_call = ps.QString()

        self.de_grid = ps.QString()
        self.dx_grid = ps.QString()

        self.tx_watchdog = ps.QInt8()
        self.sub_mode = ps.QString()
        self.fast_mode = ps.QInt8()

        # new in wsjtx-2.0.0
        self.special_op_mode = ps.QInt8()

    def __repr__(self):
        str =  'StatusPacket: from {}:{}\n\twsjtx id:{}\tde_call:{}\tde_grid:{}\n'.format(self.addr_port[0], self.addr_port[1],self.wsjtx_id,
                                                                                                 self.de_call, self.de_grid)
        str += "\tfrequency:{}\trx_df:{}\ttx_df:{}\tdx_call:{}\tdx_grid:{}\treport:{}\n".format(self.dial_frequency, self.rx_df, self.tx_df, self.dx_call, self.dx_grid, self.report)
        str += "\ttransmitting:{}\t decoding:{}\ttx_enabled:{}\ttx_watchdog:{}\tsub_mode:{}\tfast_mode:{}\tspecial_op_mode:{}".format(self.transmitting, self.decoding, self.tx_enabled, self.tx_watchdog,
                                                                                                                  self.sub_mode, self.fast_mode, self.special_op_mode)
        return str

class DecodePacket(GenericWSJTXPacket):
    TYPE_VALUE = 2
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        # handle packet-specific stuff.
        ps = PacketReader(pkt)
        the_type = ps.QInt32()
        self.wsjtx_id = ps.QString()
        self.new_decode = ps.QInt8()
        self.millis_since_midnight = ps.QInt32()
        self.time = PacketUtil.midnight_utc() + datetime.timedelta(milliseconds=self.millis_since_midnight)
        self.snr = ps.QInt32()
        self.delta_t = ps.QFloat()
        self.delta_f = ps.QInt32()
        self.mode = ps.QString()
        self.message = ps.QString()
        self.low_confidence = ps.QInt8()
        self.off_air = ps.QInt8()

    def __repr__(self):
        str = 'DecodePacket: from {}:{}\n\twsjtx id:{}\tmessage:{}\n'.format(self.addr_port[0],
                                                                                         self.addr_port[1],
                                                                                         self.wsjtx_id,
                                                                                         self.message)
        str += "\tdelta_f:{}\tnew:{}\ttime:{}\tsnr:{}\tdelta_f:{}\tmode:{}".format(self.delta_f,
                                                                                                self.new_decode,
                                                                                                self.time,
                                                                                                self.snr,
                                                                                                self.delta_f,
                                                                                                self.mode)
        return str

class ClearPacket(GenericWSJTXPacket):
    TYPE_VALUE = 3
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class ReplyPacket(GenericWSJTXPacket):
    TYPE_VALUE = 4
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class QSOLoggedPacket(GenericWSJTXPacket):
    TYPE_VALUE = 5
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class ClosePacket(GenericWSJTXPacket):
    TYPE_VALUE = 6
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class ReplayPacket(GenericWSJTXPacket):
    TYPE_VALUE = 7
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class HaltTxPacket(GenericWSJTXPacket):
    TYPE_VALUE = 8
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class FreeTextPacket(GenericWSJTXPacket):
    TYPE_VALUE = 9
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        # handle packet-specific stuff.

    @classmethod
    def Builder(cls,to_wsjtx_id='WSJT-X', text="", send=False):
        # build the packet to send
        pkt = PacketWriter()
        print('To_wsjtx_id ',to_wsjtx_id,' text ',text, 'send ',send)
        pkt.write_QInt32(FreeTextPacket.TYPE_VALUE)
        pkt.write_QString(to_wsjtx_id)
        pkt.write_QString(text)
        pkt.write_QInt8(send)
        return pkt.packet

class WSPRDecodePacket(GenericWSJTXPacket):
    TYPE_VALUE = 10
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)

class LocationChangePacket(GenericWSJTXPacket):
    TYPE_VALUE = 11
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        # handle packet-specific stuff.

    @classmethod
    def Builder(cls, to_wsjtx_id='WSJT-X', new_grid=""):
        # build the packet to send
        pkt = PacketWriter()
        pkt.write_QInt32(LocationChangePacket.TYPE_VALUE)
        pkt.write_QString(to_wsjtx_id)
        pkt.write_QString(new_grid)
        return pkt.packet

class LoggedADIFPacket(GenericWSJTXPacket):
    TYPE_VALUE = 12
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        GenericWSJTXPacket.__init__(self, addr_port, magic, schema, pkt_type, id, pkt)
        # handle packet-specific stuff.

    @classmethod
    def Builder(cls, to_wsjtx_id='WSJT-X', adif_text=""):
        # build the packet to send
        pkt = PacketWriter()
        pkt.write_QInt32(LoggedADIFPacket.TYPE_VALUE)
        pkt.write_QString(to_wsjtx_id)
        pkt.write_QString(adif_text)
        return pkt.packet

class WSJTXPacketClassFactory(GenericWSJTXPacket):

    PACKET_TYPE_TO_OBJ_MAP = {
        HeartBeatPacket.TYPE_VALUE: HeartBeatPacket,
        StatusPacket.TYPE_VALUE:    StatusPacket,
        DecodePacket.TYPE_VALUE:    DecodePacket,
        ClearPacket.TYPE_VALUE:     ClearPacket,
        ReplyPacket.TYPE_VALUE:    ReplyPacket,
        QSOLoggedPacket.TYPE_VALUE: QSOLoggedPacket,
        ClosePacket.TYPE_VALUE:     ClosePacket,
        ReplayPacket.TYPE_VALUE:    ReplayPacket,
        HaltTxPacket.TYPE_VALUE:    HaltTxPacket,
        FreeTextPacket.TYPE_VALUE:  FreeTextPacket,
        WSPRDecodePacket.TYPE_VALUE: WSPRDecodePacket
    }
    def __init__(self, addr_port, magic, schema, pkt_type, id, pkt):
        self.addr_port = addr_port
        self.magic = magic
        self.schema = schema
        self.pkt_type = pkt_type
        self.pkt_id = id
        self.pkt = pkt

    def __repr__(self):
        return 'WSJTXPacketFactory: from {}:{}\n{}' .format(self.addr_port[0], self.addr_port[1], PacketUtil.hexdump(self.pkt))

    # Factory-like method
    @classmethod
    def from_udp_packet(cls, addr_port, udp_packet):
        if len(udp_packet) < GenericWSJTXPacket.MINIMUM_NETWORK_MESSAGE_SIZE:
            return InvalidPacket( addr_port, udp_packet, "Packet too small")

        if len(udp_packet) > GenericWSJTXPacket.MAXIMUM_NETWORK_MESSAGE_SIZE:
            return InvalidPacket( addr_port, udp_packet, "Packet too large")

        (magic, schema, pkt_type, id_len) = struct.unpack('>LLLL', udp_packet[0:16])

        if magic != GenericWSJTXPacket.MAGIC_NUMBER:
            return InvalidPacket( addr_port, udp_packet, "Invalid Magic Value")

        if schema < GenericWSJTXPacket.MINIMUM_SCHEMA_SUPPORTED or schema > GenericWSJTXPacket.MAXIMUM_SCHEMA_SUPPORTED:
            return InvalidPacket( addr_port, udp_packet, "Unsupported schema value {}".format(schema))
        klass = WSJTXPacketClassFactory.PACKET_TYPE_TO_OBJ_MAP.get(pkt_type)

        if klass is None:
            return InvalidPacket( addr_port, udp_packet, "Unknown packet type {}".format(pkt_type))

        return klass(addr_port, magic, schema, pkt_type, id, udp_packet)


# ---------------------------------------------------------------------
#
#  Inserted inline from: 
#    \Documents\Arduino\libraries-python\py-wsjtx\pywsjtx\extra\simple_server.py
#
# ---------------------------------------------------------------------
#
# In WSJTX parlance, the 'network server' is a program external to the wsjtx.exe program that handles packets emitted by wsjtx
#
# TODO: handle multicast groups.
#
# see dump_wsjtx_packets.py example for some simple usage
#
import socket
import struct
#bwh import pywsjtx
import logging
import ipaddress

class SimpleServer(object):
    logger = logging.getLogger()
    MAX_BUFFER_SIZE = GenericWSJTXPacket.MAXIMUM_NETWORK_MESSAGE_SIZE
    DEFAULT_UDP_PORT = 2237
    #
    #
    def __init__(self, ip_address='127.0.0.1', udp_port=DEFAULT_UDP_PORT, **kwargs):
        self.timeout = None
        self.verbose = kwargs.get("verbose",False)

        if kwargs.get("timeout") is not None:
            self.timeout = kwargs.get("timeout")

        the_address = ipaddress.ip_address(ip_address)
        if not the_address.is_multicast:
            self.sock = socket.socket(socket.AF_INET,  # Internet
                                 socket.SOCK_DGRAM)  # UDP

            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.sock.bind((ip_address, int(udp_port)))
        else:
            self.multicast_setup(ip_address, udp_port)

        if self.timeout is not None:
            self.sock.settimeout(self.timeout)

    def multicast_setup(self, group, port=''):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('', port))
        mreq = struct.pack("4sl", socket.inet_aton(group), socket.INADDR_ANY)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    def rx_packet(self):
        try:
            pkt, addr_port = self.sock.recvfrom(self.MAX_BUFFER_SIZE)  # buffer size is 1024 bytes
            return(pkt, addr_port)
        except socket.timeout:
            if self.verbose:
                logging.debug("rx_packet: socket.timeout")
            return (None, None)

    def send_packet(self, addr_port, pkt):
        bytes_sent = self.sock.sendto(pkt,addr_port)
        self.logger.debug("send_packet: Bytes sent {} ".format(bytes_sent))

    def demo_run(self):
        while True:
            (pkt, addr_port) = self.rx_packet()
            if (pkt != None):
                the_packet = WSJTXPacketClassFactory.from_udp_packet(addr_port, pkt)
                print(the_packet)

# ---------------------------------------------------------------------
# using standard NMEA sentences
import os
import sys
import threading
from datetime import datetime
import serial
import logging  # https://docs.python.org/3/library/logging.html

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
#bwh import pywsjtx.wsjtx_packets
#bwh import pywsjtx.simple_server
#bwh import pywsjtx.latlong_to_grid_square

logging.basicConfig(level=logging.WARNING)  # 30
# logging.basicConfig(level=logging.INFO)     # 20
# logging.basicConfig(level=logging.DEBUG)    # 10


class NMEALocation(object):
    # parse the NMEA message for location into a grid square

    def __init__(self, grid_changed_callback=None):
        self.valid = False
        self.grid = ""  # this is an attribute, so allegedly doesn't need locking when used with multiple threads.
        self.last_fix_at = None
        self.grid_changed_callback = grid_changed_callback

    def handle_serial(self, text):
        # should be a single line.
        if text.startswith("$GP"):
            logging.debug("nmea gps sentence: {}".format(text.rstrip()))
            grid = LatLongToGridSquare.NMEA_to_grid(text)

            if grid != "":
                self.valid = True
                self.last_fix_at = datetime.utcnow()
            else:
                self.valid = False

            if grid != "" and self.grid != grid:
                logging.debug(
                    "NMEALocation - grid mismatch old: {} new: {}".format(
                        self.grid, grid
                    )
                )
                self.grid = grid
                if self.grid_changed_callback:
                    c_thr = threading.Thread(
                        target=self.grid_changed_callback, args=(grid,), kwargs={}
                    )
                    c_thr.start()


class SerialGPS(object):
    def __init__(self):
        self.line_handlers = []
        self.comm_thread = None
        self.comm_device = None
        self.stop_signalled = False

    def add_handler(self, line_handler):
        if (not (line_handler is None)) and (not (line_handler in self.line_handlers)):
            self.line_handlers.append(line_handler)

    def open(self, comport, baud, line_handler, **serial_kwargs):
        if self.comm_device is not None:
            logging.warning("serial: GPS comm_device already open")
            self.close()
        self.stop_signalled = False
        logging.info("serial: opening {} at {}".format(GPS_PORT, GPS_RATE))
        self.comm_device = serial.Serial(comport, baud, **serial_kwargs)
        if self.comm_device is not None:
            self.add_handler(line_handler)
            self.comm_thread = threading.Thread(target=self.serial_worker, args=())
            self.comm_thread.start()

    def close(self):
        self.stop_signalled = True
        self.comm_thread.join()

        self.comm_device.close()
        self.line_handlers = []
        self.comm_device = None
        self.stop_signalled = False

    def remove_handler(self, line_handler):
        self.line_handlers.remove(line_handler)

    def serial_worker(self):
        while True:
            if self.stop_signalled:
                return  # terminate
            line = self.comm_device.readline()
            # dispatch the line
            # note that bytes are used for readline, vs strings after the decode to utf-8
            if line.startswith(b"$"):
                try:
                    str_line = line.decode("utf-8")
                    for p in self.line_handlers:
                        p(str_line)
                except UnicodeDecodeError as ex:
                    logging.debug(
                        "serial_worker: {} - line: {}".format(
                            ex, [hex(c) for c in line]
                        )
                    )

    @classmethod
    def example_line_handler(cls, text):
        print("serial: ", text)


# set up the serial_gps to run
# get location data from the GPS, update the grid
# get the grid out of the status message from the WSJT-X instance

# if we have a grid, and it's not the same as GPS, then make it the same by sending the message.
# But only do that if triggered by a status message.

wsjtx_id = None
nmea_p = None
gps_grid = ""


def grid_callback(new_grid):
    global gps_grid
    print("New Grid! {}".format(new_grid))
    # this sets the
    gps_grid = new_grid


sgps = SerialGPS()

print(
    "Starting UDP server for WSJT-X on {} port {}".format(
        MULTICAST_ADDR, MULTICAST_PORT
    )
)
s = SimpleServer(MULTICAST_ADDR, MULTICAST_PORT)


while True:

    (pkt, addr_port) = s.rx_packet()
    if pkt != None:
        the_packet = WSJTXPacketClassFactory.from_udp_packet(addr_port, pkt)
        if wsjtx_id is None and (type(the_packet) == HeartBeatPacket):
            # we have an instance of WSJT-X
            print("WSJT-X detected, id is {}".format(the_packet.wsjtx_id))
            wsjtx_id = the_packet.wsjtx_id

            # start up the GPS reader
            print("Starting GPS monitoring")
            nmea_p = NMEALocation(grid_callback)
            sgps.open(GPS_PORT, GPS_RATE, nmea_p.handle_serial, timeout=1.2)

        if type(the_packet) == StatusPacket:
            if gps_grid != "" and the_packet.de_grid.lower() != gps_grid.lower():
                print(
                    "Sending Grid Change to WSJT-X, old grid:{} new grid: {}".format(
                        the_packet.de_grid, gps_grid
                    )
                )
                grid_change_packet = LocationChangePacket.Builder(
                    wsjtx_id, "GRID:" + gps_grid
                )
                logging.debug(PacketUtil.hexdump(grid_change_packet))
                s.send_packet(the_packet.addr_port, grid_change_packet)
                # for fun, change the TX5 message to our grid square, so we don't have to call CQ again
                # this only works if the length of the free text message is less than 13 characters.
                # if len(the_packet.de_call <= 5):
                #   free_text_packet = pywsjtx.FreeTextPacket.Builder(wsjtx_id,"73 {} {}".format(the_packet.de_call, the_packet[0:4]),False)
                #   s.send_packet(addr_port, free_text_packet)

        print(the_packet)
