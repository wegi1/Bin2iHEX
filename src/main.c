///////////////////////////////////////////////////////////////////////////////
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/stat.h>

//==========================================================================

uint8_t checksum;              // checksum of record
uint8_t opened_input = 0;      // is opened input file ? 1 = opened | 0 = closed
uint8_t opened_output = 0;     // is opened input file ? 1 = opened | 0 = closed
uint8_t many_bt;               // actual processed record length

size_t filesize ;              // size in bytes of input binary file to convert
size_t rest_file ;             // input filesize for calculate how many unreaded data bytes still waiting
size_t filecounter ;           // input filesize to count converted data bytes to ASCII

size_t offset;                 // offset in first parameter to own filename
size_t address = 0x00000000;   // 32 bit record address - defauld addres value = (0)
uint32_t addr_len;             // length of text addres parameter (argv[3])
uint32_t addr_strt;            // offset in (argv[3]) to start address text
uint32_t end_add;              // just for print end address of binary data
uint32_t addr_rec;             // actuall address of records processed and saving data

int arg_count;                 // argument count copy
char** argptr;                 // arguments pointers copy

const char* progname;          // a own executed filename

uint8_t input_buffer[65536];   // a buffer for input portion if binary data readed from file
uint32_t inbuf_count;          // pointer possition actually used in buffer /increasse
uint32_t inbuf_rest;           // count of a data to read in the input buffer /decreasse

uint8_t output_buffer[200000]; // output buffer for portions of iHEX data records
uint32_t outbuf_count = 0;     // actual first free possition in the output buffer
uint8_t converted_byte[2];     // representation of actually converted uint8_t byte value to ASCII
//======================================
int show_help(int data);
void check_prgname(void);
int check_params(void);
char U_Case(char code);
void get_addres_value(void);
void get_decimal_value(void);
void get_hex_value(void);
void debug_print(void);
void info_print(void);
int open_files(void);
void close_input(void);
void close_output(void);
void close_files(void);
int read_data(void);
int flush_buffer(void);
int write_records(void);
void store_colon(void);
int  store_byte(uint8_t data);
void store_nline(void);
uint8_t nybble_to_ASCII(uint8_t nybble);
uint8_t byte_to_ASCII(uint8_t data);
int make_seg04(void);
int store_eof(void);
//===========================
struct stat filestatus; // structure for check file exists and filelength st_nlink and st_size
//==================================================
FILE* infile;           // input read binary file to convert
FILE* outfile;          // output write iHEX converted data
//===================================================

//========================================================================
// main function
//
// return values:
// 0 = OK
//
// 1 = to less parameters ... to make conversion
// 2 = input binary file doesn't exists
// 3 = any other IO read ERROR at the check of input file infostructure
// 4 = Input and output filenames are this same
// 5 = open input  file failed 
// 6 = open output file failed 
// 7 = READ  binary  input file ERROR
// 8 = WRITE binary output file ERROR
//========================================================================
int main(int argc, char* argv[]) {
argptr = argv;    // pointer to argv - make copy
arg_count = argc; //  argument count - make copy

int result;
	
	check_prgname();         // set the own exe program name in parameter[0] string (argv[0])
	result = check_params(); // check parameters
	if (result != 0){ return show_help(result);} // if parameter errors occurred then exit
	get_addres_value();      // chec addres parameter - if error, the default value id 0x08000000
	end_add = address + filestatus.st_size; // end adress for statictics print
	addr_rec = address;      // store recordc address

    if(arg_count > 4) { debug_print(); } // if exists more than 4 args then print debug informations

    result = open_files();   // open input and output files
    if (result != 0) { return result ; } // if open files error then exit

	result = make_seg04();   // at the very first start of program make and write seg "0x04"
	if (result != 0) { return result ; } // if flush buffer error then exit

	result = read_data();    // first time read a portion of binary data input to buffer
	if (result != 0) { return result ; } // if read data error then exit

	result = write_records(); // main task - convert data to iHEX file
	if (result != 0) { return result ; } // if convert data error then exit


	if(arg_count < 5) { info_print(); } // if no debug print then print regular information

	// all OK, print the last information about conversion
	printf("\n%s: Converted file \"%s\" -> \"%s\" at address 0x%08X\n", progname, argptr[1] , argptr[2], address);

	// exit
	return 0;
}
//===========================================================================================

