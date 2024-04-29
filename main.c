#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <math.h>

#define TWO_PI 6.28318530717958647688

// the noise level here is the noise amplitude compared to the signal
// in the real world we would hope the signal is above the noise floor
//
// struggles at 20 starts going off course at around 40
// with a 1000 long integral buffer
// but can cope with a 2000 buffer!
//
#define NOISE_LEVEL 10.0 /* 4.0 */
#define DC_TERM 2048.0
#define BUFF_SIZE (10*4096)

int64_t buffer[BUFF_SIZE];
double dbuff[BUFF_SIZE], level, f_level, ff_level;

int64_t z_0, z_1_z_2;
int64_t p_0, p_1, p_2;

#define TWO_T_32 4294967296.0


//------------------------------------------------------------------------------
// No d.c. double zero placed at d.c (Z-1)
// pole placed at (Z-0.9)
//
// This is a kind of low pass filter aimed at removing d.c.
// It gives a slight gain (about 1.2) to frequecies not near d.c.
//------------------------------------------------------------------------------
double alpha = 0.1;
double no_dc_filter ( double x0 ) {
	static double x1,x2,y0,y1,y2;

	y0 = x0 - 2.0*x1 + x2  // zeros
		+ 2.0 * (1.0-alpha) * y1 - (1.0-alpha)*(1.0-alpha) * y2;

	x2 = x1;
	x1 = x0;
	y2 = y1;
	y1 = y0;

	return y0;

}
/*
double alpha2 = 0.1;
double dc_pass_filter ( double x0 ) {
	static double x1,x2,y0,y1,y2;

	y0 = x0 - 2.0*(1.0-alpha2)*x1 // zeros z^0 z^-1 
		+ (1.0-alpha2)*(1.0-alpha2)*x2  // zero z^2-2
		+ 2.0 *  (1.0-(alpha2/2.0)) * y1 // poles closer (alpha2/2) 
		- (1.0-alpha2/2.0)*(1.0-alpha/2.0) * y2;

	x2 = x1;
	x1 = x0;
	y2 = y1;
	y1 = y0;

	return y0;

}
*/
// part of the lock in filter is the integral, the lovck in rectification/amplification
// only becomes meaningfull when a large number of noise samples can cancel out each other
#define INCREMENT_BUFFER_SIZE 2000
double increment_buffer[INCREMENT_BUFFER_SIZE];

void fill_buffer()
{

    int im4;

    double avg = 0.0, integral;

    double inc, signal;

    int i;

    // fill with random numbers up to 12 bit DAC
    for (i = 0; i < BUFF_SIZE; i++) {
        double r = (double)(rand() % 10000) - 10000.0 / 2.0;
        dbuff[i] = 10.0 * r * NOISE_LEVEL / (10000.0 / 2.0);
        //buffer[i] += 2 * sin((i % 4) * (TWO_PI / 4));

	dbuff[i] += DC_TERM;
        printf(" iteration %d rand_plus_dc %lf ", i, dbuff[i]);

	// Note the sine wave (and the rand noise) aremultiplied by ten.
	// ten is the amplitude of the signal we are wishing to isolate
	//
	printf(" signal %lf ", signal = 10.0 * sin((i % 4) * (TWO_PI / 4)));
        dbuff[i] += signal;
        printf(" plus_sig %lf ",  dbuff[i]);

        // Apply a no dc filter to the buffer
	dbuff[i] = no_dc_filter ( dbuff[i] );
        printf(" no_dc %lf ", dbuff[i]);

        // perhpas, as ADC RAW would simply have a d.c. level

        // OK here it is known the sinal sarts at 0 and is four samples long.
        //
        // So as mod 4
        // 0 * 1
        // 1 * 1
        // 2 * -1
        // 3 * -1
        //
        // This should provide a rectified 
        // lockin to the buffer

        // assume the buffer is already filled
        // This is the lock in filter
        im4 = i % 4;

        if (im4 < 2)            // if phase correct should be all rectified
            inc = dbuff[i];
        else
            inc -= dbuff[i];

        increment_buffer[i % INCREMENT_BUFFER_SIZE] = inc;      // integrate last 100
        integral = 0.0;
        for (int j = 0; j < INCREMENT_BUFFER_SIZE; j++)
            integral += increment_buffer[j];
        integral /= INCREMENT_BUFFER_SIZE;

	// the integral here is a rolling average of the lock in analog multiplied values
	// the more/larger the buffer the less noise affects it

	// now lp filter the result
	//
        level = 0.95 * level + 0.05 * integral;   // first order lp
        //ff_level = dc_pass_filter (level);
        if ( !(i%100))  {
		f_level = 0.98 * f_level + 0.02 * level;  // first order lp
    	}
        printf(" inc %lf level %lf f_level %lf ", inc, level, f_level);
        //printf(" inc %lf level %lf f_level %lf ff_level %lf", inc, level, f_level, ff_level);
	printf("\n");
        avg += dbuff[i];
    }
    printf("# avg of buffer (d.c. term) was %lf", avg / BUFF_SIZE);

}

int main()
{

    static int64_t z_1, z_2;

    fill_buffer();

//    lock_in(); // now in fill_buffer if all in same loop then printf output to gnuplot possible

    return 0;

    // Z transform for a middle NOTCH
    // H(Z) = 1/( (z-i.k)(z+ik) )
    //
    // where K = 1.0 -1/1000
    // This should give a gain of approx 1 million to
    // the desired frequency
    //
    // Z^0 = 1
    // Z^1 = -2.(0.999)   : 1.998
    // Z^2 = (0.999)^2    : 0.998001
    //
    //
    //
    double d1, d2;
    d1 = 0.999;
    d2 = d1 * d1;

    p_1 = (int64_t) 2.0 *d1 * TWO_T_32;
    p_2 = (int64_t) d2 *TWO_T_32;

    printf(" p1 0x%016lx  \n", p_1);
    printf(" p2 0x%016lx  \n", p_2);
//      printf("%" PRId64 "\n", p_1);
//      printf(" p_1 %I64d  p_2 %I64d \n", p_1,p_2 );
//

    // Apply notch filter
    //
    int64_t res;
/*
    for (int i = 0; i < BUFF_SIZE; i++) {
	    // additions peformed in 64 bit
        res = (buffer[i]<<32) - p_1 * z_1 + p_2 * z_2;
        z_2 = z_1;
        z_1 = res>>32;
    printf(" res 0x%016llx \n", res      );
    }
*/
    double rd, yd_1, yd_2;
    double xd, xd_1, xd_2;
    // now perform the filter in double precision
    //
    for (int i = 0; i < BUFF_SIZE; i++) {
        // a double pole at nyquist (z+i)(z-i)
        xd = dbuff[i];
        rd = xd + ((1 - 0.001) * (1 - 0.001)) * yd_2
            // double zero at d.c.
            - xd_2;
        ;

        yd_2 = yd_1;
        yd_1 = rd;

        xd_2 = xd_1;
        xd_1 = xd;

        printf(" res %lf \n ", rd);
    }
}
