# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
# include <mdlint.h>
# include <sys/stat.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <strings.h>
# include <termios.h>
int device_fd;

mdl_u8_t load_prog_mem_cmd = 0x1;
mdl_u8_t read_prog_mem_cmd = 0x2;

mdl_u8_t load_conf_cmd = 0x7;
mdl_u8_t reset_cmd = 0x4;
mdl_u8_t bulk_erase_cmd = 0x5;
mdl_u8_t incr_addr_cmd = 0x3;
mdl_u8_t end_prog_cmd = 0x8;
mdl_u8_t load_data_cmd = 0xB;
mdl_u8_t read_data_cmd = 0xC;

mdl_u8_t load_data_mem_cmd = 0x9;
mdl_u8_t read_data_mem_cmd = 0xA;
mdl_u8_t begin_prog_only_cycle_cmd = 0xD;
mdl_u8_t begin_erase_prog_cycle_cmd = 0xE;

mdl_i8_t send_cmd(mdl_u8_t __cmd) {
	write(device_fd, &__cmd, sizeof(mdl_u8_t));

	mdl_uint_t try_count = 0;
	mdl_uint_t max_trys = 20;
	for (;;) {
		mdl_u8_t ack = 0;
		read(device_fd, &ack, sizeof(mdl_u8_t));
		usleep(1000);
		if (ack) {
			printf("ack not valid. trys: %u, max: %u\n", try_count, max_trys);
			if (try_count >= max_trys) {
				fprintf(stderr, "failed to send command.\n");
				return -1;
			}

			try_count++;
		} else {
			printf("ack success.\n");
			break;
		}
	}

	return 0;
}

mdl_i8_t load_data(mdl_u16_t __data) {
	if (send_cmd(load_data_cmd) < 0) return -1;

	mdl_uint_t tr = 0;
	tr = write(device_fd, (mdl_u8_t*)&__data, sizeof(mdl_u16_t));
	if (tr != sizeof(mdl_u16_t))
		return -1;

	usleep(1000);
	return 0;
}

mdl_i8_t read_data(mdl_u16_t *__data) {
	if (send_cmd(read_data_cmd) < 0) return -1;

	mdl_uint_t tr = 0;
	tr = read(device_fd, (mdl_u8_t*)__data, sizeof(mdl_u16_t));
	if (tr != sizeof(mdl_u16_t))
		return -1;

	usleep(1000);
	return 0;
}

mdl_i8_t load_prog_mem(mdl_u16_t __data) {
	if (send_cmd(load_prog_mem_cmd) < 0) return -1;
	if (load_data(__data) < 0) return -1;
	if (send_cmd(begin_prog_only_cycle_cmd) < 0) return -1;
	if (send_cmd(end_prog_cmd) < 0) return -1;
	usleep(8000);
	return 0;
}

mdl_i8_t read_prog_mem(mdl_u16_t *__data) {
	if (send_cmd(read_prog_mem_cmd) < 0) return -1;
	if (read_data(__data) < 0) return -1;
	return 0;
}

mdl_i8_t load_data_mem(mdl_u16_t __data) {
	if (send_cmd(load_data_mem_cmd) < 0) return -1;
	if (load_data(__data) < 0) return -1;
	if (send_cmd(begin_prog_only_cycle_cmd) < 0) return -1;
	if (send_cmd(end_prog_cmd) < 0) return -1;
	usleep(8000);
	return 0;
}

mdl_i8_t read_data_mem(mdl_u16_t *__data) {
	if (send_cmd(read_data_mem_cmd) < 0) return -1;
	if (read_data(__data) < 0) return -1;
	return 0;
}

void print_bin(mdl_uint_t __bin, mdl_u8_t __n) {
	for (mdl_u8_t itr = 0; itr != __n*8; itr++)
		printf("%u", (__bin >> (((__n*8)-1)-itr)) & 1);
}

