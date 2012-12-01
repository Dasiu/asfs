#include "asfs.h"

int main() {
    runServer();
    
    pthread_exit(NULL);
    return 0;
}