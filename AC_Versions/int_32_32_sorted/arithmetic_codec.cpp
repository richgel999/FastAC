// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Fast arithmetic coding implementation                                     -
// -> 32-bit variables, 32-bit product, periodic updates, sorted symbols     -
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

#include <stdlib.h>
#include "arithmetic_codec.h"


// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const unsigned AC__MinLength = 0x01000000U;   // threshold for renormalization
const unsigned AC__MaxLength = 0xFFFFFFFFU;      // maximum AC interval length

                                           // Maximum values for binary models
const unsigned BM__LengthShift = 13;     // length bits discarded before mult.
const unsigned BM__MaxCount    = 1 << BM__LengthShift;  // for adaptive models

                                          // Maximum values for general models
const unsigned DM__LengthShift = 15;     // length bits discarded before mult.
const unsigned DM__MaxCount    = 1 << DM__LengthShift;  // for adaptive models


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
  unsigned char * p;            // carry propagation on compressed data buffer
  for (p = ac_pointer - 1; *p == 0xFFU; p--) *p = 0;
  ++*p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_enc_interval(void)
{
  do {                                          // output and discard top byte
    *ac_pointer++ = (unsigned char)(base >> 24);
    base <<= 8;
  } while ((length <<= 8) < AC__MinLength);        // length multiplied by 256
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_dec_interval(void)
{
  do {                                              // read least-signif. byte
    value = (value << 8) | unsigned(*++ac_pointer);
  } while ((length <<= 8) < AC__MinLength);        // length multiplied by 256
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bit(unsigned bit)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  length >>= 1;                                              // halve interval
  if (bit) {
    unsigned init_base = base;
    base += length;                                               // move base
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bit(void)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  length >>= 1;                                              // halve interval
  unsigned bit = (value >= length);                              // decode bit
  if (bit) value -= length;                                       // move base

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bits(unsigned data, unsigned bits)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
  if (data >= (1U << bits)) AC_Error("invalid data");
#endif

  unsigned init_base = base;
  base += data * (length >>= bits);            // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bits(unsigned bits)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
  if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
#endif

  unsigned s = value / (length >>= bits);      // decode symbol, change length

  value -= length * s;                                      // update interval
  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Static_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif
            // multiplication approximated by two bit shifts and two additions

  unsigned x = length - (length >> M.shift_a) - (length >> M.shift_b);

                                                            // update interval
  if (M.least_probable_bit ^ (bit != 0))
    length  = x;                           // simplest case is the most common
  else {
    unsigned init_base = base;
    base   += x;
    length -= x;
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif
            // multiplication approximated by two bit shifts and two additions

  unsigned x = length - (length >> M.shift_a) - (length >> M.shift_b);

  unsigned mpb = (value < x);                                      // decision
                                                    // update & shift interval
  if (mpb)
    length  = x;
  else {
    value  -= x;                                  // shifted interval base = 0
    length -= x;
  }

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return mpb ^ M.least_probable_bit;                     // return decoded bit
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
                              Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
#endif

  unsigned x = M.mpb_prob * (length >> BM__LengthShift);     // product l x pm
                                                            // update interval
  if (M.least_probable_bit ^ (bit != 0))
    length = x;                            // simplest case is the most common
  else {
    ++M.lpb_count;
    unsigned init_base = base;
    base   += x;
    length -= x;
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization

  if (--M.bits_until_update == 0) M.update();         // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned x = M.mpb_prob * (length >> BM__LengthShift);     // product l x pm
  unsigned mpb = (value < x);                                      // decision
                                                            // update interval
  if (mpb)
    length = x;
  else {
    ++M.lpb_count;
    value  -= x;
    length -= x;
  }

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  if (--M.bits_until_update) return mpb ^ M.least_probable_bit;  // return bit
                                                        
  unsigned bit = mpb ^ M.least_probable_bit;  // save bit value before changes
  M.update();                                         // periodic model update
  return bit;                                            // return decoded bit
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Static_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  unsigned x, init_base = base, s = M.rank[data];             // symbol = rank
                                                           // compute products
  if (s == M.most_probable_symbol) {
    x = M.distribution[s] * (length >> DM__LengthShift);
    base   += x;                                            // update interval
    length -= x;                                          // no product needed
  }
  else {
    x = M.distribution[s] * (length >>= DM__LengthShift);
    base   += x;                                            // update interval
    length  = M.distribution[s+1] * length - x;
  }
             
  if (init_base > base) propagate_carry();                 // overflow = carry

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned s, n, x, y = length, m = M.first_tests[1];
  unsigned z = M.distribution[m] * (length >>= DM__LengthShift);

  if (z > value) {             // first predefined test based on probabilities
    n = m;               // initialize search from bottom and define next test
    y = z;
    x = s = 0;
    m = M.first_tests[0];
  }
  else {                    // initialize search from top and define next test
    s = m;
    x = z;
    n = M.data_symbols;
    m = M.first_tests[2];
  }

  if (n - s > 1)                  // if necessary finish with bisection search
    do {
      z = length * M.distribution[m];
      if (z > value) {
        n = m;
        y = z;                                             // value is smaller
      }
      else {
        s = m;
        x = z;                                     // value is larger or equal
      }
    } while ((m = (s + n) >> 1) != s);

  value -= x;                                               // update interval
  length = y - x;

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  return M.data[s];                               // return decoded data value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
                              Adaptive_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 1) AC_Error("encoder not initialized");
  if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

  unsigned x, init_base = base, s = M.rank[data];             // symbol = rank
                                                           // compute products
  if (s == M.most_probable_symbol) {
    x = M.distribution[s] * (length >> DM__LengthShift);
    base   += x;                                            // update interval
    length -= x;                                          // no product needed
  }
  else {
    x = M.distribution[s] * (length >>= DM__LengthShift);
    base   += x;                                            // update interval
    length  = M.distribution[s+1] * length - x;
  }
             
  if (init_base > base) propagate_carry();                 // overflow = carry

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization

  ++M.symbol_count[s];
  if (--M.symbols_until_update == 0) M.update();      // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model & M)
{
#ifdef _DEBUG
  if (mode != 2) AC_Error("decoder not initialized");
#endif

  unsigned s, n, x, y = length, m = M.first_tests[1];
  unsigned z = M.distribution[m] * (length >>= DM__LengthShift);

  if (z > value) {             // first predefined test based on probabilities
    n = m;               // initialize search from bottom and define next test
    y = z;
    x = s = 0;
    m = M.first_tests[0];
  }
  else {                    // initialize search from top and define next test
    s = m;
    x = z;
    n = M.data_symbols;
    m = M.first_tests[2];
  }

  if (n - s > 1)                  // if necessary finish with bisection search
    do {
      z = length * M.distribution[m];
      if (z > value) {
        n = m;
        y = z;                                             // value is smaller
      }
      else {
        s = m;
        x = z;                                     // value is larger or equal
      }
    } while ((m = (s + n) >> 1) != s);

  value -= x;                                               // update interval
  length = y - x;

  if (length < AC__MinLength) renorm_dec_interval();        // renormalization

  ++M.symbol_count[s];
  if (--M.symbols_until_update) return M.data[s];       // return decoded data
    
  unsigned data = M.data[s];                 // save data value before changes
  M.update();                                         // periodic model update
  return data;                                          // return decoded data
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Other Arithmetic_Codec implementations  - - - - - - - - - - - - - - - -

Arithmetic_Codec::Arithmetic_Codec(void)
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

Arithmetic_Codec::~Arithmetic_Codec(void)
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

  mode   = 1;
  base   = 0;            // initialize encoder variables: interval and pointer
  length = AC__MaxLength;
  ac_pointer = code_buffer;                       // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_decoder(void)
{
  if (mode != 0) AC_Error("cannot start decoder");
  if (buffer_size == 0) AC_Error("no code buffer set");

                  // initialize decoder: interval, pointer, initial code value
  mode   = 2;
  length = AC__MaxLength;
  ac_pointer = code_buffer + 3;
  value = (unsigned(code_buffer[0]) << 24)|(unsigned(code_buffer[1]) << 16) |
          (unsigned(code_buffer[2]) <<  8)| unsigned(code_buffer[3]);
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

  unsigned init_base = base;            // done encoding: set final data bytes

  if (length > 2 * AC__MinLength) {
    base  += AC__MinLength;                                     // base offset
    length = AC__MinLength >> 1;             // set new length for 1 more byte
  }
  else {
    base  += AC__MinLength >> 1;                                // base offset
    length = AC__MinLength >> 9;            // set new length for 2 more bytes
  }

  if (init_base > base) propagate_carry();                 // overflow = carry

  renorm_enc_interval();                // renormalization = output last bytes

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
  least_probable_bit = 0;
  shift_a = shift_b = 2;                                           // pm = 0.5
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Bit_Model::set_probability_0(double p0)
{
  if ((p0 < 0.0001)||(p0 > 0.9999)) AC_Error("invalid bit probability");

  if (p0 < 0.5)                                   // define least probable bit
    least_probable_bit = 0;
  else {
    least_probable_bit = 1;
    p0 = 1.0 - p0;
  }
  
  const double ProbThr[64] = {                       // probability thresholds
    1.000000000, 0.436829205, 0.343297135, 0.296716418,
    0.265429157, 0.217670149, 0.171498704, 0.148324226,
    0.132682629, 0.108718131, 0.085722481, 0.074155662,
    0.066335033, 0.054334924, 0.042855429, 0.037076405,
    0.033166108, 0.027161937, 0.021426357, 0.018537866,
    0.016582720, 0.013579645, 0.010712850, 0.009268851,
    0.008291278, 0.006789498, 0.005356344, 0.004634405,
    0.004145619, 0.003394669, 0.002678152, 0.002317198,
    0.002072805, 0.001697315, 0.001339071, 0.001158598,
    0.001036401, 0.000848652, 0.000669534, 0.000579299,
    0.000518200, 0.000424325, 0.000334767, 0.000289649,
    0.000259100, 0.000212162, 0.000167383, 0.000144825,
    0.000129550, 0.000106081, 0.000083692, 0.000072412,
    0.000064775, 0.000053040, 0.000041846, 0.000036206,
    0.000032387, 0.000026520, 0.000020923, 0.000018103,
    0.000016194, 0.000013260, 0.000010461, 0.000009052 };

  unsigned u = 0, n = 64, m = 32;         // find optimal values of bit shifts

  do {
    if (p0 < ProbThr[m])
      u = m;
    else
      n = m;
  } while ((m = (u + n) >> 1) != u);

  shift_a = 2 + (u >> 2);
  shift_b = shift_a + (u & 0x3);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Adaptive bit model implementation - - - - - - - - - - - - - - - - - - - -

Adaptive_Bit_Model::Adaptive_Bit_Model(void)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::reset(void)
{
                                       // initialization to equiprobable model
  least_probable_bit  = 0;
  lpb_count = 1;
  bit_count = 2;
  mpb_prob  = 1U << (BM__LengthShift - 1);
  update_cycle = bits_until_update = 4;         // start with frequent updates
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::update(void)
{
                               // halve counts when a top threshold is reached

  if ((bit_count += update_cycle) >= BM__MaxCount) {
    bit_count = (bit_count + 1) >> 1;
    lpb_count = (lpb_count + 1) >> 1;
    if (lpb_count == bit_count) ++bit_count;
  }
                                                     // test most probable bit
  unsigned mpb_count = bit_count - lpb_count;
  if (mpb_count < lpb_count) {
    mpb_count = lpb_count;
    lpb_count = bit_count - mpb_count;
    least_probable_bit ^= 1;
  }
                                           // compute scaled bit 0 probability
  unsigned scale = 0x80000000U / bit_count;
  mpb_prob = (mpb_count * scale) >> (31 - BM__LengthShift);

                                             // set frequency of model updates
  update_cycle = (5 * update_cycle) >> 2;
  if (update_cycle > 64) update_cycle = 64;
  bits_until_update = update_cycle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static data model implementation  - - - - - - - - - - - - - - - - - - -

Static_Data_Model::Static_Data_Model(void)
{
  data_symbols = 0;
  distribution = 0;
}

Static_Data_Model::~Static_Data_Model(void)
{
  delete [] distribution;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Data_Model::set_distribution(unsigned number_of_symbols,
                                  const double probability[])
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    most_probable_symbol = data_symbols - 1;
    delete [] distribution;
    distribution = new unsigned[3*data_symbols];
    rank = distribution + data_symbols;
    data = rank + data_symbols;
    if (distribution == 0) AC_Error("cannot assign model memory");
  }
                           // sort symbols by probability using insertion sort
  unsigned i, k;
  double p = 1.0 / double(data_symbols);     // value for uniform distribution

  if (probability == 0)
    for (k = 0; k < data_symbols; k++) data[k] = k;
  else 
    for (k = 0; k < data_symbols; k++) {
      unsigned s = k;
      double t = probability[k];
      for (i = k; i > 0; i--) {
        if (t >= probability[data[i-1]]) break;
        data[i] = data[i-1];
      }
      data[i] = s;
    }
                               // compute cumulative distribution, first tests
  unsigned c = 0;
  double sum = 0.0, threshold = 0.26;

  for (i = 0; i < data_symbols; i++) {
    k = data[i];
    rank[k] = i;
    if (probability) p = probability[k];
    if ((p < 0.0001) || (p > 0.9999)) AC_Error("invalid symbol probability");
    distribution[i] = unsigned(sum * (1 << DM__LengthShift));
    sum += p;
    while (sum > threshold) {
      first_tests[c++] = i;
      threshold += 0.25;
    }
  }
  if (first_tests[0] == first_tests[1]) --first_tests[0]; 

  if ((sum < 0.9999) || (sum > 1.0001)) AC_Error("invalid probabilities");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Adaptive data model implementation  - - - - - - - - - - - - - - - - - -

Adaptive_Data_Model::Adaptive_Data_Model(void)
{
  data_symbols = 0;
  distribution = 0;
}

Adaptive_Data_Model::Adaptive_Data_Model(unsigned number_of_symbols)
{
  data_symbols = 0;
  distribution = 0;
  set_alphabet(number_of_symbols);
}

Adaptive_Data_Model::~Adaptive_Data_Model(void)
{
  delete [] distribution;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::set_alphabet(unsigned number_of_symbols)
{
  if ((number_of_symbols < 2) || (number_of_symbols > (1 << 11)))
    AC_Error("invalid number of data symbols");

  if (data_symbols != number_of_symbols) {     // assign memory for data model
    data_symbols = number_of_symbols;
    most_probable_symbol = data_symbols - 1;
    delete [] distribution;
    distribution = new unsigned[4*data_symbols];
    symbol_count = distribution + data_symbols;
    rank = symbol_count + data_symbols;
    data = rank + data_symbols;
    if (distribution == 0) AC_Error("cannot assign model memory");
  }

  reset();                                                 // initialize model
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::update(void)
{
                               // halve counts when a top threshold is reached

  if ((total_count += update_cycle) >= DM__MaxCount) {
    total_count = 0;
    for (unsigned n = 0; n < data_symbols; n++)
      total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
  }

  unsigned i, k;                                  // update sorting of symbols

  for (k = 1; k < data_symbols; k++)
    if (symbol_count[k] < symbol_count[k-1]) {
      unsigned t = symbol_count[k];
      symbol_count[k]  = symbol_count[k-1];
      unsigned s = data[k];
      data[k] = data[k-1];
      for (i = k - 1; i > 0; i--) {
        if (t >= symbol_count[i-1]) break;
        symbol_count[i]  = symbol_count[i-1];
        data[i] = data[i-1];
      }
      symbol_count[i]  = t;
      data[i] = s;
    }
                               // compute cumulative distribution, first tests
  unsigned sum = 0, c = 0;
  unsigned d = (total_count + 3) >> 2, threshold = d + 1;
  unsigned scale = 0x80000000U / total_count;

  for (i = 0; i < data_symbols; i++) {
    rank[data[i]] = i;
    distribution[i] = (scale * sum) >> (31 - DM__LengthShift);
    sum += symbol_count[i];
    while (sum > threshold) {
      first_tests[c++] = i;
      threshold += d;
    }
  }
  if (first_tests[0] == first_tests[1]) --first_tests[0]; 

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
  for (unsigned k = 0; k < data_symbols; k++) {
    data[k] = k;
    symbol_count[k] = 1;
  }
  update();
  symbols_until_update = update_cycle = (data_symbols + 6) >> 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
