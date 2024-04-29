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

double dbuff[BUFF_SIZE], level, f_level, ff_level;




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



// part of the lock in filter is the integral, the lovck in rectification/amplification
// only becomes meaningfull when a large number of noise samples can cancel out each other
#define INCREMENT_BUFFER_SIZE 2000
double increment_buffer[INCREMENT_BUFFER_SIZE];

void test_lock_in ()
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
       
	printf("\n");
        avg += dbuff[i];
    }
    printf("# avg of buffer (d.c. term) was %lf", avg / BUFF_SIZE);

}

int main()
{

    test_lock_in();

    return 0;
}
