# WifiPerformanceAnalysis
Evaluating the performance of an IEEE 802.11ac network in terms of aggregated throughput with different station (STA) counts using both the DCF/EDCA mechanism and the RTS/CTS mechanism using the NS3 network simulator

•EDCA
•TXOP
•Block ACK
•Frame aggregation

Number of STA is selected as a 4, 8, 12, 20.
ALL STAs are very close to the AP so they are using the highest possible MCS level.
The MCS selection algorithm of the simulator is selected as the well-known Minstrel algorithm.
There is only uplink traffic in the network which uses the TCP protocol.
The total load of the STAs is a parameter (totalLoadPercent) which is given as a percentage of the raw data rate of 20 MHz, 1x1 MIMO which is selected as: 50%, 80%, and 90%.

The result of the project given in the files.
