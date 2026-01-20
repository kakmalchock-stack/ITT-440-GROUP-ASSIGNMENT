// /c-server/server.c (FULL FIXED VERSION)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

// --- Constants and Environment Variables ---
#define SERVER_USER "c_user-akmal"
#define UPDATE_INTERVAL 30 // seconds
#define LOG_PREFIX "[c_user-akmal Server]"

// Global buffers for DB connection string parts
char DB_HOST[50];
char DB_USER[50];
char DB_PASS[50];
char DB_NAME[50];
int SERVER_PORT;

// --- Utility Functions ---
void get_env_vars() {
    strncpy(DB_HOST, getenv("DB_HOST") ? getenv("DB_HOST") : "database", sizeof(DB_HOST) - 1);
    strncpy(DB_USER, getenv("DB_USER") ? getenv("DB_USER") : "root", sizeof(DB_USER) - 1);
    strncpy(DB_PASS, getenv("DB_PASS") ? getenv("DB_PASS") : "root@12345", sizeof(DB_PASS) - 1);
    strncpy(DB_NAME, getenv("DB_NAME") ? getenv("DB_NAME") : "project_db", sizeof(DB_NAME) - 1);
    DB_HOST[sizeof(DB_HOST)-1] = '\0';
    DB_USER[sizeof(DB_USER)-1] = '\0';
    DB_PASS[sizeof(DB_PASS)-1] = '\0';
    DB_NAME[sizeof(DB_NAME)-1] = '\0';

    SERVER_PORT = getenv("SERVER_PORT") ? atoi(getenv("SERVER_PORT")) : 5005;
}

// Execute a SQL command using system()
int execute_sql(const char *sql_query) {
    char command[1024];
    snprintf(command, sizeof(command),
             "mysql -h%s -u%s --password=%s -D%s -e \"%s\" > /dev/null 2>&1",
             DB_HOST, DB_USER, DB_PASS, DB_NAME, sql_query);
    return system(command);
}

// Fetch points and timestamp from DB (safe parsing)
int fetch_user_data(int *points, char *timestamp_str, size_t size) {
    char command[1024];
    snprintf(command, sizeof(command),
             "mysql -N -h%s -u%s --password=%s -D%s -e \"SELECT points, datetime_stamp FROM users WHERE user='%s' LIMIT 1;\" > /tmp/c_server_data.txt 2>/dev/null",
             DB_HOST, DB_USER, DB_PASS, DB_NAME, SERVER_USER);

    if (system(command) != 0) return -1;

    FILE *fp = fopen("/tmp/c_server_data.txt", "r");
    if (!fp) return -1;

    char raw[64];
    if (fgets(raw, sizeof(raw), fp) != NULL) {
        fclose(fp);

        // Strip trailing whitespace
        size_t len = strlen(raw);
        while (len > 0 && (raw[len-1]=='\n' || raw[len-1]=='\r' || raw[len-1]==' ' || raw[len-1]=='\t')) {
            raw[--len]='\0';
        }

        char *tab = strchr(raw, '\t');
        if (!tab) return -1;

        *tab = '\0';
        *points = atoi(raw);
        char *ts = tab+1;

        struct tm tm_time;
        if (strptime(ts, "%Y-%m-%d %H:%M:%S", &tm_time) == NULL) return -1;
        strftime(timestamp_str, size, "%Y-%m-%d %H:%M:%S", &tm_time);

        return 0;
    }

    fclose(fp);
    return -1;
}

// --- Background Task: Update points every 30 seconds ---
void *database_update_task(void *arg) {
    printf("%s Background update task started...\n", LOG_PREFIX);

    sleep(10); // Wait for DB startup

    const char *update_sql = "UPDATE users SET points = points + 1, datetime_stamp = NOW() WHERE user='" SERVER_USER "';";
    const char *connect_sql = "SELECT 1;";

    int MAX_RETRIES = 10;
    int RETRY_DELAY = 3;

    for (int attempt=0; attempt<MAX_RETRIES; attempt++) {
        if (execute_sql(connect_sql) == 0) {
            printf("%s Database connection established successfully.\n", LOG_PREFIX);
            goto success;
        } else {
            printf("%s DB connection failed (Attempt %d/%d). Retrying in %d seconds...\n", LOG_PREFIX, attempt+1, MAX_RETRIES, RETRY_DELAY);
            sleep(RETRY_DELAY);
        }
    }
    fprintf(stderr, "%s CRITICAL: Cannot connect to DB. Exiting update thread.\n", LOG_PREFIX);
    return NULL;

success:;
    while (1) {
        if (execute_sql(update_sql) == 0) {
            printf("%s DB updated: Points incremented.\n", LOG_PREFIX);
        } else {
            fprintf(stderr, "%s ERROR: Failed DB update.\n", LOG_PREFIX);
        }
        sleep(UPDATE_INTERVAL);
    }
    return NULL;
}

// --- Socket Server ---
int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;
    socklen_t addrlen = sizeof(addr);
    char buffer[1024] = {0};
    pthread_t thread_id;

    setbuf(stdout, NULL); // immediate logs

    get_env_vars();
    printf("%s Starting C Server on port %d...\n", LOG_PREFIX, SERVER_PORT);

    // Start background update thread
    if (pthread_create(&thread_id, NULL, database_update_task, NULL) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    // Socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { perror("socket"); exit(EXIT_FAILURE); }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR|SO_REUSEPORT, &opt, sizeof(opt))) { perror("setsockopt"); exit(EXIT_FAILURE); }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(server_fd, 3) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    printf("%s Listening for clients on 0.0.0.0:%d...\n", LOG_PREFIX, SERVER_PORT);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("%s Connection established from %s:%d\n", LOG_PREFIX, client_ip, ntohs(addr.sin_port));

        long valread = read(client_fd, buffer, sizeof(buffer)-1);
        buffer[valread] = '\0';
        printf("%s Received: %s\n", LOG_PREFIX, buffer);

        int pts = 0;
        char ts[32] = "N/A";

        if (fetch_user_data(&pts, ts, sizeof(ts)) == 0) {
            printf("%s Database connection established successfully.\n", LOG_PREFIX);
        } else {
            printf("%s ERROR: Failed fetching DB data.\n", LOG_PREFIX);
        }

        char response[256];
        snprintf(response, sizeof(response), "User: %s, Points: %d, Last Update: %s\n", SERVER_USER, pts, ts);
        send(client_fd, response, strlen(response), 0);
        printf("%s Sent: %s", LOG_PREFIX, response);

        close(client_fd);
        memset(buffer, 0, sizeof(buffer));
    }

    close(server_fd);
    return 0;
}
