// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Implementation of random-number generators and chronometer                -
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


// - - Definitions - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef TEST_SUPPORT
#define TEST_SUPPORT

void Error(const char * message);               // stops execution after error

const double MinProbability = 1e-4; // avoid prob. too small for static models


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Class definitions - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Chronometer
{                                          // Class to measure execution times
public:

  Chronometer(void)  { time = 0.0;  on = false; }

  void   reset(void) { time = 0.0;  on = false; }

  void   start(char * s = 0);
  void   display(char * s = 0);
  void   restart(char * s = 0);

  void   stop(void);
  double read(void);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  bool   on;
  double mark, time;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Random_Generator
{                                            // Pseudo-random number generator
public:

  Random_Generator(void)          { set_seed(0); }
  Random_Generator(unsigned seed) { set_seed(seed); }

  void     set_seed(unsigned seed);

  unsigned word(void);                          // 32-bit pseudo-random number
  double   uniform(void);                      // same, converted to double fp
  unsigned integer(unsigned range) { return unsigned(range * uniform()); }

protected: // .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
  unsigned s1, s2, s3;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Random_Bit_Source : public Random_Generator
{
public:  // Pseudo-random generator of binary symbols with given probabilities

  Random_Bit_Source(void) { prob_0 = 0.5;  ent = 1.0; }

  double   entropy(void)              { return ent; }
  double   symbol_0_probability(void) { return prob_0; }
  double   symbol_1_probability(void) { return 1.0 - prob_0; }

  unsigned bit(void);         // pseudo-random bit with predefined probability

  void     switch_probabilities(void);
  void     shuffle_probabilities(void);

  void     set_entropy(double entropy);
  double   set_probability_0(double symbol_0_probability);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  unsigned threshold;
  double   ent, prob_0;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Random_Data_Source : public Random_Generator
{
public:    // Pseudo-random generator of data symbols with given probabilities

  Random_Data_Source(void);
 ~Random_Data_Source(void);

  double   entropy(void)      { return ent; }
  double * probability(void)  { return prob; }
  double   data_symbols(void) { return symbols; }

  unsigned data(void);     // pseudo-random data with predefined distributions

  void     shuffle_probabilities(void);

  double   set_distribution(unsigned data_symbols,
                            const double probability[]);

  double   set_truncated_geometric(unsigned data_symbols,
                                   double entropy);

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  void     assign_memory(unsigned);
  double   set_tg(double);
  double   ent, * prob;
  unsigned symbols, * dist, low_bound[257];
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Zero_Finder
{
public:        // Class used by 'Random_Data_Source' to find source parameters

  Zero_Finder(double first_test, double second_test);

  double set_new_result(double function_value);            // returns new test

private:  //  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
  int    phase, iter;
  double x0, y0, x1, y1, x2, y2, x;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Inline implementations  - - - - - - - - - - - - - - - - - - - - - - - -

inline unsigned Random_Generator::word(void)
{
        // "Taus88" generator with period (2^31 - 1) * (2^29 - 1) * (2^28 - 1)

  register unsigned b;
  b  = ((s1 << 13) ^ s1)   >> 19;
  s1 = ((s1 & 0xFFFFFFFEU) << 12) ^ b;
  b  = ((s2 <<  2) ^ s2)   >> 25;
  s2 = ((s2 & 0xFFFFFFF8U) <<  4) ^ b;
  b  = ((s3 <<  3) ^ s3)   >> 11;
  s3 = ((s3 & 0xFFFFFFF0U) << 17) ^ b;
  return s1 ^ s2 ^ s3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline double Random_Generator::uniform(void)
{
  const double WordToDouble = 1.0 / (1.0 + double(0xFFFFFFFFU));
 
  register unsigned b;
  b  = ((s1 << 13) ^ s1)   >> 19;
  s1 = ((s1 & 0xFFFFFFFEU) << 12) ^ b;
  b  = ((s2 <<  2) ^ s2)   >> 25;
  s2 = ((s2 & 0xFFFFFFF8U) <<  4) ^ b;
  b  = ((s3 <<  3) ^ s3)   >> 11;
  s3 = ((s3 & 0xFFFFFFF0U) << 17) ^ b;             // open interval: 0 < r < 1
  return WordToDouble * (0.5 + double(s1 ^ s2 ^ s3));
}

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
