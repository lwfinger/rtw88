/* Program to convert header into a binary file
 * Coded by Larry Finger
 * August 2015
 *
 * There is no Makefile, build with:
 *	gcc -o convert_firmware convert_firmware.c
 */

#include <stdio.h>
#define u8 char
#define u32 int

/* Get the firmware data that has been copied from a vendor driver */
#include "convert_firmware.h"

void output_bin(FILE *outb, const u8 *array, int len)
{
	int i;
	for (i = 0; i < len; i++)
		fwrite(&array[i], 1, 1, outb);
}

int main(int argc, char **argv)
{
	FILE *outb;
	int i;

	/* convert firmware */
	outb = fopen("rtw8821c_fw.bin", "w");
	if (!outb) {
		fprintf(stderr, "File open error\n");
		return 1;
	}
	output_bin(outb, array_mp_8821c_fw_nic,
		   array_length_mp_8821c_fw_nic);
	fclose(outb);

	return 0;
}


