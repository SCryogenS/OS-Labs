#include "CryoLib.h"
#include <math.h>

long double powed(long double base, unsigned int exp){
    long double value = 1.0L;

    while (exp > 0){
        if (exp % 2 != 0){value *= base;}
        exp /= 2;
        if (exp > 0){base *= base;}
    }

    return value;
}

int Evklid_gcd(int a, int b){
    if (a <= 0 || b <= 0){return 0;}

    int left_part;
    int right_part;
    int divider;
    if (a > b){left_part = a; right_part = b;}else{left_part = b; right_part = a;}
    while (left_part > right_part){
        divider = left_part - right_part;
        if (divider < right_part){left_part = right_part; right_part = divider;}else{left_part = divider;}
    }

    return left_part;
}

int fool_gcd(int a, int b){
    if (a <= 0 || b <= 0){return 0;}

    int min;
    int max_divider = 1;
    if (a > b){min = b;}else{min = a;}
    for (long long i = 1; i <= min; i++){
        if (a % i == 0 && b % i == 0){max_divider = (int)i;}
    }

    return max_divider;
}

float lim_e(int x){
    if (x <= 0){return NAN;}

    return (float)powed(1.0L + 1.0L / x, x);
}

float row_e(int x){
    if (x <= 0){return NAN;}

    double row_result = 1.0;
    double term = 1.0;
    for (int n = 1; n <= x; n++){
        term /= n;
        double next = row_result + term;
        if (next == row_result){break;}
        row_result = next;
    }

    return (float)row_result;
}

#ifdef CRYOLIB_SECOND
int gcd(int a, int b){return fool_gcd(a, b);}
float e(int x){return row_e(x);}
#else
int gcd(int a, int b){return Evklid_gcd(a, b);}
float e(int x){return lim_e(x);}
#endif
