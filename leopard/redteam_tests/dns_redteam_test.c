/*
 * CVE-2008-1447 DNS Port Randomization Red Team Test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int sockets[10];
    int ports[10];
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int i, sequential = 1;
    
    printf("=== CVE-2008-1447 DNS Port Randomization Test ===\n\n");
    
    for (i = 0; i < 10; i++) {
        sockets[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockets[i] < 0) { perror("socket"); return 1; }
        
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = 0;
        
        if (bind(sockets[i], (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind"); return 1;
        }
        
        getsockname(sockets[i], (struct sockaddr*)&addr, &len);
        ports[i] = ntohs(addr.sin_port);
    }
    
    printf("Assigned ports: ");
    for (i = 0; i < 10; i++) printf("%d ", ports[i]);
    printf("\n\n");
    
    for (i = 1; i < 10; i++) {
        if (ports[i] != ports[i-1] + 1) {
            sequential = 0;
            break;
        }
    }
    
    if (sequential) {
        printf("RESULT: Sequential ports detected - VULNERABLE\n");
    } else {
        printf("RESULT: Random ports detected - MITIGATED\n");
    }
    
    for (i = 0; i < 10; i++) close(sockets[i]);
    return 0;
}
