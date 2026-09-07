#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "daemon.h"
#include "cli.h"
#include "tuya_client.h"
#include "telemetry.h"

static volatile sig_atomic_t running = 1;

static void handle_signal(int signal)
{
    if (signal == SIGTERM || signal == SIGINT || signal == SIGQUIT) {
        running = 0;
    }
}

int main(int argc, char **argv)
{
    struct arguments arguments = {0};
    struct sigaction sa;
    int ret;

    openlog("tuya-daemon", LOG_PID | LOG_CONS, LOG_USER);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) != 0 ||
        sigaction(SIGINT, &sa, NULL) != 0 ||
        sigaction(SIGQUIT, &sa, NULL) != 0) {

        syslog(LOG_ERR, "Failed to install signal handlers");
        closelog();
        return EXIT_FAILURE;
    }

    ret = parse_arguments(argc, argv, &arguments);

    if (ret != 0) {
        syslog(LOG_ERR, "Failed to parse arguments");
        closelog();
        return EXIT_FAILURE;
    }

    if (arguments.device_id == NULL ||
        arguments.device_secret == NULL ||
        arguments.product_id == NULL) {

        syslog(LOG_ERR, "Missing required arguments");
        closelog();
        return EXIT_FAILURE;
    }

    if (arguments.daemon) {
        if (daemonize() != 0) {
            syslog(LOG_ERR, "Failed to daemonize");
            closelog();
            return EXIT_FAILURE;
        }

        syslog(LOG_INFO, "Running as daemon");
    }

    ret = tuya_client_init(
        arguments.device_id,
        arguments.device_secret
    );

    if (ret != 0) {
        syslog(LOG_ERR, "Failed to initialize Tuya MQTT: %d", ret);
        closelog();
        return EXIT_FAILURE;
    }

    ret = tuya_client_connect();

    if (ret != 0) {
        syslog(LOG_ERR, "Failed to connect to Tuya Cloud: %d", ret);
        tuya_client_deinit();
        closelog();
        return EXIT_FAILURE;
    }

    while (running) {
        tuya_client_loop();
        telemetry_report();

        if (running) {
            sleep(10);
        }
    }

    tuya_client_deinit();

    syslog(LOG_INFO, "Tuya daemon stopped");

    closelog();

    return EXIT_SUCCESS;
}