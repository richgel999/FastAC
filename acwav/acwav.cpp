// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Simple audio compression using arithmetic coding                          -
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

#include <string.h>
#include <stdlib.h>

#include "arithmetic_codec.h"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Prototypes  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_WAV_File(char * data_file_name,
                     char * code_file_name);

void Decode_WAV_File(char * code_file_name,
                     char * data_file_name);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Main function - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(int numb_arg, char * arg[])
{
                                                       // define program usage
  if ((numb_arg != 4) || (arg[1][0] != '-') || 
     ((arg[1][1] != 'c') && (arg[1][1] != 'd'))) {
    puts("\n\t Compression parameters:   acwav -c wav_file compressed_file");
    puts("\n\t Decompression parameters: acwav -d compressed_file new_wav_file\n");
    exit(0);
  }

  if (arg[1][1] == 'c')
    Encode_WAV_File(arg[2], arg[3]);
  else
    Decode_WAV_File(arg[2], arg[3]);

  return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Constansts  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char * W_MSG = "cannot write to file";

const char * R_MSG = "cannot read from file";

const unsigned NumModels  = 40;

const unsigned BufferSize = 65536;

const unsigned WAV_ID     = 0x46464952U;

const unsigned ACW_ID     = 0xF3C2047BU;

const unsigned char WAVE_HEADER[44] = {
  0x52, 0x49, 0x46, 0x46, 0x7F, 0x7F, 0x7F, 0x7F, 0x57, 0x41, 0x56,
  0x45, 0x66, 0x6D, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x02, 0x00, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x04,
  0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x7F, 0x7F, 0x7F, 0x7F };


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Implementations - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Error(const char * s)
{
  fprintf(stderr, "\n Error: %s.\n\n", s);
  exit(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Audio_Samples(const unsigned char header[44])
{
  return (unsigned(header[40]) >>  2) + (unsigned(header[41]) <<  6) +
         (unsigned(header[42]) << 14) + (unsigned(header[43]) << 22);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Save_Number(unsigned n, unsigned char * b)
{
  b[0] = (unsigned char)( n        & 0xFFU);
  b[1] = (unsigned char)((n >>  8) & 0xFFU);
  b[2] = (unsigned char)((n >> 16) & 0xFFU);
  b[3] = (unsigned char)( n >> 24         );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Recover_Number(unsigned char * b)
{
  return unsigned(b[0]) + (unsigned(b[1]) << 8) + 
        (unsigned(b[2]) << 16) + (unsigned(b[3]) << 24);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Split_Integer(int n, unsigned & bits, unsigned & data)
{
  if (n == 0) {
    bits = data = 0;
    return;
  }

  static unsigned group_table[4096];

  if (group_table[1] == 0)
    for (unsigned g = 0; g <= 12; g++) {
      unsigned k = 1U << g, r = k >> 1;
      while (k > r) group_table[--k] = g;
    }

  unsigned a = unsigned(n < 0 ? -n : n);
  bits = (a < 4096 ? group_table[a] : 13 + group_table[(a>>13)&0xFFF]);
  data = a + a - (1U << bits) + (n < 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int Restore_Integer(unsigned bits, unsigned data)
{
  int v = (data + (1 << bits)) >> 1;
  return (data & 1 ? -v : v);
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

void SpP_Analysis(unsigned n, int * c, int * l, int * h)
{
                      // computation of the forward (reversible) S+P transform
  int d0, d1, d2;
  unsigned i, m = (n >> 1) - 1;

  for (i = 0; i <= m; i++, c += 2) {
    l[i] = (c[0] + c[1]) >> 1;
    h[i] =  c[0] - c[1];
  }
  if (n & 1) l[i] = c[0];

  h[0] -= (d1 = l[0] - l[1]) >> 2;
  d2 = l[1] - l[2];
  h[1] -= (((d1 + d2 - h[2]) << 1) + d2 + 3) >> 3;

  for (i = 2; i < m; i++) {
    d0 = d1;  d1 = d2;  d2 = l[i] - l[i+1];
    h[i] -= (8 * d2 + 4 * d1 - d0 - 6 * h[i+1] + 7) >> 4;
  }

  h[m] -= d2 >> 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SpP_Synthesis(unsigned n, int * l, int * h, int * c)
{
                                   // computation of the inverse S+P transform
  int d0, d1, d2;
  unsigned i, m = (n >> 1) - 1;

  int t = (h[m] += (d1 = l[m-1] - l[m]) >> 2);
  d0 = l[m-2] - l[m-1];

  for (i = m - 1; i > 1; i--) {
    d2 = d1;  d1 = d0;  d0 = l[i-2] - l[i-1];
    t = (h[i] += (8 * d2 + 4 * d1 - d0 - 6 * t + 7) >> 4);
  }
  h[1] += (((d0 + d1 - h[2]) << 1) + d1 + 3) >> 3;
  h[0] += d0 >> 2;

  for (i = 0; i <= m; i++) {
    t = l[i] - (h[i] >> 1);
    *c++ = t + h[i];
    *c++ = t;
  }
  if (n & 1) *c = l[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SpP_Analysis(unsigned ed,
                  int transform[],
                  int buffer[])
{
                                                       // dyadic S+P transform
  for (int lv = 0; lv <= 5; lv++) {
    int fd = ed >> lv, hd = fd >> 1;
    memcpy(buffer, transform, fd * sizeof(int));
    SpP_Analysis(fd, buffer, transform, transform + hd);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void SpP_Synthesis(unsigned ed,
                   int transform[],
                   int buffer[])
{
                                               // dyadic S+P inverse transform
  for (int lv = 5; lv >= 0; lv--) {
    int fd = ed >> lv, hd = fd >> 1;
    memcpy(buffer, transform, fd * sizeof(int));
    SpP_Synthesis(fd, buffer, buffer + hd, transform);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Separate_Channels(unsigned od,
                       unsigned ed,
                       short * data,
                       int left[],
                       int right[])
{
  unsigned k;                  // separate interleaved left and right channels
  for (k = 0; k < od; k++) {
    left[k]  = int(*data++);
    right[k] = int(*data++);
  }
  for (k = od; k < ed; k++) {
    left[k]  = (7 * left[k-1]) >> 3;
    right[k] = (7 * right[k-1]) >> 3;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Interleave_Channels(unsigned od,
                         int left[],
                         int right[],
                         short * data)
{
  for (unsigned k = 0; k < od; k++) {
    *data++ = short(left[k]);
    *data++ = short(right[k]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_SpP(unsigned dim,
                int transform[],
                Arithmetic_Codec & encoder,
                Adaptive_Data_Model data_model[])
{
  float ctx = 0;
  for (int k = dim - 1; k >= 0; k--) {
    int nm = int(ctx);            // context = weighted average of past values
    unsigned bits, data;
    Split_Integer(transform[k], bits, data);
    encoder.encode(bits, data_model[nm]);               // encode with context
    if (bits)                                              // write "raw" bits
      if (bits == 1)
        encoder.put_bit(data);
      else
        encoder.put_bits(data, bits);
    ctx = 0.9F * ctx + 0.2F * bits;                     // update soft context
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_SpP(unsigned dim,
                Arithmetic_Codec & decoder,
                Adaptive_Data_Model data_model[],
                int transform[])
{
  float ctx = 0;
  for (int k = dim - 1; k >= 0; k--) {
    int nm = int(ctx);            // context = weighted average of past values
    unsigned bits = decoder.decode(data_model[nm]);     // decode with context
    if (bits == 0)
      transform[k] = 0;
    else                                                    // read "raw" bits
      if (bits == 1)
        transform[k] = (decoder.get_bit() ? -1 : 1);
      else
        transform[k] = Restore_Integer(bits, decoder.get_bits(bits));
    ctx = 0.9F * ctx + 0.2F * bits;                     // update soft context
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Input_File(unsigned file_id,
                       char * file_name,
                       unsigned char header[44])
{
  FILE * new_file = fopen(file_name, "rb");
  if (new_file == 0) Error("cannot open input file");

  if (fread(header, 1, 44, new_file) != 44) Error(R_MSG);

  if (Recover_Number(header) != file_id)
    Error("invalid input file");

  for (unsigned n = 4; n < 44; n++)
    if ((WAVE_HEADER[n] != 0x7F) && (WAVE_HEADER[n] != header[n]))
      Error("unsupported audio file");

  return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Output_File(unsigned file_id,
                        char * file_name,
                        unsigned char header[44])
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

  Save_Number(file_id, header);
  if (fwrite(header, 1, 44, new_file) != 44) Error(W_MSG);

  return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_WAV_File(char * data_file_name,
                     char * code_file_name)
{
                                                                 // open files
  unsigned char header[44];
  FILE * data_file = Open_Input_File(WAV_ID, data_file_name, header);

  unsigned file_samples = Audio_Samples(header);
  if ((file_samples < 64) || (file_samples >= 0x10000000U))
    Error("invalid WAV file");

  FILE * code_file = Open_Output_File(ACW_ID, code_file_name, header);

                                                      // memory for audio data
  int * data = new int[3*BufferSize];
  int * left_channel   = data + BufferSize;
  int * right_channel  = data + BufferSize * 2;

  Arithmetic_Codec encoder(5 * BufferSize);

  Adaptive_Data_Model * dm = new Adaptive_Data_Model[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(24);

  unsigned crc = 0;
  do {

    unsigned ns = (file_samples < BufferSize ? file_samples : BufferSize);
    unsigned es = (ns + 63) & 0xFFFFFFC0U;              // next multiple of 64
    file_samples -= ns;
    if (fread(data, 4, ns, data_file) != ns) Error(R_MSG);       // read audio

    crc ^= Buffer_CRC(4 * ns, (unsigned char *) data);          // compute CRC
    Separate_Channels(ns, es, (short*) data, left_channel, right_channel);

    SpP_Analysis(es, left_channel, data);                // compute transforms
    SpP_Analysis(es, right_channel, data);

    encoder.start_encoder();                            // prepare to compress

    Encode_SpP(es, left_channel, encoder, dm);          // compress transforms
    Encode_SpP(es, right_channel, encoder, dm);

    encoder.write_to_file(code_file);        // stop and write compressed data

  } while (file_samples);
                                                              // save file CRC
  Save_Number(crc, header);
  if (fwrite(header, 1, 4, code_file) != 4) Error(W_MSG);

                                                          // done: close files
  fflush(code_file);
  unsigned data_bytes = ftell(data_file), code_bytes = ftell(code_file);
  printf(" Compressed file size = %d bytes (%5.2f:1 compression)\n",
    code_bytes, double(data_bytes) / double(code_bytes));
  fclose(data_file);
  fclose(code_file);

  delete [] data;
  delete [] dm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_WAV_File(char * code_file_name,
                     char * data_file_name)
{
                                                                 // open files
  unsigned char header[44];
  FILE * code_file = Open_Input_File(ACW_ID, code_file_name, header);
  FILE * data_file = Open_Output_File(WAV_ID, data_file_name, header);
  unsigned file_samples = Audio_Samples(header);

                                                      // memory for audio data
  int * data = new int[3*BufferSize];
  int * left_channel   = data + BufferSize;
  int * right_channel  = data + BufferSize * 2;

  Arithmetic_Codec decoder(5 * BufferSize);

  Adaptive_Data_Model * dm = new Adaptive_Data_Model[NumModels];
  for (unsigned m = 0; m < NumModels; m++) dm[m].set_alphabet(24);

  unsigned crc = 0;
  do {

    unsigned ns = (file_samples < BufferSize ? file_samples : BufferSize);
    unsigned es = (ns + 63) & 0xFFFFFFC0U;              // next multiple of 64
    file_samples -= ns;

    decoder.read_from_file(code_file);  // read compressed data, start decoder

    Decode_SpP(es, decoder, dm, left_channel);        // decompress transforms
    Decode_SpP(es, decoder, dm, right_channel);

    decoder.stop_decoder();                                    // stop decoder

    SpP_Synthesis(es, left_channel, data);               // compute transforms
    SpP_Synthesis(es, right_channel, data);

    Interleave_Channels(ns, left_channel, right_channel, (short*) data);

    crc ^= Buffer_CRC(4 * ns, (unsigned char *) data);          // compute CRC

    if (fwrite(data, 4, ns, data_file) != ns) Error(W_MSG);      // read audio

  } while (file_samples);

                                                   // check if file is correct
  if (fread(header, 1, 4, code_file) != 4) Error(R_MSG);
  if (crc != Recover_Number(header)) Error("incorrect file CRC");

                                                          // done: close files
  fclose(data_file);
  fclose(code_file);

  delete [] data;
  delete [] dm;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