# define ret_err {any_err=-1;goto err;}
# define incr_itr(__a, __b) __a+=__b
# define PIC_MEM_SIZE 0x2200
mdl_u8_t has_conf_mem = 0, has_data_mem = 0;
mdl_i8_t read_ihex(mdl_u16_t *__data, mdl_uint_t *__prog_size, char *__hex_fpth) {
	int fd;
	struct stat st;

	if ((fd = open(__hex_fpth, O_RDONLY)) < 0) {
		fprintf(stderr, "failed to open file at '%s', errno: %d\n", __hex_fpth, errno);
		return -1;
	}

	mdl_i8_t any_err = 0;
	if (stat(__hex_fpth, &st) < 0) {
		close(fd);
		fprintf(stderr, "failed to stat file at '%s', errno: %d\n", __hex_fpth, errno);
		ret_err
	}

	mdl_u8_t *file = NULL;
	if (!(file = (mdl_u8_t*)malloc(st.st_size))) {
		fprintf(stderr, "failed to alloc memory, errno: %d\n", errno);
		ret_err
	}

	mdl_u8_t *itr = file;

	read(fd, file, st.st_size);

	mdl_u8_t byte_c, record_type;
	mdl_u16_t addr;
	mdl_u16_t data;

	while(itr != file+st.st_size) {
		if (*itr != ':') {
			fprintf(stderr, "invalid start point got '%c' and not ':'.\n", *itr);
			ret_err
		}

		incr_itr(itr, 1);

		if (sscanf(itr, "%2hhx", &byte_c) != 1) {
			fprintf(stderr, "failed to read byte count.\n");
			ret_err
		} else
			printf("byte count: %u\n", byte_c);

		incr_itr(itr, 2);

		if (sscanf(itr, "%4hx", &addr) != 1) {
			fprintf(stderr, "failed to read address.\n");
			ret_err
		} else
			printf("addr: %u\n", addr);

		incr_itr(itr, 4);

		if (sscanf(itr, "%2hhx", &record_type) != 1) {
			fprintf(stderr, "failed to read record type.\n");
			ret_err
		} else
			printf("record type: %u\n", record_type);

		incr_itr(itr, 2);

		mdl_u8_t *data_end = itr+(byte_c*4);
		mdl_uint_t o = 0;
		for (;itr != data_end; incr_itr(itr, sizeof(mdl_u16_t)*2), o++) {
			if (sscanf(itr, "%4hx", &data) != 1) {
				fprintf(stderr, "failed to read data\n");
				ret_err
			} else
				printf("placed %x at addr %u\n", data, addr+o);

			if (addr+o < 0x2000) {
				if (addr+o > *__prog_size)
					*__prog_size = addr+o;
			} else if (addr+o >= 0x2000 && addr+o <= 0x2007)
				has_conf_mem = 1;
			else if (addr+o >= 0x2100)
				has_data_mem = 1;

			*(__data+addr+o) = data;
		}

		if (record_type == 1) break;
		printf("\n");

		incr_itr(itr, 3);
	}

	err:
	if (file)
		free(file);

	close(fd);
	return any_err;
}

