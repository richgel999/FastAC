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


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_support.h"

#ifdef CLOCKS_PER_SEC
const double ClockRate = 1.0 / CLOCKS_PER_SEC;
#else
const double ClockRate = 1.0e-6;
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Implementations - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Error(const char * s)
{
  fprintf(stderr, "\n\n -> Error: ");
  fputs(s, stderr);
  fputs("\n Execution terminated!\n", stderr);
  exit(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Chronometer::start(char * s)
{
  if (s != NULL) puts(s);
  if (on)
    puts("chronometer already on!");
  else {
    on   = true;
    mark = double(clock());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Chronometer::restart(char * s)
{
  if (s != NULL) puts(s);
  time = 0.0;
  on   = true;
  mark = double(clock());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Chronometer::stop(void)
{
  if (on) {
    on = false;
    time += clock() - mark;
  }
  else
    puts("chronometer already off!");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Chronometer::read(void)
{
  return (on ? time + (clock() - mark) : time) * ClockRate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Chronometer::display(char * s)
{
  double sc = (on ? time + (clock() - mark) : time) * ClockRate;
  int hr = int(sc / 3600.0);  sc -= 3600.0 * hr;
  int mn = int(sc / 60.0);  sc -= 60.0 * mn;
  if (s != NULL) printf(" %s ", s);
  if (hr) {
    printf("%d hour", hr);
    if (hr > 1) printf("s, "); else printf(", ");
  }
  if ((hr) || (mn)) {
    printf("%d minute", mn);
    if (mn > 1) printf("s, and "); else printf(", and ");
  }
  printf("%5.2f seconds.\n", sc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Generator::set_seed(unsigned seed)
{
  s1 = (seed ? seed & 0xFFFFFFFU : 0x147AE11U);
  s2 = s1 ^ 0xFFFFF07U;
  s3 = s1 ^ 0xF03CD2FU;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Random_Bit_Source::set_probability_0(double p0)
{
  if ((p0 < MinProbability) || (p0 > 1.0 - MinProbability))
    Error("invalid random bit probability");

  prob_0 = p0;
  threshold = unsigned(prob_0 * double(0xFFFFFFFFU));
  ent = ((p0 - 1.0) * log(1.0 - p0) - p0 * log(p0)) / log(2.0);

  return ent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Bit_Source::set_entropy(double entropy)
{
  if ((entropy < 0.0001) || (entropy > 1.0))
    Error("invalid random bit entropy");

  double h = entropy * log(2.0), p = 0.5 * entropy * entropy;
  for (int k = 0; k < 8; k++) {
    double lp1 = log(1.0 - p), lp2 = lp1 - log(p);
    double d = h + lp1 - p * lp2;
    if (fabs(d) < 1e-12) break;
    p += d / lp2;
  }
  set_probability_0(p);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Bit_Source::switch_probabilities(void)
{
  set_probability_0(1.0 - prob_0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Bit_Source::shuffle_probabilities(void)
{
  if (word() > 0x80000000U) set_probability_0(1.0 - prob_0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Random_Bit_Source::bit(void)
{
  return (word() > threshold ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Random_Data_Source::Random_Data_Source(void)
{
  symbols = 0;
  dist = 0;
  prob = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Random_Data_Source::~Random_Data_Source(void)
{ 
  delete [] dist;
  delete [] prob;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Data_Source::assign_memory(unsigned dim)
{
  if (symbols == dim) return;
  symbols = dim;
  delete [] dist;
  delete [] prob;
  prob = new double[dim];
  dist = new unsigned[dim];
  if ((prob == 0) || (dist == 0)) Error("cannot assign random source buffer");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Random_Data_Source::set_distribution(unsigned dim,
                                            const double probability[])
{
  assign_memory(dim);

  double sum = ent = 0.0;
  unsigned s = low_bound[0] = 0;
  const double DoubleToWord = 1.0 + double(0xFFFFFFFFU);

  for (unsigned n = 0; n < symbols; n++) {
    double p = probability[n];
    if (p < MinProbability) Error("invalid random source probability");
    prob[n] = p;
    dist[n] = unsigned(0.49 + DoubleToWord * sum);
    unsigned w = (dist[n] >> 24);
    while (s < w) low_bound[++s] = n - 1;
    sum += p;
    ent -= p * log(p);
  }
  while (s < 256) low_bound[++s] = symbols - 1;

  if (fabs(1.0 - sum) > 1e-4) Error("invalid random source distribution");
  return ent /= log(2.0);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Random_Data_Source::set_tg(double a)
{
  double s, r = 0, e = 0, m = symbols;

  if (a > 1e-4)
    s = (1.0 - exp(-a)) / (1.0 - exp(-a * m));
  else
    s = (2.0 - a) / (m * (2.0 - a * m));

  for (int n = symbols - 1; n >= 0; n--) {

    double p = (a * n > 30.0 ? 0 : s * exp(-a * n));

    if (p < MinProbability) {
      r += MinProbability - p;
      p = MinProbability;
    }
    else
      if (r > 0)
        if (r <= p - MinProbability) {
          p -= r;
          r = 0;
        }
        else {
          r -= p - MinProbability;
          p = MinProbability;
        }
    prob[n] = p;
    e -= p * log(p);
  }

  return e / log(2.0);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Random_Data_Source::set_truncated_geometric(unsigned dim,
                                                   double entropy)
{
  assign_memory(dim);

  double max_entropy = log(symbols) / log(2.0);
  double mgr_prob = (dim - 1) * MinProbability;
  double min_entropy = ((mgr_prob - 1.0) * log(1.0 - mgr_prob) -
           mgr_prob * log(MinProbability)) * 1.2 / log(2.0);

  if ((entropy <= min_entropy) || (entropy > max_entropy))
    Error("invalid data source entropy");

                                   // find distribution with desired entropy
  Zero_Finder ZF(0, 2);
  double a = ZF.set_new_result(max_entropy - entropy);

  for (unsigned itr = 0; itr < 20; itr++) {
    double ne = set_tg(a) - entropy;
    if (fabs(ne) < 1e-5) break;
    a = ZF.set_new_result(ne);
  }

  set_distribution(symbols, prob);
  if (fabs(ent - entropy) > 1e-4) Error("cannot set random source entropy");

  return ent;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Random_Data_Source::shuffle_probabilities(void)
{
  for (unsigned n = symbols - 1; n; n--) {
    unsigned m = integer(n + 1);
    if (m == n) continue;
    double t = prob[m];
    prob[m] = prob[n];
    prob[n] = t;
  }
  set_distribution(symbols, prob);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Random_Data_Source::data(void)
{
  unsigned v = word(), w = v >> 24;
  unsigned u = low_bound[w], n = low_bound[w+1] + 1;
  while (n > u + 1) {
    unsigned m = (u + n) >> 1;
    if (dist[m] < v) u = m; else n = m;
  }
  return u;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Zero_Finder::Zero_Finder(double first_x, double second_x)
{
  phase = iter = 0;
  x0 = first_x;
  x1 = second_x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

double Zero_Finder::set_new_result(double y)
{
  if (++iter > 30) Error("cannot find solution");

  if (phase >= 2) {
    if (y * y0 <= 0) {
      if ((phase == 2) || (fabs(y1) < fabs(y2))) {
        x2  = x1;
        y2  = y1;
      }
      x1  = x;
      y1  = y;
    }
    else {
      if ((phase == 2) || (fabs(y0) < fabs(y2))) {
        x2  = x0;
        y2  = y0;
      }
      x0  = x;
      y0  = y;
    }
                                   // interpolation y = [(x-x0)-f]/[g(x-x0)+h]
    if (fabs(y0) < fabs(y1)) {
      double r = y0 / y2, c = x2 - x0;
      double s = y0 / y1, d = x1 - x0;
      x = x0 - (c * d * (s - r)) / (c * (1.0 - s) - d * (1.0 - r));
    }
    else {
      double r = y1 / y2, c = x2 - x1;
      double s = y1 / y0, d = x0 - x1;
      x = x1 - (c * d * ( s - r)) / (c * (1.0 - s) - d * (1.0 - r));
    }
    phase = 3;
    return x;
  }

  if (iter > 8) Error("too many initial tests");

  if (phase == 1) {
    if (y * y0 <= 0) {                  // different signs: bracketed interval
      y1 = y;
      phase = 2;
      if (fabs(y0) < fabs(y1)) {                 // regula falsi interpolation
        double s = y0 / y1;
        x = x0 - ((x1 - x0) * s) / (1.0 - s);
      }
      else {
        double s = y1 / y0;
        x = x1 - ((x0 - x1) * s) / (1.0 - s);
      }
    }
    else {                              // same sign: increase search interval
      x += x1 - x0;
      x0 = x1;
      y0 = y;
      x1 = x;
    }
    return x;
  }

  y0 = y;
  phase = 1;

  return x = x1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