//===========================================
// set pointer to program name
//===========================================
void check_prgname(void){
	offset = 0;
	while((uint8_t)argptr[0][offset] != (uint8_t)0) { // get the full length
		offset++;
	}
	while (offset--) { // if exists full path, move pointer to filename
		if ((argptr[0][offset] == '/') || (argptr[0][offset] == '\\')) {
			break;
		}
	}
	offset++;
	progname = (&(argptr[0][offset])); // progname is a pointer to executed file name
}
//===
//==============================
// show short help information
//==============================
int show_help(int data)
{
	fprintf(stderr,
		"\r\n"
		"Usage:\n"
		"    %s infile outfile [load address] \r\n"
		"\r\n",
		progname
	);
	return data;
}
//=============================
//============================================
// try to convert passed byte to UPPER CASE
//
// return uin8_t converted byte
//============================================
char U_Case(char code){

uint8_t data;

	data = (uint8_t) code;
	if(data > 0x60){
		if(data < 0x7B){
			data = data - 0x20;
		}
	}
    return data;
}
//==
//====================================================
// if exists more than 4 parameters sended to program
// the this debug inf is printed
//=====================================================
void debug_print(void){
    // Display each command-line argument.
    printf_s( "\nCommand-line arguments:\n" );
    for(int count = 0; count < arg_count; count++ )
        printf_s( "  argv[%d]   %s\n", count, argptr[count] );
	printf("Address parameter = \"%s\"\n" , &argptr[3][addr_strt] );
	info_print();
}
//===
//====================================
// printing a few information lines
//====================================
void info_print(void){
	printf("\nA length of input file \"%s\" is: %d bytes\n", &argptr[1][0] , filestatus.st_size);
	printf("Start address of data = 0x%08X\n" , address);
	printf("..End address of data = 0x%08X\n" , end_add);
}
//===
//====================================================
// check parameters as:
// - to less parameters?
// - input  file exists?
// - input  file info read error
// - input  file name length
// - output file name length
// - comparison booth filename strings of inputfile and outputfile
//
// return 0 = OK | != 0 = ERROR
//=====================================================
int check_params(void){

int len1; // length input filename
int len2; // length output filename
int i; // iterator
int test_val; // test value of comparision two strings
uint8_t data1; // temp byte value
uint8_t data2; // temp byte value

	if(arg_count < 3) { // to less parameters !!!
		fprintf(stderr, "\n\n To less parameters ! \n\n" );
		return 1;
	}
	if(stat(&argptr[1][0], &filestatus) < 0) // check file exists and file size
	{
		if(filestatus.st_nlink == 0){ // is file exists??
			fprintf(stderr, "\n\nFile: \"%s\" doesn't exists !!!\n\n" , &argptr[1][0]);
			return 2;
		} else { // any other IO read ERROR
			fprintf(stderr, "\nError of checking info file: %s !!!\n"  , &argptr[1][0]);
			return 3;
		}
	}
	filesize = filestatus.st_size; // get the file size
	rest_file = filesize; // for counting the rest of unreaded data from input file
	filecounter = filesize; // for counting all processed input data to ASCII and EOF check

	len1 = 0;
	while((uint8_t)argptr[1][len1] != (uint8_t)0) { // take the length of input filename
		len1++;
	}
	len2 = 0;
	while((uint8_t)argptr[2][len2] != (uint8_t)0) { // take the length of output filename
		len2++;
	}	
	test_val = 1; // filenames are not this same

	if(len1 == len2){
		test_val = 0;
		for(i = 0; i < len1 ; i++){
			data1 = (uint8_t) U_Case(argptr[1][i]);
			data2 = (uint8_t) U_Case(argptr[2][i]);

			if( data1 != data2 ){ 
				test_val = 1; // filenames are not this same
				break; // break "for" loop
			}
		}
	}
	if(test_val == 0) { // are filenames this same?
		fprintf(stderr, "\nInput and output filenames are this same !!!\n");
		return 4;
	}
	return 0;
}
//===
//=====================================================
// try to get the decimal value from parameter
// as address and store addres to variable
//=====================================================
void get_decimal_value(void){
	
uint32_t start_string, end_string, i, new_add;
uint8_t data;

	start_string = addr_strt; // offset in address string (usually 0)
	end_string = start_string + addr_len; // length string of address param
	new_add = 0; // new address initial value

	for(i = start_string; i < end_string ; i++){
		data = (uint8_t) U_Case(argptr[3][i]);
		if(data == 0) { break; }

		if((data > 0x2F) && (data < 0x3A)) { // ignore no digit value - isn't digit?
			data = data &0x0f; // take next digit
			new_add = new_add * 10 ; // and calculate address
			new_add = new_add + data;
		}
	}
	address = new_add; // store new addres value
}
//===
//=====================================================
// try to get data as hex input string of value
//=====================================================
void get_hex_value(void){

uint32_t start_string, end_string, i, new_add;
uint8_t data;

	start_string = addr_strt + 2;// offset in address string (usually 0) + 2 ommited "0x"
	end_string = start_string + addr_len -2 ; // end of hex address text string
	new_add = 0; // initial value of new address

	if(addr_len > 10) { end_string = start_string + 10 -2 ; } // if string is to long then trim it
	for(i = start_string; i < end_string ; i++){ // take the data to convert from string
		data = (uint8_t) U_Case(argptr[3][i]); // upper case
		if((data > 0x2F) && (data < 0x3A)){ // is a digit value?
			data = data &0x0f;
		} else {
			data = data &0x0f; // it's not digit value, change to A..F (10..15) value
			data = data +9;
		}
		new_add = new_add << 4; // next nyblle insight comming
		new_add = new_add + data; 
	}
	address = new_add; // store new addres value
}
//===
//=====================================================
// try to check address value parameter and convert
// it to address variable value trying to check and
// get value as hexadecimal or decimal fro ASCII
//=====================================================
void get_addres_value(void){

uint32_t chk_dec = 0;
uint32_t temp1, temp2, temp3, i;

	if(arg_count > 3) { // exist address parameter?
		temp1 = 0;
		while((uint8_t)argptr[3][temp1] != (uint8_t)0) { // check length of address parameter
			temp1++;
		}
		temp2 = 0;
		while((uint8_t)argptr[3][temp2] == (uint8_t)0x20) { // delete leading spaces
			temp2++;
		}
		addr_len  = temp1 - temp2; // length of address parameter
		addr_strt = temp2;         // offset in address string

		if(addr_len > 2) { // check "0x" string for hex value
			if((uint8_t) argptr[3][addr_strt] == 0x30 ) {
				if((U_Case(argptr[3][addr_strt+1])) == 0x58) {
					chk_dec++;
					get_hex_value();
				}
			}
		}
		if(chk_dec == 0) { get_decimal_value(); }
	} // if(arg_count > 3) { // exist address parameter?
}
//===

