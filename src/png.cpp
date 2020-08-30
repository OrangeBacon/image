#include "png.hpp"

uint16_t clamp(double val) {
    return static_cast<uint16_t>(std::round(val * UINT16_MAX));
}

// http://www.splinter.com.au/converting-hsv-to-rgb-colour-using-c/
Pixel::Pixel(double H, double S, double V) : a(0) {
    while (H < 0) { H += 360; }
    while (H >= 360) { H -= 360; }

    double R;
    double G;
    double B;

    if (V <= 0) {
        R = G = B = 0;
    } else if (S <= 0) {
        R = G = B = V;
    } else {
        double hf = H / 60.0;
        int i = static_cast<int>(std::floor(hf));
        double f = hf - i;
        double pv = V * (1 - S);
        double qv = V * (1 - S * f);
        double tv = V * (1 - S * (1 - f));

        switch (i) {
            // mainly red
        case 6:
        case 0:
            R = V;
            G = tv;
            B = pv;
            break;

            // mainly green
        case 1:
            R = qv;
            G = V;
            B = pv;
            break;
        case 2:
            R = pv;
            G = V;
            B = tv;
            break;

            // mainly blue
        case 3:
            R = pv;
            G = qv;
            B = V;
            break;
        case 4:
            R = tv;
            G = pv;
            B = V;
            break;

            // mainly red (again)
        case 5:
        case -1:
            R = V;
            G = pv;
            B = qv;
            break;

            // if error
        default:
            R = G = B = V; // greyscale
        }
    }

    r = clamp(R);
    g = clamp(G);
    b = clamp(B);
}
