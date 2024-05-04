#include "math.h"


/*
 * IIR convolution function implementation with arbitrary order
 */
template <int ORDER>
struct IIR {
    float bCoef[ORDER+1] = {};  // feedforward
    float aCoef[ORDER] = {};    // feedback
    float xCoef[ORDER];         // input history
    float yCoef[ORDER];         // output history

    /*
     * Constrcutor resets all arrays to contain 0's
     */
    IIR() {
        for (int i = 1; i < ORDER+1; i++) {
            xCoef[i - 1] = 0.f;
        }
        for (int i = 1; i < ORDER+1; i++) {
            yCoef[i - 1] = 0.f;
        }
    }

    /*
     * Process a single sample
     */
    float process(float in) {
        float out = 0.f;
        // Add x state
        if (0 < ORDER+1) {
            out = bCoef[0] * in;
        }
        for (int i = 1; i < ORDER+1; i++) {
            out += bCoef[i] * xCoef[i - 1];
        }
        // Subtract y state
        for (int i = 1; i < ORDER+1; i++) {
            out -= aCoef[i - 1] * yCoef[i - 1];
        }
        // Shift x state
        for (int i = ORDER; i >= 2; i--) {
            xCoef[i - 1] = xCoef[i - 2];
        }
        xCoef[0] = in;
        // Shift y state
        for (int i = ORDER; i >= 2; i--) {
            yCoef[i - 1] = yCoef[i - 2];
        }
        yCoef[0] = out;
        return out;
    }
};

/*
 * Biquad filter implementation
 * IIR filter of order 2
 */
struct Biquad : IIR<2> {
public:
    /*
     * Set IIR filter coefficients for it to act as a low pass filter with desired cutoff frequency
     */
    void setParametersLow(float cutoff){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + sqrt(2)*K + K * K);
        this->bCoef[0] = K * K * norm;
        this->bCoef[1] = 2.f * this->bCoef[0];
        this->bCoef[2] = this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - sqrt(2)*K + K * K) * norm;
    }
    /*
     * Set IIR filter coefficients for it to act as a band pass filter with desired cutoff frequency
     */
    void setParametersBand(float cutoff, float quality){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + K / quality + K * K);
        this->bCoef[0] = K / quality * norm;
        this->bCoef[1] = 0.f;
        this->bCoef[2] = -this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - K / quality + K * K) * norm;
    }
    /*
     * Set IIR filter coefficients for it to act as a high pass filter with desired cutoff frequency
     */
    void setParametersHigh(float cutoff){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + sqrt(2) * K + K * K);
        this->bCoef[0] = norm;
        this->bCoef[1] = -2.f * this->bCoef[0];
        this->bCoef[2] = this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - sqrt(2) * K + K * K) * norm;
    }
};


struct Cascade6PButterFilter{
    Biquad filters[3];
    IIR<2> resonance;
public:
    void setResonance(float cutoff, float T){
        float r = 0.5f;
        resonance.bCoef[0] = (1-r*r)/2;
        resonance.bCoef[1] = 0;
        resonance.bCoef[2] = -resonance.bCoef[0];
        resonance.aCoef[0] = -2 * r * cos(2 * M_PI * cutoff * T);
        resonance.aCoef[1] = r*r;
    }

    /*
     * Set all filter coefficients to lowpass impulse response
     */
    void setCutoffLow(float cutoff){
        filters[0].setParametersLow(cutoff);
        filters[1].setParametersLow(cutoff);
        filters[2].setParametersLow(cutoff);
    }

    /*
     * Set all filter coefficients to bandpass impulse response
     */
    void setCutoffBand(float cutoff){
        filters[0].setParametersBand(cutoff, .51763809);
        filters[1].setParametersBand(cutoff, 0.70710678);
        filters[2].setParametersBand(cutoff, 1.9318517);
    }
    /*
     * Set all filter coefficients to highpass impulse response
     */
    void setCutoffHigh(float cutoff){
        filters[0].setParametersHigh(cutoff);
        filters[1].setParametersHigh(cutoff);
        filters[2].setParametersHigh(cutoff);
    }
    float process(float in){
        float out = filters[0].process(in);
        out = filters[1].process(out);
        out = filters[2].process(out);
        //out += resonance.process(in);

        return out;
    }
};

/*
 * FIR convolution function implementation with arbitrary order
 */
template <int ORDER>
struct FIR {
    float coefs[ORDER] = {};
    float buffer[ORDER] = {};
    int index = 0;
public:

    /*
     * Constructor initializes buffer array to only 0's
     */
    FIR() {
        for (int i =0; i<ORDER; i++){
            buffer[i] = 0;
        }
    }

    /*
     * Process a single sample
     */
    float process(float in) {
        float out = 0;

        // Place sample in buffer
        buffer[index] = in;

        // Update buffer index
        index++;
        if (index == ORDER) {
            index = 0;
        }

        int sum_index = 0;

        // For every item in the buffer perform convolution sum
        for (int i = 0; i<ORDER; i++){
            if (sum_index>0){
                sum_index--;
            }
            else {
                sum_index = ORDER-1;
            }
            out += coefs[i] * buffer[sum_index];
        }

        return out;
    }
};

/*
 * Basic FIR filter implementation using window-sinc method and the hamming window function
 */
template <int ORDER>
struct VariableCutoffFIRFilter : FIR<ORDER> {
    /*
     * Set lowpass coefficients
     */
    void setLowPass(float cutoffFreq, float sampleRate) {
        float wc = 2.f * M_PI*cutoffFreq / sampleRate;
        float M = ORDER/2;

        for (int i=0; i<ORDER; i++){
            if (i==M){
                this->coefs[i] = wc / M_PI;
            }
            else {
                this->coefs[i] = sin(wc * (i-M)) / (M_PI*(i-M));
            }
            this->coefs[i] *= 0.54f - 0.46f * cos(2.f*M_PI*i/ORDER);
        }
    }

    /*
     * Set highpass coefficients
     */
    void setHighpass(float cutoffFreq, float sampleRate) {
        float wc = 2.f * M_PI*cutoffFreq / sampleRate;
        float M = ORDER/2;

        for (int i=0; i<ORDER; i++){
            if (i==M){
                this->coefs[i] = 1.f - wc / M_PI;
            }
            else {
                this->coefs[i] = -sin(wc * (i-M)) / (M_PI*(i-M));
            }
            this->coefs[i] *= 0.54f - 0.46f * cos(2.f*M_PI*i/ORDER);
        }
    }
};