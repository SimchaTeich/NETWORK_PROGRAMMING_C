#include <stdio.h>
#include <poll.h>

int main()
{
    struct pollfd pfds[1]; // More if you want to monitor more

    pfds[0].fd = 0;          // Standard input
    pfds[0].events = POLLIN; // Tell me when ready to read

    printf("Hit RETURN or wait 2.5 seconds for timeout\n");

    int num_events = poll(pfds, 1, 2500) // 2.5 seconds timout

    if(num_events == 0)
    {
        printf("Poll time out!\n");
    }
    else
    {
        int pollin_happened = pfds[0].revents & POLLIN;

        if(pollin_happened)
        {
            printf("file descriptor %d is ready to read\n", pfds[0].fd);
        }
        else
        {
            pritnf("Unexpected event occurred: %d\n", pfds[0].revents);
        }
    }

    return 0;
}