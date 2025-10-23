#include "circuit.hpp"
#include <stdio.h>
#include <stdlib.h>

Circuit::Circuit() {}

Circuit::Circuit(unsigned int nN, unsigned int nV, unsigned int nR, unsigned int nI)
    : nN(nN), nV(nV), nR(nR), nI(nI) {}

Circuit Circuit::createFromFile(const char *filename) {
    Circuit c = Circuit(0, 0, 0, 0);
	FILE *fPtr;
	Component p;
	
	/* Try to open the file */
	fPtr = fopen(filename, "r");
	if (!fPtr) { fprintf(stderr, "Could not open file: %s\n", filename); exit(EXIT_FAILURE); }

    unsigned int nC = 0;

	/* Read the netlist to find out how many components there are and dynamically allocate the memory */
	while(fscanf(fPtr, "%s %u %u %lf", p.name, &p.n1, &p.n2, &p.value) == 4) nC++; 

    c.comp.reserve(nC);
    c.comp.resize(nC);

	/* Do another pass to read in the components */
	rewind(fPtr);
		
	for(nC=0; fscanf(fPtr, "%s %u %u %lf", c.comp[nC].name, &c.comp[nC].n1, &c.comp[nC].n2, &c.comp[nC].value) == 4; nC++) {

		switch(c.comp[nC].name[0]) {
			case 'R': c.nR++; c.comp[nC].type = resistor; break;
			case 'V': c.nV++; c.comp[nC].type = voltage; break;
			case 'I': c.nI++; c.comp[nC].type = current; break;
			default: fprintf(stderr, "Unknown component on line %u in file %s.\n", nC + 1, filename); exit(EXIT_FAILURE);
		}
		if (c.comp[nC].n1 > c.nN) c.nN = c.comp[nC].n1;
		if (c.comp[nC].n2 > c.nN) c.nN = c.comp[nC].n2;
	}

	c.nN++; /* Node labelling is zero based so add one to get total number of nodes. */
	fclose(fPtr);
	return c;
}

void Circuit::analyseCircuit() {
    unsigned int n1, n2, i, cV;
	double value, g;
	
	/* Initialise matrices and vectors and zero elements */
	Matrix A(0.0, nN + nV, nN + nV);
	Vector Z(0.0, nN + nV), X(0.0, nN + nV);

	/* Build nodal analysis equations */
	for(i=0, cV=0; i<comp.size(); i++) {
		n1 = comp[i].n1;
		n2 = comp[i].n2;
		value = comp[i].value;
		switch(comp[i].type) {
			case resistor: 
				g = 1.0/value;
                A(n1, n2) -= g;
				A(n2, n1) -= g;
				A(n1, n1) += g;
				A(n2, n2) += g;	
			break;
			case voltage: 
				A(n1, cV + nN) = A(cV + nN, n1) = 1.0;                
				A(n2, cV + nN) = A(cV + nN, n2) = -1.0;
				Z[cV + nN] = -value;
				cV++;
			break;
			case current: 
				Z[n1] -= value;
				Z[n2] += value;
			break;
		}
	}
	
	/* Node 0 is ground and is treated differently */
	A(0, 0) = 1.0; 
	Z[0] = 0.0;
	for(i=1; i<nN + nV; i++) 
		A(0, i) = A(i, 0) = 0.0;

	// printMatrix(A);
	// printVector(Z);

	/* Analyse and display results */
	X = solveLinearSystem(A, Z);
	printf("----------------------------\n");
	printf(" Voltage sources: %u\n", nV);
	printf(" Current sources: %u\n", nI);
	printf("       Resistors: %u\n", nR);
	printf("           Nodes: %u\n", nN);
	printf("----------------------------\n");
	for(i=0; i<nN; i++)
		printf(" Node %3u = %10.6lf V\n", i, X[i]);
	printf("----------------------------\n");
	if (nV) {
		for(i=0, cV=0; i<comp.size(); i++)
			if (comp[i].type == voltage)
				printf(" I(%s)    = %10.6lf A\n", comp[i].name, X[nN + cV++]);
		printf("----------------------------\n");
	}
}

Vector solveLinearSystem(Matrix A, Vector b)
{
	/* Note: This function overwrites the contents of A and b. */
	int i, j, k;
	double mult, sum;
	Vector x(0.0, A.col_size());

	/* reduce to upper triangular form */
	for (i=0; i<A.row_size()-1; i++)
		for (j=i+1; j<A.row_size(); j++) {   
			mult = -A(j, i)/A(i, i);
			for (k=i; k<A.col_size(); k++)
				A(j, k) += mult*A(i, k);
			b[j] += mult*b[i];
		}
	/* back substitute to find Vector x */
	for (i=A.row_size()-1; i>=0; i--) {
		for (j=i+1, sum = 0.0; j<A.col_size(); j++)
			sum += A(i, j) * x[j];
		x[i] = (b[i] - sum)/A(i, i);
	}
	return x;
}