//==============================================================
// try to open input and output files
//
// return value 0 = OK | != 0 = ERROR
//==============================================================
int open_files(void)
{
	fopen_s(&infile , argptr[1], "rb"); // open input binary file to read
	if (infile == NULL) { // occurred error in open file?
		fprintf(stderr , "\n%s: open input file \"%s\" failed !!!\n\n", progname, argptr[1]);
		return 5;
	}
	opened_input =1; // mark the input file is opened

	fopen_s(&outfile , argptr[2], "wb"); // open/recreate otput file for write bytes
	if (outfile == NULL) { // I/O ERROR occurred?
		fprintf(stderr, "\n%s: open output file \"%s\" failed !!!\n\n", progname, argptr[2]);
		close_input();
		return 6;
	}
	opened_output = 1; // mark the output file is opened

	return 0;
}
//=============================
// close input and output file
// if needed
//=============================
void close_files(void) {
	close_input();
	close_output();
}
//===
//=============================
// close input file if needed
//=============================
void close_input(void){
	if(opened_input == 1) {
		fclose(infile);
		opened_input = 0;
	}
}
//===
//=============================
// close output file if needed
//=============================
void close_output(void) {
	if(opened_output == 1) {
		fclose(outfile);
		opened_output = 0;
	}
}
//=============================

