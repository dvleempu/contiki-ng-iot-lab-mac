import time
import os
import re
import sys
import numpy as np

class node:
    def __init__(self, id):
        self.id = id
        self.packets = []
        self.sent = np.zeros(100000)
        self.received = np.zeros(100000)
        self.overflows = 0
        self.n_sent = 0

def search_node(nodes,n):
    if(len(nodes) != 0):
        for i in range(0,len(nodes)):
            if(n == nodes[i].id):
                return i
    return -1

REGEXP_SERVER = re.compile('^.*?(?P<mins>([0-9]+)):(?P<secs>([0-9]+)).(?P<mils>([0-9]+))\tID:1\t\[INFO:\sApp\s\s\s\s\s\s\s]\sReceived\s(?P<seqno>([0-9]+))\sfrom\s0(?P<source>([0-9]))')
REGEXP_CLIENT = re.compile('^.*?(?P<mins>([0-9]+)):(?P<secs>([0-9]+)).(?P<mils>([0-9]+))\tID:(?P<source>([0-9]))\t\[INFO:\sApp\s\s\s\s\s\s\s]\sSending\s(?P<seqno>([0-9]+))')
REGEXP_OVERFLOW = re.compile('^.*?ID:(?P<source>([0-9]))\t\[ERR\s:\sCSMA\s\s\s\s\s\s]\sNeighbor queue full')
REGEXP_OVERFLOW2 = re.compile('^.*?ID:(?P<source>([0-9]))\t\[ERR\s:\sTSCH\sQueue]\s!\sadd packet failed')

logfile = open(str(sys.argv[1]))
print("Parsing " + str(sys.argv[1]))
lines = logfile.readlines()

nodes = []

for line in lines:
    chomped_line = line.rstrip()
    server_match = re.match(REGEXP_SERVER, chomped_line)
    if(server_match):
        seqno = int(server_match.group("seqno"))
        source = int(server_match.group("source"))
        mins = int(server_match.group("mins"))
        secs = int(server_match.group("secs"))
        mils = int(server_match.group("mils"))
        received = mils + secs*1000 + mins*1000*60
        if(source != 0):
            node_place = search_node(nodes,source)
            if(node_place == -1):
                nodes.append(node(source))
                node_place = len(nodes)-1
            nodes[node_place].packets.append(seqno)
            nodes[node_place].received[seqno] = received
    client_match = re.match(REGEXP_CLIENT, chomped_line)
    if(client_match):
        seqno = int(client_match.group("seqno"))
        source = int(client_match.group("source"))
        mins = int(client_match.group("mins"))
        secs = int(client_match.group("secs"))
        mils = int(client_match.group("mils"))
        sent = mils + secs*1000 + mins*1000*60
        if(source != 0):
            node_place = search_node(nodes,source)
            if(node_place == -1):
                nodes.append(node(source))
                node_place = len(nodes)-1
            nodes[node_place].sent[seqno] = sent
            nodes[node_place].n_sent += 1
    overflow_match = re.match(REGEXP_OVERFLOW, chomped_line)
    if(overflow_match):
        source = int(overflow_match.group("source"))
        if(source != 0):
            node_place = search_node(nodes,source)
            if(node_place == -1):
                nodes.append(node(source))
                node_place = len(nodes)-1
            nodes[node_place].overflows += 1 
    overflow_match2 = re.match(REGEXP_OVERFLOW2, chomped_line)
    if(overflow_match2):
        source = int(overflow_match2.group("source"))
        if(source != 0):
            node_place = search_node(nodes,source)
            if(node_place == -1):
                nodes.append(node(source))
                node_place = len(nodes)-1
            nodes[node_place].overflows += 1 
#Global values
global_passed_to_mac = []
global_accepted_by_mac = []
global_received = []
global_average_delay = []
global_goodput = []
global_reliability = []

for node in nodes:
    if(len(node.packets) != 0):
        passed_to_mac = node.packets[-1]+1
        accepted_by_mac = node.n_sent - node.overflows
        received = len(node.packets)        
        delays = []
        last = 0
        for i in range(0,len(node.received)):
            if((node.received[i] != 0) and (node.sent[i] != 0)):
                delays.append(node.received[i]-node.sent[i])
                last = node.received[i]
        average_delay = np.mean(delays)
        total_time = last-node.received[0]
        goodput = received/(total_time/1000.0)
        reliability = received/accepted_by_mac

        print("*** Node " + str(node.id) + " ***")
        print("Packets passed to MAC:\t\t" + str(passed_to_mac))
        print("Packets accepted by MAC:\t" + str(accepted_by_mac))
        print("Packets received by server:\t" + str(received))
        print("Average delay:\t\t\t" + str(average_delay) + " ms")
        print("----------")
        print("Goodput:\t\t\t" + str(goodput) + " packets/s")
        print("Reliability:\t\t\t" + str(reliability))
        print()

        global_passed_to_mac.append(passed_to_mac)
        global_accepted_by_mac.append(accepted_by_mac)
        global_received.append(received)
        global_average_delay.append(average_delay)
        global_goodput.append(goodput)
        global_reliability.append(reliability)

print("*** Global ***")
print("Packets passed to MAC:\t\t" + str(np.sum(global_passed_to_mac)))
print("Packets accepted by MAC:\t" + str(np.sum(global_accepted_by_mac)))
print("Packets received by server:\t" + str(np.sum(global_received)))
print("Average delay:\t\t\t" + str(np.mean(global_average_delay)) + " ms")
print("----------")
print("Goodput:\t\t\t" + str(np.sum(global_goodput)) + " packets/s")
print("Reliability:\t\t\t" + str(np.sum(global_received)/np.sum(global_accepted_by_mac)))
print()