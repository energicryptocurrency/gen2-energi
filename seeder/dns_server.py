#!/usr/bin/env python
# coding=utf-8
from mn_rand_ips import MNRandIPs

import argparse
import datetime
import sys
import time
import threading
import traceback
import SocketServer
import struct
import logging

try:
    from dnslib import *
except ImportError:
    logging.error("Missing dependency dnslib: <https://pypi.python.org/pypi/dnslib>. Please install it with `pip`.")
    sys.exit(2)


logging.basicConfig(filename='~/seeder.log', level=logging.DEBUG)

class DomainName(str):
    def __getattr__(self, item):
        return DomainName(item + '.' + self)

DOMAN_NAME = 'dnsseed1.energiplatform.com'
IP = '52.89.51.214'
D = DomainName(DOMAN_NAME)

TTL = 60 * 5

soa_record = SOA(
    mname=D.ns1,  # primary name server
    rname=D.energidev,  
    times=(
        201307231,  # serial number
        60 * 60 * 1,  # refresh
        60 * 60 * 3,  # retry
        60 * 60 * 24,  # expire
        60 * 60 * 1,  # minimum
    )
)
ns_records = [NS(D.ns1), NS(D.ns2)]
records = {
    D: [A(IP), AAAA((0,) * 16), MX(D.mail), soa_record] + ns_records,
    D.ns1: [A(IP)],  # MX and NS records must never point to a CNAME alias (RFC 2181 section 10.3)
    D.ns2: [A(IP)],
    D.mail: [A(IP)],
    D.andrei: [CNAME(D)],
}

mn_rand_ips = MNRandIPs()
mn_rand_ips.refresh_mn_ips()
ips_per_req = 8

_NS_RECORD = "NS"
def dns_response(data):
    request = DNSRecord.parse(data)

    logging.info(request)

    reply = DNSRecord(DNSHeader(id=request.header.id, qr=1, aa=1, ra=1), q=request.q)

    qname = request.q.qname
    qn = str(qname)
    qtype = request.q.qtype
    qt = QTYPE[qtype]

    if qt == "NS":
        reply.add_answer(*RR.fromZone(DOMAN_NAME + " 3600 NS ns1.energiplatform.com"))
        reply.add_ar(*RR.fromZone("ns1.energiplatform.com A " + IP))
    else:
        domain_name_a = DOMAN_NAME + " A "
        for ip in mn_rand_ips.get_random_x_ips( ips_per_req ):
            reply.add_answer(*RR.fromZone(domain_name_a + ip))

    logging.info("---- Reply: %s\n", str(reply) )

    return reply.pack()


class BaseRequestHandler(SocketServer.BaseRequestHandler):

    def get_data(self):
        raise NotImplementedError

    def send_data(self, data):
        raise NotImplementedError

    def handle(self):
        now = datetime.datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')
        logging.info("\n\n%s request %s (%s %s):" % (self.__class__.__name__[:3], now, self.client_address[0],
                                               self.client_address[1]))
        try:
            data = self.get_data()
            logging.info(str(len(data)) + " : " + str(data) )  # repr(data).replace('\\x', '')[1:-1]
            self.send_data(dns_response(data))
        except Exception as e:
            logging.error("Error handling reques" + str(e) )
            logging.error(str(traceback.print_exc()))


class TCPRequestHandler(BaseRequestHandler):

    def get_data(self):
        data = self.request.recv(8192).strip()
        sz = struct.unpack('>H', data[:2])[0]
        if sz < len(data) - 2:
            raise Exception("Wrong size of TCP packet")
        elif sz > len(data) - 2:
            raise Exception("Too big TCP packet")
        return data[2:]

    def send_data(self, data):
        sz = struct.pack('>H', len(data))
        return self.request.sendall(sz + data)


class UDPRequestHandler(BaseRequestHandler):

    def get_data(self):
        return self.request[0].strip()

    def send_data(self, data):
        return self.request[1].sendto(data, self.client_address)


def main():
    parser = argparse.ArgumentParser(description='Start a DNS implemented in Python.')
    parser = argparse.ArgumentParser(description='Start a DNS implemented in Python. Usually DNSs use UDP on port 53.')
    parser.add_argument('--port', default=53, type=int, help='The port to listen on.')
    parser.add_argument('--tcp', action='store_true', help='Listen to TCP connections.')
    parser.add_argument('--udp', action='store_true', help='Listen to UDP datagrams.')
    
    args = parser.parse_args()
    if not (args.udp or args.tcp): parser.error("Please select at least one of --udp or --tcp.")

    logging.info("Starting nameserver...")

    servers = []
    if args.udp: servers.append(SocketServer.ThreadingUDPServer(('', args.port), UDPRequestHandler))
    if args.tcp: servers.append(SocketServer.ThreadingTCPServer(('', args.port), TCPRequestHandler))

    for s in servers:
        thread = threading.Thread(target=s.serve_forever)  # that thread will start one more thread for each request
        thread.daemon = True  # exit the server thread when the main thread terminates
        thread.start()
        logging.info("%s server loop running in thread: %s" % (s.RequestHandlerClass.__name__[:3], thread.name))

    try:
        while 1:
            time.sleep(1)
            sys.stderr.flush()
            sys.stdout.flush()

    except KeyboardInterrupt:
        pass
    finally:
        for s in servers:
            s.shutdown()

if __name__ == '__main__':
    main()
