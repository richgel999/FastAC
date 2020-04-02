// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Functions to test and benchmark the arithmetic coding implementations     -
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
//                                                                           -
// Analysis of coding complexity done with a similar program is available at -
//                                                                           -
// A. Said, Comparative Analysis of Arithmetic Coding Comput. Complexity     -
// HP Labs report HPL-2004-75  -  http://www.hpl.hp.com/techreports/         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <math.h>
#include <stdlib.h>

#include "test_support.h"
#include "arithmetic_codec.h"


// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const unsigned SimulTests = 1000000;


// - - Definitions - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct Test_Result
{
  unsigned alphabet_symbols;
  double   encoder_time, decoder_time;
  double   entropy, bits_used, test_symbols;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Implementations for testing encoder/decoder - - - - - - - - - - - - - -

unsigned Encode_Bit_Buffer(unsigned char bit_buffer[],
                           Static_Bit_Model & model,
                           Arithmetic_Codec & encoder)
{
  encoder.start_encoder();
  for (unsigned k = 0; k < SimulTests; k++)
    encoder.encode(unsigned(bit_buffer[k]), model);
  return 8 * encoder.stop_encoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_Bit_Buffer(unsigned char bit_buffer[],
                       Static_Bit_Model & model,
                       Arithmetic_Codec & decoder)
{
  decoder.start_decoder();
  for (unsigned k = 0; k < SimulTests; k++)
    bit_buffer[k] = unsigned char(decoder.decode(model));
  decoder.stop_decoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Encode_Bit_Buffer(unsigned char bit_buffer[],
                           Adaptive_Bit_Model & model,
                           Arithmetic_Codec & encoder)
{
  encoder.start_encoder();
  for (unsigned k = 0; k < SimulTests; k++)
    encoder.encode(unsigned(bit_buffer[k]), model);
  return 8 * encoder.stop_encoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_Bit_Buffer(unsigned char bit_buffer[],
                       Adaptive_Bit_Model & model,
                       Arithmetic_Codec & decoder)
{
  decoder.start_decoder();
  for (unsigned k = 0; k < SimulTests; k++)
    bit_buffer[k] = unsigned char(decoder.decode(model));
  decoder.stop_decoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Encode_Data_Buffer(unsigned short data_buffer[],
                            Static_Data_Model & model,
                            Arithmetic_Codec & encoder)
{
  encoder.start_encoder();
  for (unsigned k = 0; k < SimulTests; k++)
    encoder.encode(unsigned(data_buffer[k]), model);
  return 8 * encoder.stop_encoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_Data_Buffer(unsigned short data_buffer[],
                        Static_Data_Model & model,
                        Arithmetic_Codec & decoder)
{
  decoder.start_decoder();
  for (unsigned k = 0; k < SimulTests; k++)
    data_buffer[k] = unsigned short(decoder.decode(model));
  decoder.stop_decoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Encode_Data_Buffer(unsigned short data_buffer[],
                            Adaptive_Data_Model & model,
                            Arithmetic_Codec & encoder)
{
  encoder.start_encoder();
  for (unsigned k = 0; k < SimulTests; k++)
    encoder.encode(unsigned(data_buffer[k]), model);
  return 8 * encoder.stop_encoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_Data_Buffer(unsigned short data_buffer[],
                        Adaptive_Data_Model & model,
                        Arithmetic_Codec & decoder)
{
  decoder.start_decoder();
  for (unsigned k = 0; k < SimulTests; k++)
    data_buffer[k] = unsigned short(decoder.decode(model));
  decoder.stop_decoder();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
void Fill_Bit_Buffer(Random_Bit_Source & src,
                     unsigned char bit_buffer[])
{
  src.shuffle_probabilities();
  for (unsigned k = 0; k < SimulTests; k++)
    bit_buffer[k] = unsigned char(src.bit());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
void Fill_Data_Buffer(Random_Data_Source & src,
                      unsigned short data_buffer[])
{
  src.shuffle_probabilities();
  for (unsigned k = 0; k < SimulTests; k++)
    data_buffer[k] = unsigned short(src.data());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Display_Results(bool first,
                     bool adaptive,
                     Test_Result & pr,
                     double source_time)
{
  if (adaptive)
    puts(" Test with adaptive model\n");
  else {
    if (first)
      puts("\n==============================================================="
        "==========");
    puts(" Test with static model\n");
  }

  printf(" Random  data generated in %5.2f seconds\n", source_time);
  printf(" Encoder test completed in %5.2f seconds\n", pr.encoder_time);
  printf(" Decoder test completed in %5.2f seconds\n", pr.decoder_time);

  printf("\n Used %g bytes for coding %g symbols\n",
    0.125 * pr.bits_used, pr.test_symbols);
    
  printf(" Data source entropy = %8.5f bits/symbol [%d symbols]\n",
    pr.entropy, pr.alphabet_symbols);

  double bit_rate = pr.bits_used / pr.test_symbols;
  printf(" Compression rate    = %8.5f bits/symbol (%9.4f %% redundancy)\n\n",
    bit_rate, 100.0 * (bit_rate - pr.entropy) / pr.entropy);

  printf(" Encoding time  = %8.3f ns/symbol  = %8.3f ns/bit\n",
    1e9 * pr.encoder_time / pr.test_symbols,
    1e9 * pr.encoder_time / pr.bits_used);

  printf(" Decoding time  = %8.3f ns/symbol  = %8.3f ns/bit\n",
    1e9 * pr.decoder_time / pr.test_symbols,
    1e9 * pr.decoder_time / pr.bits_used);

  printf(" Encoding speed = %8.3f Msymbols/s = %8.3f Mbits/s\n",
    1e-6 * pr.test_symbols / pr.encoder_time,
    1e-6 * pr.bits_used / pr.encoder_time);

  printf(" Decoding speed = %8.3f Msymbols/s = %8.3f Mbits/s\n",
    1e-6 * pr.test_symbols / pr.decoder_time,
    1e-6 * pr.bits_used / pr.decoder_time);

  puts("====================================================================="
    "====");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Binary_Benchmark(int num_cycles)
{
                                                  // set simulation parameters
  int num_simulations = 10;
  double entropy = 0.1, entropy_increment = 0.1;
                                                                  // variables
  Test_Result        result;
  Random_Bit_Source  src;
  Arithmetic_Codec   codec(SimulTests >> 2);
  Static_Bit_Model   static_model;
  Adaptive_Bit_Model adaptive_model;
  Chronometer        encoder_time, decoder_time, source_time;

                                         // assign memory for random test data
  unsigned code_bits;
  unsigned char * source_bits  = new unsigned char[2*SimulTests];
  unsigned char * decoded_bits = source_bits + SimulTests;
  if (source_bits == 0) Error("Cannot assign memory for random bit buffer");

  for (int simul = 0; simul < num_simulations; simul++) {

    for (int pass = 0; pass <= 1; pass++) {

      src.set_entropy(entropy);
      src.set_seed(1839304 + 2017 * simul);

      result.alphabet_symbols = 2;
      result.entropy          = src.entropy();
      result.test_symbols     = 0;
      result.bits_used        = 0;

      source_time.reset();                           // reset all chronometers
      encoder_time.reset();
      decoder_time.reset();

      for (int cycle = 0; cycle < num_cycles; cycle++) {

        source_time.start();
        Fill_Bit_Buffer(src, source_bits);
        source_time.stop();

        if (pass == 0) {
          static_model.set_probability_0(src.symbol_0_probability());
          encoder_time.start();
          code_bits = Encode_Bit_Buffer(source_bits, static_model, codec);
          encoder_time.stop();

          decoder_time.start();
          Decode_Bit_Buffer(decoded_bits, static_model, codec);
          decoder_time.stop();
        }
        else {
          adaptive_model.reset();
          encoder_time.start();
          code_bits = Encode_Bit_Buffer(source_bits, adaptive_model, codec);
          encoder_time.stop();

          adaptive_model.reset();
          decoder_time.start();
          Decode_Bit_Buffer(decoded_bits, adaptive_model, codec);
          decoder_time.stop();
        }

        result.test_symbols += SimulTests;
        result.bits_used    += code_bits;

                                                  // check for decoding errors
        for (int k = 0; k < SimulTests; k++) 
          if (source_bits[k] != decoded_bits[k]) Error("incorrect decoding");
      }

      result.encoder_time = encoder_time.read();
      result.decoder_time = decoder_time.read();
      Display_Results(simul == 0, pass != 0, result, source_time.read());
    }
    entropy += entropy_increment;
  }

  delete [] source_bits;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void General_Benchmark(int data_symbols,
                       int num_cycles)
{
                                                  // set simulation parameters
  double entropy, entropy_increment;
  if (data_symbols <= 8) {
    entropy = 0.2;
    entropy_increment = 0.20;
  }
  else
    if (data_symbols <= 32) {
      entropy = 0.5;
      entropy_increment = 0.25;
    }
    else {
      entropy = 1.0;
      entropy_increment = 0.50;
    }

  int num_simulations = 1 + int((log(data_symbols) / log(2.0) - entropy) /
    entropy_increment);

                                                                  // variables
  Test_Result         result;
  Random_Data_Source  src;
  Arithmetic_Codec    codec(SimulTests << 1);
  Static_Data_Model   static_model;
  Adaptive_Data_Model adaptive_model(data_symbols);
  Chronometer         encoder_time, decoder_time, source_time;

                                         // assign memory for random test data
  unsigned code_bits;
  unsigned short * source_data  = new unsigned short[2*SimulTests];
  unsigned short * decoded_data = source_data + SimulTests;
  if (source_data == 0) Error("Cannot assign memory for random data buffer");

  adaptive_model.set_alphabet(data_symbols);

  for (int simul = 0; simul < num_simulations; simul++) {

    for (int pass = 0; pass <= 1; pass++) {

      src.set_truncated_geometric(data_symbols, entropy);
      src.set_seed(8315739 + 1031 * simul + 11 * data_symbols);

      result.alphabet_symbols = data_symbols;
      result.entropy          = src.entropy();
      result.test_symbols     = 0;
      result.bits_used        = 0;

      source_time.reset();                           // reset all chronometers
      encoder_time.reset();
      decoder_time.reset();

      for (int cycle = 0; cycle < num_cycles; cycle++) {

        source_time.start();
        Fill_Data_Buffer(src, source_data);
        source_time.stop();

        if (pass == 0) {
          static_model.set_distribution(data_symbols, src.probability());
          encoder_time.start();
          code_bits = Encode_Data_Buffer(source_data, static_model, codec);
          encoder_time.stop();

          decoder_time.start();
          Decode_Data_Buffer(decoded_data, static_model, codec);
          decoder_time.stop();
        }
        else {
          adaptive_model.reset();
          encoder_time.start();
          code_bits = Encode_Data_Buffer(source_data, adaptive_model, codec);
          encoder_time.stop();

          adaptive_model.reset();
          decoder_time.start();
          Decode_Data_Buffer(decoded_data, adaptive_model, codec);
          decoder_time.stop();
        }

        result.test_symbols += SimulTests;
        result.bits_used    += code_bits;

                                                  // check for decoding errors
        for (int k = 0; k < SimulTests; k++) 
          if (source_data[k] != decoded_data[k]) Error("incorrect decoding");
      }

      result.encoder_time = encoder_time.read();
      result.decoder_time = decoder_time.read();
      Display_Results(simul == 0, pass != 0, result, source_time.read());
    }
    entropy += entropy_increment;
  }

  delete [] source_data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Main function - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(int numb_arg, char * arg[])
{
                            // set number of tests from command-line parameter 
  if ((numb_arg != 2) && ((numb_arg != 3))) {
    puts(" Parameters: alphabet_symbols [test_cycles=10]");
    return 0;
  }

  int ns = atoi(arg[1]), tc = (numb_arg < 3 ? 10 : atoi(arg[2]));
  if ((ns < 2) || (ns > 500)) Error("invalid number of data symbols");
  if ((tc < 1) || (tc > 999)) Error("invalid number of simulations");

  if (ns == 2)
    Binary_Benchmark(tc);
  else
    General_Benchmark(ns, tc);

  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
