#ifndef TINYML_H
#define TINYML_H

/**
 * Linear Regression Model: y = (w * x) + b
 * Προβλέπει την επόμενη τιμή βάσει της τρέχουσας.
 */
static inline float ml_predict_next_temp(float current_temp) {
    const float weight = 1.045f;
    const float bias   = 1.120f;
    return (current_temp * weight) + bias;
}

#endif