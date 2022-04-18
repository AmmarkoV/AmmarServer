#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run this script as root"
  exit
fi

echo "Current Linux TCP buffer sizes"
cat /proc/sys/net/ipv4/tcp_mem 

echo "Current receive socket memory"
cat /proc/sys/net/core/rmem_default 
cat /proc/sys/net/core/rmem_max

echo "Current send socket memory"
cat /proc/sys/net/core/wmem_default
cat /proc/sys/net/core/wmem_max

echo "Maximum number of option memory buffers"
cat /proc/sys/net/core/optmem_max 


echo '#AmmarServer tuneTCPIP.sh ( sudo sysctl -p to load )' >> /etc/sysctl.conf
echo 'net.core.rmem_max = 1048576' >> /etc/sysctl.conf
echo 'net.core.wmem_max = 1048576' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_window_scaling = 1' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_timestamps = 0' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_sack = 0' >> /etc/sysctl.conf

echo 'net.ipv4.tcp_no_metrics_save = 1' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_moderate_rcvbuf = 1' >> /etc/sysctl.conf
echo 'net.core.netdev_max_backlog = 10000' >> /etc/sysctl.conf

sysctl -p
tcpdump -ni eth0

exit 0
