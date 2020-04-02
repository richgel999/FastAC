// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Fast arithmetic coding implementation                                     -
// -> double-precision floating-point arithmetic                             -
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
// A description of the arithmetic coding method used here is available in   -
//                                                                           -
// Lossless Compression Handbook, ed. K. Sayood                              -
// Chapter 5: Arithmetic Coding (A. Said), pp. 101-152, Academic Press, 2003 -
//                                                                           -
// A. Said, Introduction to Arithetic Coding Theory and Practice             -
// HP Labs report HPL-2004-76  -  http://www.hpl.hp.com/techreports/         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <math.h>
#include <stdlib.h>
#include "arithmetic_codec.h"


// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      // This encoder saves data 2 bytes a time: renormalization factor = 2^16
const double AC__OutputFactor   = double(0x10000);                 // D = 2^16
const double AC__MinLength      = 1.0 / double(0x10000);        // 1/D = 2^-16
const double AC__LeastSignifBit = 1.0 / double(0x1000000) / double(0x1000000);
const double AC__Leakage        = 2.0 * AC__LeastSignifBit;

const unsigned BM__MaxCount  = 1 << 14;     // Maximum bit count for Bit_Model
const unsigned DM__MaxCount  = 1 << 17; // Maximum symbol count for Data_Model


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static functions  - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void AC_Error(const char * msg)
{
  fprintf(stderr, "\n\n -> Arithmetic coding error: ");
  fputs(msg, stderr);
  fputs("\n Execution terminated!\n", stderr);
  exit(1);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Coding implementations  - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::propagate_carry(void)
{
  base -= 1.0;                  // carry propagation on compressed data buffer
  unsigned char * p;
  for (p = ac_pointer - 1; *p == 0xFFU; p--) *p = 0;
  ++*p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_enc_interval(void)
{
                     // rescale arithmetic encoder interval, output data bytes
  do {
    unsigned a = unsigned(base *= AC__OutputFactor);
    *ac_pointer++ = (unsigned char) (a >> 8);
    *ac_pointer++ = (unsigned char) (a & 0xFF);
    base -= double(a);
  } while ((length *= AC__OutputFactor) <= AC__MinLength);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_dec_interval(void)
{
                      // rescale arithmetic decoder interval, input data bytes
  do {
    unsigned a = unsigned(base *= AC__OutputFactor);
    base -= double(a);
    value = (AC__OutputFactor * value - double(a)) + AC__LeastSignifBit *
            (256.0 * double(ac_pointer[0]) + double(ac_pointer[1]));
    ac_pointer += 2;
  } while ((length *= AC__OutputFactor) <= AC__MinLength);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bit(unsigned bit)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif
                                              // encode assuming p0 = p1 = 1/2
  double x = 0.5 * length;                             // compute middle point

  if (bit == 0)                             // update interval base and length 
    length  = x;
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) propagate_carry();                  // check if carry bit
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bit(void)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  double x = 0.5 * length;                    // compute interval middle point
  unsigned bit = (value + AC__LeastSignifBit >= base + x); // decode bit value

  if (bit == 0)                             // update interval base and length 
    length  = x;
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) {                     // decoder does not propagate carry
      base   -= 1.0;                                         // shift interval
      value  -= 1.0;
    }
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bits(unsigned data,
                                unsigned bits)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
  if (data >= (1U << bits)) AC_Error("invalid data");
#endif

  unsigned symbols = 1U << bits;                              // alphabet size
  double y, d = length / symbols;               // assume uniform distribution
                                                           // compute products
  if (data == symbols - 1)
    y = base + length;                            // avoid multiplication by 1
  else
    y = base + d * (data + 1);
                                                           // set new interval
  base  += d * data;
  length = y - base;

  if (base >= 1.0) propagate_carry();                    // check if carry bit

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bits(unsigned bits)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
#endif

  unsigned s = 0, n = 1U << bits, m = n >> 1;
  double x = base, y = base + length, d = length / n;
  double shifted_value = value + AC__LeastSignifBit;

                   // bissection search of index in arithmetic coding interval
  do {
    double z = base + d * m;
    if (z > shifted_value) {
      y = z;                                          // code value is smaller
      n = m;
    }
    else {
      x = z;                                  // code value is larger or equal
      s = m;
    }
  } while ((m = (s + n) >> 1) != s);
                                                           // set new interval
  base   = x;
  length = y - x;

  if (base >= 1.0) {                       // decoder does not propagate carry
    base   -= 1.0;                                           // shift interval
    value  -= 1.0;
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Static_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  double x = length * M.bit_0_prob;                  // compute product l x p0

  if (bit == 0)                             // update interval base and length 
    length  = x;
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) propagate_carry();                  // check if carry bit
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  double x = length * M.bit_0_prob;         // compute interval-division point
  unsigned bit = (value + AC__LeastSignifBit >= base + x); // decode bit value

  if (bit == 0)                             // update interval base and length 
    length  = x;
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) {                     // decoder does not propagate carry
      base   -= 1.0;                                         // shift interval
      value  -= 1.0;
    }
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  double x = length * M.bit_0_prob;                  // compute product l x p0

  if (bit == 0) {                           // update interval length and base
    length  = x;
    ++M.bit_0_count;
  }
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) propagate_carry();                  // check if carry bit
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();

  if (--M.bits_until_update == 0) M.update();         // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  double x = length * M.bit_0_prob;         // compute interval-division point
  unsigned bit = (value + AC__LeastSignifBit >= base + x); // decode bit value

  if (bit == 0) {                           // update interval length and base
    length  = x;
    ++M.bit_0_count;
  }
  else {
    base   += x;
    length -= x;
    if (base >= 1.0) {                     // decoder does not propagate carry
      base   -= 1.0;                                         // shift interval
      value  -= 1.0;
    }
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  if (--M.bits_until_update == 0) M.update();         // periodic model update

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Static_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  double y;
                                                           // compute products
  if (data == M.data_symbols - 1)
    y = base + length;                            // avoid multiplication by 1
  else
    y = base + length * M.distribution[data+1];
                                                           // set new interval
  base  += length * M.distribution[data];
  length = y - base;

  if (base >= 1.0) propagate_carry();                    // check if carry bit

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned s = 0, n = M.data_symbols, m = n >> 1;
  double x = base, y = base + length;
  double shifted_value = value + AC__LeastSignifBit;

                   // bissection search of index in arithmetic coding interval
  do {
    double z = base + length * M.distribution[m];
    if (z > shifted_value) {
      y = z;                                          // code value is smaller
      n = m;
    }
    else {
      x = z;                                  // code value is larger or equal
      s = m;
    }
  } while ((m = (s + n) >> 1) != s);
                                                           // set new interval
  base   = x;
  length = y - x;

  if (base >= 1.0) {                       // decoder does not propagate carry
    base   -= 1.0;                                           // shift interval
    value  -= 1.0;
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Adaptive_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  double y;
                                                           // compute products
  if (data == M.data_symbols - 1)
    y = base + length;                            // avoid multiplication by 1
  else
    y = base + length * M.distribution[data+1];

  base  += length * M.distribution[data];  // set new interval base and length
  length = y - base;

  if (base >= 1.0) propagate_carry();                    // check if carry bit

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_enc_interval();

  ++M.symbol_count[data];
  if (--M.symbols_until_update == 0) M.update();      // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned s = 0, n = M.data_symbols, m = n >> 1;
  double x = base, y = base + length;
  double shifted_value = value + AC__LeastSignifBit;

                   // bissection search of index in arithmetic coding interval
  do {
    double z = base + length * M.distribution[m];
    if (z > shifted_value) {
      y = z;                                          // code value is smaller
      n = m;
    }
    else {
      x = z;                                  // code value is larger or equal
      s = m;
    }
  } while ((m = (s + n) >> 1) != s);
                                                           // set new interval
  base   = x;
  length = y - x;

  if (base >= 1.0) {                       // decoder does not propagate carry
    base   -= 1.0;                                           // shift interval
    value  -= 1.0;
  }

  if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
    renorm_dec_interval();

  ++M.symbol_count[s];
  if (--M.symbols_until_update == 0) M.update();      // periodic model update

  return s;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Other Arithmetic_Codec implementations  - - - - - - - - - - - - - - - -

Arithmetic_Codec::Arithmetic_Codec(void)                       // constructors
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
}

Arithmetic_Codec::Arithmetic_Codec(unsigned max_code_bytes,
                                   unsigned char * user_buffer)
{
  mode = buffer_size = 0;
  new_buffer = code_buffer = 0;
  set_buffer(max_code_bytes, user_buffer);
}

Arithmetic_Codec::~Arithmetic_Codec(void)                        // destructor
{
  delete [] new_buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::set_buffer(unsigned max_code_bytes,
                                  unsigned char * user_buffer)
{
                                                  // test for reasonable sizes
  if ((max_code_bytes < 16) || (max_code_bytes > 0x1000000U))
    AC_Error("invalid codec buffer size");
  if (mode != 0) AC_Error("cannot set buffer while encoding or decoding");

  if (user_buffer != 0) {                       // user provides memory buffer
    buffer_size = max_code_bytes;
    code_buffer = user_buffer;               // set buffer for compressed data
    delete [] new_buffer;                 // free anything previously assigned
    new_buffer = 0;
    return;
  }

  if (max_code_bytes <= buffer_size) return;               // enough available

  buffer_size = max_code_bytes;                           // assign new memory
  delete [] new_buffer;                   // free anything previously assigned
  if ((new_buffer = new unsigned char[buffer_size+16]) == 0) // 16 extra bytes
    AC_Error("cannot assign memory for compressed data buffer");
  code_buffer = new_buffer;                  // set buffer for compressed data
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_encoder(void)
{
  if (mode != 0) AC_Error("cannot start encoder");
  if (buffer_size == 0) AC_Error("no code buffer set");

  mode   =   1;
  base   = 0.0;          // initialize encoder variables: interval and pointer
  length = 1.0;
  ac_pointer = code_buffer;                       // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_decoder(void)
{
  if (mode != 0) AC_Error("cannot start decoder");
  if (buffer_size == 0) AC_Error("no code buffer set");

  mode   =   2;
  length = 1.0;                       // initialize decoder: interval, pointer
  value  = base = 0.0;
                                            // set initial code value: 48 bits
  for (unsigned k = 0; k < 6; k++)
    value = 256.0 * value + AC__LeastSignifBit * code_buffer[k];
  ac_pointer = code_buffer + 6;                   // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::read_from_file(FILE * code_file)
{
  unsigned shift = 0, code_bytes = 0;
  int file_byte;
                      // read variable-length header with number of code bytes
  do {
    if ((file_byte = getc(code_file)) == EOF)
      AC_Error("cannot read code from file");
    code_bytes |= unsigned(file_byte & 0x7F) << shift;
    shift += 7;
  } while (file_byte & 0x80);
                                                       // read compressed data
  if (code_bytes > buffer_size) AC_Error("code buffer overflow");
  if (fread(code_buffer, 1, code_bytes, code_file) != code_bytes)
    AC_Error("cannot read code from file");

  start_decoder();                                       // initialize decoder
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::stop_encoder(void)
{
  if (mode != 1) AC_Error("invalid to stop encoder");
  mode = 0;
                                        // done encoding: set final data bytes
  unsigned last_bytes;
  unsigned a = unsigned(AC__OutputFactor * base);
  unsigned b = unsigned(AC__OutputFactor * (base + length));

  if (b - a < 2) {     // decide number of final byes based on interval length
    base += 0.5 * AC__MinLength;
    last_bytes = 3;                                          // output 3 bytes
  }
  else
    if ((b >> 8) - (a >> 8) < 2) {
      base += AC__MinLength;
      last_bytes = 2;                                        // output 2 bytes
    }
    else {
      base += 256.0 * AC__MinLength;
      last_bytes = 1;                                         // output 1 byte
    }

  if (base >= 1.0) propagate_carry();

  do {
    base *= 256.0;
    unsigned a = unsigned(base);               // save 8 most-significant bits
    *ac_pointer++ = (unsigned char) a;
    base -= double(a);                       // rescale interval by factor 256
  } while (--last_bytes);

  unsigned code_bytes = unsigned(ac_pointer - code_buffer);
  if (code_bytes > buffer_size) AC_Error("code buffer overflow");

  return code_bytes;                                   // number of bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::write_to_file(FILE * code_file)
{
  unsigned header_bytes = 0, code_bytes = stop_encoder(), nb = code_bytes;

                     // write variable-length header with number of code bytes
  do {
    int file_byte = int(nb & 0x7FU);
    if ((nb >>= 7) > 0) file_byte |= 0x80;
    if (putc(file_byte, code_file) == EOF)
      AC_Error("cannot write compressed data to file");
    header_bytes++;
  } while (nb);
                                                      // write compressed data

  if (fwrite(code_buffer, 1, code_bytes, code_file) != code_bytes)
    AC_Error("cannot write compressed data to file");

  return code_bytes + header_bytes;                              // bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::stop_decoder(void)
{
  if (mode != 2) AC_Error("invalid to stop decoder");
  mode = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Static bit model implementation - - - - - - - - - - - - - - - - - - - - -

Static_Bit_Model::Static_Bit_Model(void)
{
  bit_0_prob = 0.5;                           // set initial probability value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Bit_Model::set_probability_0(double p0)
{
  if ((p0 < 0.00001)||(p0 > 0.99999))           // check for reasonable values
    AC_Error("invalid Static_Bit_Model probability");

  bit_0_prob = p0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Adaptive bit model implementation - - - - - - - - - - - - - - - - - - - -

Adaptive_Bit_Model::Adaptive_Bit_Model(void)
{
  reset();                                                 // initialize model
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::reset(void)
{
  bit_0_count = 1;                     // initialization to equiprobable model
  bit_count   = 2;
  bit_0_prob  = 0.5;
  update_cycle = bits_until_update = 4;         // start with frequent updates
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::update(void)
{
                                   // halve counts when a threshold is reached

  if ((bit_count += update_cycle) >= BM__MaxCount) {
    bit_count = (bit_count + 1) >> 1;
    bit_0_count = (bit_0_count + 1) >> 1;
    if (bit_0_count == bit_count) ++bit_count;
  }
                                           // compute scaled bit 0 probability

  bit_0_prob = double(bit_0_count) / double(bit_count);

                                             // set frequency of model updates
  update_cycle = (5 * update_cycle) >> 2;
  if (update_cycle > 64) update_cycle = 64;
  bits_until_update = update_cycle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static data model implementation  - - - - - - - - - - - - - - - - - - -

Static_Data_Model::Static_Data_Model(void)                                   // constructors
{
  data_symbols = 0;
  distribution = 0;
}

Static_Data_Model::~Static_Data_Model(void)                                    // destructor
{
  delete [] distribution;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Data_Model::set_distribution(unsigned number_of_symbols,
                                         const double probability[])
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 14)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    delete [] distribution;
    distribution = new double[data_symbols];
    if (distribution == 0) AC_Error("cannot assign model memory");
  }
                                            // compute cumulative distribution
  double sum = 0.0, p = 1.0 / double(data_symbols);
  for (unsigned k = 0; k < data_symbols; k++) {
    if (probability) p = probability[k];
    if ((p < 0.00001) || (p > 0.99999)) AC_Error("invalid symbol probability");
    distribution[k] = sum;
    sum += p;
  }
  if ((sum < 0.99999) || (sum > 1.00001))
    AC_Error("invalid set of probabilities");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Adaptive data model implementation  - - - - - - - - - - - - - - - - - -

Adaptive_Data_Model::Adaptive_Data_Model(void)                 // constructors
{
  data_symbols = 0;
  distribution = 0;
  symbol_count = 0;
}

Adaptive_Data_Model::Adaptive_Data_Model(unsigned number_of_symbols)
{
  data_symbols = 0;
  distribution = 0;
  symbol_count = 0;
  set_alphabet(number_of_symbols);
}

Adaptive_Data_Model::~Adaptive_Data_Model(void)                  // destructor
{
  delete [] distribution;
  delete [] symbol_count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::set_alphabet(unsigned number_of_symbols)
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 14)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    delete [] distribution;
    delete [] symbol_count;
    symbol_count = new unsigned[data_symbols];
    distribution = new double[data_symbols];
    if ((symbol_count == 0) || (distribution == 0))
      AC_Error("cannot assign model memory");
  }

  reset();                                                 // initialize model
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::update(void)
{
                                   // halve counts when a threshold is reached

  if ((total_count += update_cycle) > DM__MaxCount) {
    total_count = 0;
    for (unsigned n = 0; n < data_symbols; n++)
      total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
  }

  unsigned sum = 0;
  double scale = 1.0 / double(total_count);
                                            // compute cumulative distribution
  for (unsigned k = 0; k < data_symbols; k++) {
    distribution[k] = scale * sum;
    sum += symbol_count[k];
  }
                                             // set frequency of model updates
  update_cycle = (5 * update_cycle) >> 2;
  unsigned max_cycle = (data_symbols + 6) << 3;
  if (update_cycle > max_cycle) update_cycle = max_cycle;
  symbols_until_update = update_cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::reset(void)
{
  if (data_symbols == 0) return;

                      // restore probability estimates to uniform distribution
  total_count = 0;
  update_cycle = data_symbols;
  for (unsigned k = 0; k < data_symbols; k++) symbol_count[k] = 1;
  update();
  symbols_until_update = update_cycle = (data_symbols + 6) >> 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