//==============================================================
// try to read portion of input data to input buffer in 65536
// bytes sequences or less if data to read are shortly
// if readed last data portion (EOF occurred) 
// input file will be closed
//
// return 0 = OK | != 0 = ERROR
//===============================================================
int read_data(void) {

	size_t length_data_bytes;
	size_t elements_read;

	if(rest_file < 65536) { 
		length_data_bytes = rest_file; // data to read
		rest_file = 0; // actualize rest data value
	} else {
		length_data_bytes = 65536; // data to read
		rest_file = rest_file - 65536; // actualize rest data value
	}
	inbuf_count = 0; // pointer input buffer possition = start (0)
	inbuf_rest = 0;  // set temporary count of a data to read in the input buffer to (0)

	if(opened_input == 1) { // file is still opened
		elements_read =  fread(&input_buffer, length_data_bytes, 1, infile); // read data into a buffer
	}
	   
	if(elements_read == 0) { // read ERROR occurred?
		fprintf(stderr , "\n\n%s: READ binary input file \"%s\" ERROR !!!\n\n", progname, argptr[1]);
		close_files();
		return 7 ;
	}
	if(opened_input == 1) { inbuf_rest = length_data_bytes; } // set the count of a data to read in the input buffer 
	if(rest_file == 0) { close_input();} // close after EOF and readed data into input buffer

	// all OK
	return 0;
}
//===
//==============================================================
// try to write iHEX data from output buffer
// to output iHEX file (argv[2])
//
// add return status value
// 0 = OK | != 0 = WRITE ERROR
// if WRITE ERRORS occurred then close opened files
//===============================================================
int flush_buffer(void) {
size_t bytes_to_write = outbuf_count; // bytes to write used bytes in iHEX buffer
size_t elements_written = fwrite(&output_buffer, bytes_to_write, 1, outfile); // write data
	if(elements_written == 0) { // exists WRITE ERROR?
		fprintf(stderr , "\n\n%s: WRITE binary output file \"%s\" ERROR !!!\n\n", progname, argptr[2]);
		close_files(); // close opened files
		return 8 ; // ERRROR
	}
	outbuf_count = 0; // rewind used buffer pointer possition
	return 0; // all OK
}
//===
//==============================================================
// at the start write of data
// and at the every one overflow of low 16 bits records address
// make and save extend address segment "0x04"
// and flushing iHEX buffer to file
//==============================================================
int make_seg04(void) {
uint8_t data;
	store_colon(); // start record marker
	checksum = 0x00;
	checksum = checksum + byte_to_ASCII((uint8_t) 2); // 2 data bytes = highest 16 bits of record addres
	byte_to_ASCII(0); // store 2 times "0" value
	byte_to_ASCII(0);
	checksum = checksum + byte_to_ASCII((uint8_t) 4); // type of record (0x04)
	data = addr_rec >> 24 ;
	checksum = checksum + byte_to_ASCII(data); // high 8 bits from high 16bits of 32bits records addres
	data = addr_rec >> 16 ;
	checksum = checksum + byte_to_ASCII(data); // low  8 bits from high 16bits of 32bits records addres

	// checksum = NEG checksum
	checksum ^= 0xFF; // final calc checksum
	checksum ++;
	//---
	byte_to_ASCII(checksum); // store ASCII checksum
	store_nline(); // new line marker
	return flush_buffer(); // save to file all used iHEX data from output buffer
}
//===
//=========================================
// convert low nybble passed byte
// to ASCII value
//
// return value is a ASCII representation
// of converted nybble
//=========================================
uint8_t nybble_to_ASCII(uint8_t nybble){
uint8_t data;
	data = nybble &0x0f;
	data = data + 0x30;  // first try to check is digits?
	if(data > 0x39) { data = data + 7 ; } // if no - convert to A..F ASCII letter
	return data;
}
//=====================================================
// a simple conversion byte to ASCII txt
// input uint8_t byte to convert
// output = input byte for calc checksum
//
// store converted byte to outbuff
// and to the 2 bytes table 
// called uint8_t converted_byte[2] 
//=====================================================
uint8_t byte_to_ASCII(uint8_t data) {

uint8_t data2 = data;

	data2 = data2 >> 4; // high 8 bits
	converted_byte[0] = nybble_to_ASCII(data2); // convert high nybble to ASCII
	store_byte(converted_byte[0]); // store ASCII nybble value to iHEX buffer
	converted_byte[1] = nybble_to_ASCII(data); // convert low  nybble to ASCII
	store_byte(converted_byte[1]); // store ASCII nybble value to iHEX buffer
	return data; // return passing arg to calculate checksum possibility
}
//===
//==============================================================
// store new line sign (0X0A)
// at the end of every one record
// this data doesn't use to calculate checksum record
//===============================================================
void store_nline(void) {
		store_byte(0x0A); // store new line marker at the end of record
}
//===
//==========================================
// store one byte of colon (0x3A) value
// into output iHEX buffer
//==========================================
void store_colon(void) {
		store_byte(0x3A); // start of everu one record
}
//===
//===========================================
// store passed uint8_t byte to otput buff
// and return passed byte to checksum calc
//===========================================
int store_byte(uint8_t data) {
		output_buffer[outbuf_count] = data; // stor one byte to output buffer
		outbuf_count++; // increasse output buffer pointer possition
		return data;
}
//================================================================
//================================================
//      ***********************************
//      ********* MAIN PROCEDURE **********
//      ***********************************
// write iHEX data records from input buffer
// loaded portion of binary file
// to output buffer and store the out buffer
// to output file..
// until reached EOF input binary data file
//
// return value 0 = OK | != 0 = ERROR
//================================================
int write_records(void) {
	uint8_t chk_rec , i, tmpr;
	int result;
	uint16_t low_ad;
	size_t tst01;
	while((opened_input + opened_output) != 0) { // repeat untill booth files are not closed
		store_colon(); // start write record
		checksum = 0;  // set checksum start value
		many_bt = 16;  // first attemp default value of record size assume to 16 bytes
		chk_rec = addr_rec & 0x0f;  // check record address modulo 16
		if(chk_rec != 0) {          // just tell me chk_rec mod 16 = 0?
			many_bt = 16 - chk_rec; // if is not mod 16 then record len = fill up to mod 16
		}
		tst01 = many_bt; // tst01 = many_bt just for comparision 32bits value
		if(tst01 > filecounter) { // if the rest of file bytes is less than assumed record length
			many_bt = filecounter & 0x0f; // then record length is equal the rest of file bytes
		} // uff... we got the properly record length value
		checksum = checksum + byte_to_ASCII(many_bt); // store in to the record length of data and actualize checksum
		low_ad = addr_rec &0xffff;   // now get the lowest 16 bits record address
		tmpr = low_ad >> 8;  // now shift to send highest 8 bits of record address
		checksum = checksum + byte_to_ASCII(tmpr); // store highest 8 bits of record address and actualize checksum
		tmpr = low_ad &0xff; // now take the lowest 8 bits of record addres
		checksum = checksum + byte_to_ASCII(tmpr);	// and store lowest 8 bits of record address and actualize checksum
		tmpr = 0; // record type value - now means "DATA RECORD"
		byte_to_ASCII(tmpr); // data type record = 0 => what mean "DATA RECORD" in "0" case don't need actualize checksum
		for(i = 0; i< many_bt ; i++) { // take the assumed record length bytes to convert
			if(inbuf_rest == 0) { // if data in buffer are finished 
				result = read_data(); // then get the new portion of data if exists
				if (result != 0) { return result ; } // if READ DATA ERROR exist then return
			}
			tmpr = input_buffer[inbuf_count]; // HERE IS TAKE A DATA FROM INPUT BUFFER
			inbuf_count++; // increasse pointer possition to data in buffer 
			inbuf_rest--;  // decreasse/count value of data bytes taked from buffer
			filecounter--; // count the all bytes taked from binary file as well
			checksum = checksum + byte_to_ASCII(tmpr);	// store the DATA BYTE  and actualize checksum
		}
		//--- checksum = NEG checksum ---
		checksum ^= 0xFF; // calculate checksum
		checksum ++;      // checksum = NEG checksum
		//---
		byte_to_ASCII(checksum); // store checksum converted to ASCII to record
		store_nline(); // store newline value to end of one record (0x0A)
		addr_rec = addr_rec + many_bt; // actualize record address
		low_ad = addr_rec &0xffff;  // check the overflow in 16 bits record address
		if(low_ad == 0) { // exists 16 bit address overflow?
			result = make_seg04(); // yes - then save record "04" and flush output buffer to output file
			if (result != 0) { return result; }
		}

		if(filecounter == 0) { return store_eof(); } // last data was send to buffer, store the "EOF" record and go out from procedure
		
	} // while((opened_input + opened_output) != 0)
	return 0; // never reached
}
//==============================================================

//================================================
// store the "EOF" iHEX record procedure
// to output buffer, flushing buffer to
// output file and close opened files
// return result 0 = OK, != 0 -> ERROR
//=================================================
int store_eof(void) {
int result;
	store_colon(); // eof record = ":", 0,0,0,1,0xFF,"/n"

	byte_to_ASCII(0);
	byte_to_ASCII(0);
	byte_to_ASCII(0);
	byte_to_ASCII(1);
	byte_to_ASCII(255);

	store_nline(); // save new line character "(0x0A)"
	result = flush_buffer(); // store all unsaved data from output buffer to otput iHEX file
	close_files();
	return result;
}
//================================================
