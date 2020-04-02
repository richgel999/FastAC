// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Simple file compression using arithmetic coding                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.00  -  April 25, 2004                                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdlib.h>

#include "arithmetic_codec.h"


// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char * W_MSG = "cannot write to file";
const char * R_MSG = "cannot read from file";

const unsigned NumModels  = 16;                          // MUST be power of 2

const unsigned FILE_ID    = 0xB8AA3B29U;

const unsigned BufferSize = 65536;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Prototypes  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_File(char * data_file_name,
                 char * code_file_name);

void Decode_File(char * code_file_name,
                 char * data_file_name);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Main function - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(int numb_arg, char * arg[])
{
                                                       // define program usage
  if ((numb_arg != 4) || (arg[1][0] != '-') || 
     ((arg[1][1] != 'c') && (arg[1][1] != 'd'))) {
    puts("\n\t Compression parameters:   acfile -c data_file compressed_file");
    puts("\n\t Decompression parameters: acfile -d compressed_file new_file\n");
    exit(0);
  }

  if (arg[1][1] == 'c')
    Encode_File(arg[2], arg[3]);
  else
    Decode_File(arg[2], arg[3]);

  return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Implementations - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Error(const char * s)
{
  fprintf(stderr, "\n Error: %s.\n\n", s);
  exit(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Buffer_CRC(unsigned bytes,
                    unsigned char * buffer)
{
  static const unsigned CRC_Gen[8] = {        // data for generating CRC table
    0xEC1A5A3EU, 0x5975F5D7U, 0xB2EBEBAEU, 0xE49696F7U,
    0x486C6C45U, 0x90D8D88AU, 0xA0F0F0BFU, 0xC0A0A0D5U };

  static unsigned CRC_Table[256];            // table for fast CRC computation

  if (CRC_Table[1] == 0)                                      // compute table
    for (unsigned k = CRC_Table[0] = 0; k < 8; k++) {
      unsigned s = 1 << k, g = CRC_Gen[k];
      for (unsigned n = 0; n < s; n++) CRC_Table[n+s] = CRC_Table[n] ^ g;
    }

                                  // computes buffer's cyclic redundancy check
  unsigned crc = 0;
  if (bytes)
    do {
      crc = (crc >> 8) ^ CRC_Table[(crc&0xFFU)^unsigned(*buffer++)];
    } while (--bytes);
  return crc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Input_File(char * file_name)
{
  FILE * new_file = fopen(file_name, "rb");
  if (new_file == 0) Error("cannot open input file");
  return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Output_File(char * file_name)
{
  FILE * new_file = fopen(file_name, "rb");
  if (new_file != 0) {
    fclose(new_file);
    printf("\n Overwrite file %s? (y = yes, otherwise quit) ", file_name);
    char line[128];
    gets(line);
    if (line[0] != 'y') exit(0);
  }
  new_file = fopen(file_name, "wb");
  if (new_file == 0) Error("cannot open output file");
  return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Save_Number(unsigned n, unsigned char * b)
{                                                   // decompose 4-byte number
  b[0] = (unsigned char)( n        & 0xFFU);
  b[1] = (unsigned char)((n >>  8) & 0xFFU);
  b[2] = (unsigned char)((n >> 16) & 0xFFU);
  b[3] = (unsigned char)( n >> 24         );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Recover_Number(unsigned char * b)
{                                                    // recover 4-byte integer
  return unsigned(b[0]) + (unsigned(b[1]) << 8) + 
        (unsigned(b[2]) << 16) + (unsigned(b[3]) << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_File(char * data_file_name,
                 char * code_file_name)
{
                                                                 // open files
  FILE * data_file = Open_Input_File(data_file_name);
  FILE * code_file = Open_Output_File(code_file_name);

                                                  // buffer for data file data
  unsigned char * data = new unsigned char[BufferSize];

  unsigned nb, bytes = 0, crc = 0;       // compute CRC (cyclic check) of file
  do {
    nb = fread(data, 1, BufferSize, data_file);
    bytes += nb;
    crc ^= Buffer_CRC(nb, data);
  } while (nb == BufferSize);

                                                      // define 12-byte header
  unsigned char header[12];
  Save_Number(FILE_ID, header);
  Save_Number(crc,      header + 4);
  Save_Number(bytes,    header + 8);
  if (fwrite(header, 1, 12, code_file) != 12) Error(W_MSG);
                                                            // set data models
  Adaptive_Data_Model dm[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(256);

  Arithmetic_Codec encoder(BufferSize);                  // set encoder buffer

  rewind(data_file);                               // second pass to code file

  unsigned context = 0;
  do {

    nb = (bytes < BufferSize ? bytes : BufferSize);
    if (fread(data, 1, nb, data_file) != nb) Error(R_MSG);   // read file data

    encoder.start_encoder();
    for (unsigned p = 0; p < nb; p++) {                       // compress data
      encoder.encode(data[p], dm[context]);
      context = unsigned(data[p]) & (NumModels - 1);
    }

    encoder.write_to_file(code_file);  // stop encoder & write compressed data

  } while (bytes -= nb);

                                                          // done: close files
  fflush(code_file);
  unsigned data_bytes = ftell(data_file), code_bytes = ftell(code_file);
  printf(" Compressed file size = %d bytes (%6.2f:1 compression)\n",
    code_bytes, double(data_bytes) / double(code_bytes));
  fclose(data_file);
  fclose(code_file);

  delete [] data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_File(char * code_file_name,
                 char * data_file_name)
{
                                                                 // open files
  FILE * code_file = Open_Input_File(code_file_name);
  FILE * data_file  = Open_Output_File(data_file_name);

                                   // read file information from 12-byte header
  unsigned char header[12];
  if (fread(header, 1, 12, code_file) != 12) Error(R_MSG);
  unsigned fid   = Recover_Number(header);
  unsigned crc   = Recover_Number(header + 4);
  unsigned bytes = Recover_Number(header + 8);

  if (fid != FILE_ID) Error("invalid compressed file");

                                                  // buffer for data file data
  unsigned char * data = new unsigned char[BufferSize];
                                                            // set data models
  Adaptive_Data_Model dm[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(256);

  Arithmetic_Codec decoder(BufferSize);                  // set encoder buffer

  unsigned nb, new_crc = 0, context = 0;                    // decompress file
  do {

    decoder.read_from_file(code_file); // read compressed data & start decoder

    nb = (bytes < BufferSize ? bytes : BufferSize);
                                                            // decompress data
    for (unsigned p = 0; p < nb; p++) {
      data[p] = (unsigned char) decoder.decode(dm[context]);
      context = unsigned(data[p]) & (NumModels - 1);
    }
    decoder.stop_decoder();

    new_crc ^= Buffer_CRC(nb, data);                // compute CRC of new file
    if (fwrite(data, 1, nb, data_file) != nb) Error(W_MSG);

  } while (bytes -= nb);


  fclose(data_file);                                     // done: close files
  fclose(code_file);

  delete [] data;
                                                   // check if file is correct
  if (crc != new_crc) Error("incorrect file CRC");
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
