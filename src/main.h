#ifndef MAIN_H
#define MAIN_H

int main(int argc, const char *argv[]);

void main_handleSignalSIGINT(int signalNumber);
void main_handleSignalSIGTERM(int signalNumber);
void main_emptySignalHandler(int signalNumber);

#endif
