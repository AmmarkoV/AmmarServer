EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CONN_01X02 J1
U 1 1 5B90552F
P 800 1550
F 0 "J1" H 800 1700 50  0000 C CNN
F 1 "DC OUT 9V" V 900 1550 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x02_Pitch2.00mm" H 800 1550 50  0001 C CNN
F 3 "" H 800 1550 50  0001 C CNN
	1    800  1550
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X02 J4
U 1 1 5B90562E
P 3250 1600
F 0 "J4" H 3250 1750 50  0000 C CNN
F 1 "DC IN 9V" V 3350 1600 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x02_Pitch2.00mm" H 3250 1600 50  0001 C CNN
F 3 "" H 3250 1600 50  0001 C CNN
	1    3250 1600
	1    0    0    -1  
$EndComp
$Comp
L D D2
U 1 1 5B9056A1
P 2900 1350
F 0 "D2" H 2900 1450 50  0000 C CNN
F 1 "D" H 2900 1250 50  0000 C CNN
F 2 "Diodes_THT:D_DO-41_SOD81_P7.62mm_Horizontal" H 2900 1350 50  0001 C CNN
F 3 "" H 2900 1350 50  0001 C CNN
	1    2900 1350
	1    0    0    -1  
$EndComp
$Comp
L D D1
U 1 1 5B9057E6
P 1700 1500
F 0 "D1" H 1700 1600 50  0000 C CNN
F 1 "D" H 1700 1400 50  0000 C CNN
F 2 "Diodes_THT:D_DO-41_SOD81_P7.62mm_Horizontal" H 1700 1500 50  0001 C CNN
F 3 "" H 1700 1500 50  0001 C CNN
	1    1700 1500
	1    0    0    -1  
$EndComp
$Comp
L R R3
U 1 1 5B905A4E
P 2500 1350
F 0 "R3" V 2580 1350 50  0000 C CNN
F 1 "1k" V 2500 1350 50  0000 C CNN
F 2 "Resistors_SMD:R_1210_HandSoldering" V 2430 1350 50  0001 C CNN
F 3 "" H 2500 1350 50  0001 C CNN
	1    2500 1350
	0    1    1    0   
$EndComp
$Comp
L CONN_01X02 J3
U 1 1 5B905B9F
P 2150 1550
F 0 "J3" H 2150 1700 50  0000 C CNN
F 1 "BATT 9V" V 2250 1550 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x02_Pitch2.00mm" H 2150 1550 50  0001 C CNN
F 3 "" H 2150 1550 50  0001 C CNN
	1    2150 1550
	1    0    0    -1  
$EndComp
$Comp
L CP C1
U 1 1 5B905FA9
P 1100 1550
F 0 "C1" H 1125 1650 50  0000 L CNN
F 1 "CP" H 1125 1450 50  0000 L CNN
F 2 "Capacitors_SMD:CP_Elec_6.3x7.7" H 1138 1400 50  0001 C CNN
F 3 "" H 1100 1550 50  0001 C CNN
	1    1100 1550
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5B906257
P 1350 1250
F 0 "R1" V 1430 1250 50  0000 C CNN
F 1 "10k" V 1350 1250 50  0000 C CNN
F 2 "Resistors_SMD:R_1210_HandSoldering" V 1280 1250 50  0001 C CNN
F 3 "" H 1350 1250 50  0001 C CNN
	1    1350 1250
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5B9062B2
P 1350 1850
F 0 "R2" V 1430 1850 50  0000 C CNN
F 1 "10k" V 1350 1850 50  0000 C CNN
F 2 "Resistors_SMD:R_1210_HandSoldering" V 1280 1850 50  0001 C CNN
F 3 "" H 1350 1850 50  0001 C CNN
	1    1350 1850
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X01 J2
U 1 1 5B90678D
P 1450 1550
F 0 "J2" H 1450 1650 50  0000 C CNN
F 1 "VOLTAGE SENSOR" V 1550 1550 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Angled_1x01_Pitch2.00mm" H 1450 1550 50  0001 C CNN
F 3 "" H 1450 1550 50  0001 C CNN
	1    1450 1550
	1    0    0    -1  
$EndComp
Connection ~ 1500 1100
Wire Wire Line
	3050 1550 3050 1350
Wire Wire Line
	2750 1350 2650 1350
Wire Wire Line
	2350 1350 1950 1350
Wire Wire Line
	1950 1100 1950 1500
Wire Wire Line
	1950 1500 1850 1500
Wire Wire Line
	1550 1500 1550 1100
Wire Wire Line
	1100 1100 1950 1100
Connection ~ 1950 1350
Wire Wire Line
	1350 1400 1350 1700
Wire Wire Line
	1350 1550 1250 1550
Wire Wire Line
	1100 1100 1100 1400
Connection ~ 1350 1100
Wire Wire Line
	1950 2000 1100 2000
Wire Wire Line
	1100 2000 1100 1700
Wire Wire Line
	1100 1400 600  1400
Wire Wire Line
	600  1400 600  1500
Wire Wire Line
	1100 1700 600  1700
Wire Wire Line
	600  1700 600  1600
Wire Wire Line
	3050 1650 1950 1650
Wire Wire Line
	1950 1600 1950 2000
Connection ~ 1350 2000
Wire Wire Line
	1550 1100 1500 1100
Connection ~ 1550 1100
Connection ~ 1950 1650
Connection ~ 1350 1550
$EndSCHEMATC
