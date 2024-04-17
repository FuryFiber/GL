#include "math.h"


/*
 * IIR filter implementation with arbitrary order
 */
template <int ORDER>
struct IIR {
    float bCoef[ORDER+1] = {};  // feedforward
    float aCoef[ORDER] = {};    // feedback
    float xCoef[ORDER];         // input history
    float yCoef[ORDER];         // output history

    IIR() {
        for (int i = 1; i < ORDER+1; i++) {
            xCoef[i - 1] = 0.f;
        }
        for (int i = 1; i < ORDER+1; i++) {
            yCoef[i - 1] = 0.f;
        }
    }

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
    // Set IIR filter coefficients for it to act as a low pass filter with desired cutoff frequency
    void setParametersLow(float cutoff, float quality){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + K / quality + K * K);
        this->bCoef[0] = K * K * norm;
        this->bCoef[1] = 2.f * this->bCoef[0];
        this->bCoef[2] = this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - K / quality + K * K) * norm;
    }
    void setParametersBand(float cutoff, float quality){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + K / quality + K * K);
        this->bCoef[0] = K / quality * norm;
        this->bCoef[1] = 0.f;
        this->bCoef[2] = -this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - K / quality + K * K) * norm;
    }
    void setParametersHigh(float cutoff, float quality){
        float K = tan(M_PI * cutoff);
        float norm = 1.f / (1.f + K / quality + K * K);
        this->bCoef[0] = norm;
        this->bCoef[1] = -2.f * this->bCoef[0];
        this->bCoef[2] = this->bCoef[0];
        this->aCoef[0] = 2.f * (K * K - 1.f) * norm;
        this->aCoef[1] = (1.f - K / quality + K * K) * norm;
    }
};


struct Cascade6PButterFilter{
    Biquad filters[3];
public:
    void setCutoffLow(float cutoff){
        filters[0].setParametersLow(cutoff, .51763809);
        filters[1].setParametersLow(cutoff, 0.70710678);
        filters[2].setParametersLow(cutoff, 1.9318517);
    }
    void setCutoffBand(float cutoff){
        filters[0].setParametersBand(cutoff, .51763809);
        filters[1].setParametersBand(cutoff, 0.70710678);
        filters[2].setParametersBand(cutoff, 1.9318517);
    }
    void setCutoffHigh(float cutoff){
        filters[0].setParametersHigh(cutoff, .51763809);
        filters[1].setParametersHigh(cutoff, 0.70710678);
        filters[2].setParametersHigh(cutoff, 1.9318517);
    }
    float process(float in){
        float out = filters[0].process(in);
        out = filters[1].process(out);
        out = filters[2].process(out);

        return out;
    }
};

template <int ORDER>
struct FIR {
    float coefs[ORDER+1] = {};
    float buffer[ORDER+1] = {};
    int index = 0;
public:
    float process(float in) {
        float out = 0;
        buffer[index] = in;

        for (int i = 0; i<=ORDER; i++){
            out += coefs[i] * buffer[(index+order-i) % (ORDER + 1)];
        }
        index = (index + 1) % (ORDER + 1)
    }
};

template <int ORDER>
struct VariableCutoffFIRFilter : FIR<ORDER> {
    void setParametersLow(float cutoff, float samplerate) {
        for (int n = 0; n <= ORDER; n++){
            if (n==ORDER/2){
                this->coefs[n] = 2 * cutoff/samplerate;
            }
            else {
                this->coefs[n] = sin(2 * M_PI * cutoff / samplerate * (n - ORDER / 2)) / (M_PI * (n - ORDER / 2));
            }
        }
        for (int n = 0; n <= ORDER; n++) {
            this->coefs[n] *= 0.54f - 0.46f * cos(2 * M_PI * n / ORDER);
        }
    }
    void setParametersHigh(float cutoff, float samplerate) {
        for (int n = 0; n <= ORDER; n++){
            if (n==ORDER/2){
                this->coefs[n] = 1.f - 2.f * cutoff/samplerate;
            }
            else {
                this->coefs[n] = -sin(2.f * M_PI * cutoff / samplerate * (n - ORDER / 2.f)) / (M_PI * (n - ORDER / 2.f));
            }
        }
        for (int n = 0; n <= ORDER; n++) {
            this->coefs[n] *= 0.54f - 0.46f * cos(2 * M_PI * n / ORDER);
        }
    }
};