# include <math.h>
//# define __DBIN
# define __DEBUG
mdl_i8_t pic_prog_write(char *__hex_fpth) {
	mdl_i8_t any_err = 0;

	mdl_u16_t *data = NULL;
	if (!(data = (mdl_u16_t*)malloc(PIC_MEM_SIZE*sizeof(mdl_u16_t)))) {
		fprintf(stderr, "failed to alloc memory, errno: %d\n", errno);
		ret_err
	}

	memset((mdl_u8_t*)data, (mdl_u8_t)~0, PIC_MEM_SIZE*sizeof(mdl_u16_t));

	mdl_uint_t prog_size = 0;
	read_ihex(data, &prog_size, __hex_fpth);

	mdl_u16_t *itr = data;
	printf("program memory.\n");
	while(itr <= data+prog_size) {
		if (*itr != (mdl_u16_t)~0) {
			mdl_u16_t outbound = *itr;
# ifndef __DEBUG
			load_prog_mem(outbound);

			mdl_u16_t r = 0;
			read_prog_mem(&r);
			if (r != (outbound & 0x3FFF)) {
				fprintf(stderr, "sent: %x, recved: %x, mismatch.\n", outbound, r);
				ret_err
			}
# endif
		}

# ifndef __DEBUG
		send_cmd(incr_addr_cmd);
# endif
		incr_itr(itr, 1);
	}

# ifndef __DEBUG
	if (send_cmd(reset_cmd) < 0) ret_err
# endif

	printf("conf memory.\n");
	if (has_conf_mem) {
		itr = data+0x2000;
# ifndef __DEBUG
		if (send_cmd(load_conf_cmd) < 0) ret_err
		if (load_data(*itr) < 0) ret_err
# endif
		while(itr < data+0x2008) {
			if ((itr <= data+0x2003 || itr == data+0x2007) && *itr != (mdl_u16_t)~0) {
				mdl_u16_t outbound = *itr;
# ifndef __DEBUG
				if (itr == data+0x2007) {
					if (send_cmd(load_prog_mem_cmd) < 0) ret_err
					if (load_data(outbound) < 0) ret_err
					if (send_cmd(begin_erase_prog_cycle_cmd) < 0) ret_err
					if (send_cmd(end_prog_cmd) < 0) ret_err

					usleep(20000);
				} else
					load_prog_mem(outbound);

				mdl_u16_t r = 0;

				read_prog_mem(&r);
				if (r != (outbound & 0x3FFF)) {
					fprintf(stderr, "sent: %x, recved: %x, mismatch.\n", outbound, r);
					ret_err
				}
# endif
			}
# ifndef __DEBUG
			send_cmd(incr_addr_cmd);
# endif
			incr_itr(itr, 1);
		}
	}

	err:
	if (data != NULL)
		free(data);

	return any_err;
}

int main(int __argc, char const *__argv[]) {
	if (__argc < 2) {
		fprintf(stderr, "please suply hex file.\n");
		return -1;
	}

	mdl_i8_t any_err = 0;
# ifndef __DEBUG
	if ((device_fd = open("/dev/ttyACM0", O_RDWR)) < 0) {
		fprintf(stderr, "failed to open serial port.\n");
		return -1;
	}

	struct termios tty;
	memset(&tty, 0, sizeof(struct termios));

	if (tcgetattr(device_fd, &tty) < 0) {
		fprintf(stderr, "failed to get attributes for serial interface.\n");
		close(device_fd);
		return -1;
	}

	cfsetospeed(&tty, B38400);
	cfsetispeed(&tty, B38400);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~CSTOPB;

	tty.c_cc[VTIME] = 200;
	tty.c_cc[VMIN] = 1;

	if (tcsetattr(device_fd, TCSANOW, &tty) < 0) {
		fprintf(stderr, "failed to set attributes for serial interface.\n");
		close(device_fd);
		return -1;
	}

	printf("please wait for 8seconds for the programmer to setup.\n");
	usleep(8000000); // wait
# endif

# ifndef __DEBUG
	// bulk erase pic device
	if (send_cmd(bulk_erase_cmd) < 0) ret_err

	// reset config
	if (send_cmd(load_conf_cmd) < 0) ret_err;
	if (load_data(0x3FFF) < 0) ret_err;

	// reset device
	if (send_cmd(reset_cmd) < 0) ret_err;
# endif

	printf("writing hex to pic, please wait...\n");
	if (!(any_err = pic_prog_write((char*)__argv[1])))
		fprintf(stderr, "failed to upload program to pic.\n");
	else
		printf("succucfuly uploaded program to the pic.\n");

# ifndef __DEBUG
	if (send_cmd(reset_cmd) < 0) ret_err;
# endif

	err:
# ifndef __DEBUG
	close(device_fd);
# endif
	return any_err;
}
