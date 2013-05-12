#include <iostream>
#include <cmath>
#include <math.h>
using namespace std;

double sampleData[] = {1024, 0, 0, 0, 0, 0, 0, 0};

double* dct2(double* raw, int n, double scale);
double* dct3(double* raw, int n, double scale);
double rnd(double n);
void printVector(double* raw, int n);

int main(int argc, char** argv)
{
	int n = sizeof(sampleData)/sizeof(double);

	cout << "Sample Input Data: " << endl;
	printVector(sampleData, n);

	double* encoded = dct2(sampleData, n, 1.0);
	cout << "DCT-II:" << endl;
	printVector(encoded, n);

	double* decoded = dct3(encoded, n, 2.0 / n);
	cout << "DCT-III with scalefactor 2/n:" << endl;
	printVector(decoded, n);	

	delete[] encoded;
	delete[] decoded;
	return 0;
}

double* dct2(double* raw, int n, double scale)
{
	if (n <= 2) {
		return NULL;
	}

	double* result = new double[n];

	for (int f = 0; f < n; f++) {
		double c = 0.0;
		for (int t = 0; t < n; t++) {
			c += raw[t] * cos( (M_PI / n) * (t + 0.5) * f );
		}
		result[f] = scale * c;
	}

	return result;
}

double* dct3(double* raw, int n, double scale)
{
	if (n <= 2) {
		return NULL;
	}

	double* result = new double[n];

	for (int f = 0; f < n; f++) {
		double c = 0.5 * raw[0];
		for (int t = 1; t < n; t++) {
			c += raw[t] * cos( (M_PI / n) * (f + 0.5) * t);
		}
		result[f] = scale * c;
	}

	return result;
}

double rnd(double n)
{
	n = round(n*1000000)/1000000;
	if ( n == -0) {
		return 0;
	}
	return n;
}

void printVector(double* raw, int n)
{
	for (int i = 0; i < n; i++) {
		if ( i == n - 1 ) {
			cout << rnd(raw[i]) << endl;
		} else {
			cout << rnd(raw[i]) << ", ";
		}
	}
}
