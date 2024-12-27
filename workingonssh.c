// gcc thisisit.c -o thisisit -lssh

#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error_handling(ssh_session session, const char *msg) {
    fprintf(stderr, "%s: %s\n", msg, ssh_get_error(session));
    ssh_free(session);
    exit(EXIT_FAILURE);
}

int main() {
    const char *host = "<remote ip>"; //ip
    int port = 51745;   //port
    const char *username = "endureeyes"; //username
    const char *password = "endureeyes"; //password

    ssh_session session = ssh_new();    // Create a new SSH session
    if (session == NULL) {
        fprintf(stderr, "Error creating SSH session\n");
        exit(EXIT_FAILURE);
    }

    // Set SSH options
    ssh_options_set(session, SSH_OPTIONS_HOST, host);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, username);

    // Connect to the SSH server
    if (ssh_connect(session) != SSH_OK) {
        error_handling(session, "Error connecting to SSH server");
    }

    // Authenticate with password
    if (ssh_userauth_password(session, NULL, password) != SSH_AUTH_SUCCESS) {
        error_handling(session, "Authentication failed");
    }

    // Create and execute a remote shell
    ssh_channel channel = ssh_channel_new(session);
    if (channel == NULL) {
        error_handling(session, "Error creating channel");
    }

    if (ssh_channel_open_session(channel) != SSH_OK) {
        error_handling(session, "Error opening channel");
    }

    if (ssh_channel_request_shell(channel) != SSH_OK) {
        error_handling(session, "Error requesting shell");
    }

    char buffer[256];
    int nbytes;

    // Read initial prompt
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {   //the last 0 is for if we want to read from stderr
        buffer[nbytes] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "1000") != NULL) break;  // Wait for full prompt
    }

    // Main interaction loop
    char guess[16];
    int low = 1, high = 1000;
    while (1) {
        int mid = (low + high) / 2;
        snprintf(guess, sizeof(guess), "%d\n", mid);

        // Send guess
        if (ssh_channel_write(channel, guess, strlen(guess)) <= 0) {    //write
            fprintf(stderr, "Error sending guess\n");
            break;
        }
        printf("%s\n",guess);

        // Read response
        memset(buffer, 0, sizeof(buffer));
        while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {   //read
            buffer[nbytes] = '\0';
            printf("%s", buffer);

            // Update search range based on response
            if (strstr(buffer, "Higher") != NULL) {
                low = mid + 1;
                break;
            } else if (strstr(buffer, "Lower") != NULL) {
                high = mid - 1;
                break;
            } else if (strstr(buffer, "Correct") != NULL || strstr(buffer, "flag") != NULL) {
                goto done;  // Exit both loops when we find the flag
            }
        }
    }

    done:
    // Cleanup
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    return 0;
}
