all: main.o pid.o uart_com.o i2c_com.o csv_pull.o
	gcc $(CFLAGS) -o forno main.o pid.o csv_pull.o i2c_com.o uart_com.o crc/crc16.o -lwiringPi -lm -lpthread -lcrypt -lrt

main.o: main.c i2c_com.h uart_com.h PID/pid.h csv_pull.h
	gcc $(CFLAGS) -c main.c

pid.o: PID/pid.h PID/pid.c
	gcc $(CFLAGS) -c PID/pid.c

i2c_com.o: i2c_com.c i2c_com.h
	gcc $(CFLAGS) -c i2c_com.c

uart_com.o: uart_com.c uart_com.h crc/crc16.h
	gcc $(CFLAGS) -c uart_com.c

crc.o: crc/crc16.h crc/crc16.c
	gcc $(CFLAGS) -c crc/crc16.c

csv_pull.o: csv_pull.h
	gcc $(CFLAGS) -c csv_pull.c


clean:
	rm -f *.o
