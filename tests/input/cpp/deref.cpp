
myNewValue = something[arrayNumber] *someOtherValue;
myNewValue = multidimensional[arrayNumber][anotherNumber] *someOtherValue;
myNewValue = noArrayVariableWorksFine * someOtherValue;


int func(int * thingy,
volatile int *arrayThingy[NUMBER]);

int func(int * thingy,
volatile int *arrayThingy[NUMBER][anotherNumber]);

int func(int * thingy,
volatile int *noArrayThingyWorksFine);

