#include <stdio.h>
#include <string.h>

#define NUM_APPS 4

typedef struct {
    char name[20];
    int memory;      // simulated memory in MB
    int status;      // 0 = CLOSED, 1 = OPEN
} App;

App apps[NUM_APPS] = {
    {"Chrome", 520, 0},
    {"YouTube", 650, 0},
    {"Maps", 450, 0},
    {"Calculator", 120, 0}
};

void showMenu() {
    printf("\n===== ANDROID OS SIMULATION =====\n");
    printf("1. Open Application\n");
    printf("2. Close Application\n");
    printf("3. Task Manager\n");
    printf("4. Shutdown\n");
    printf("Enter your choice: ");
}

void listApps() {
    printf("\nApplications:\n");
    for (int i = 0; i < NUM_APPS; i++) {
        printf("%d. %s\n", i + 1, apps[i].name);
    }
}

void openApp() {
    listApps();
    int choice;
    printf("Select an app to open: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > NUM_APPS) {
        printf("Invalid choice.\n");
        return;
    }

    App *a = &apps[choice - 1];
    if (a->status == 1) {
        printf("%s is already OPEN.\n", a->name);
    } else {
        a->status = 1;
        printf("%s opened. Memory allocated: %d MB\n", a->name, a->memory);
    }
}

void closeApp() {
    listApps();
    int choice;
    printf("Select an app to close: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > NUM_APPS) {
        printf("Invalid choice.\n");
        return;
    }

    App *a = &apps[choice - 1];
    if (a->status == 0) {
        printf("%s is already CLOSED.\n", a->name);
    } else {
        a->status = 0;
        printf("%s closed. Memory released: %d MB\n", a->name, a->memory);
    }
}

void taskManager() {
    int totalMemory = 0;
    printf("\n----- TASK MANAGER -----\n");
    printf("%-12s %-8s %-10s\n", "App", "Status", "Memory");
    for (int i = 0; i < NUM_APPS; i++) {
        printf("%-12s %-8s %-10d\n",
               apps[i].name,
               apps[i].status ? "OPEN" : "CLOSED",
               apps[i].status ? apps[i].memory : 0);
        if (apps[i].status)
            totalMemory += apps[i].memory;
    }
    printf("------------------------\n");
    printf("Total memory in use: %d MB\n", totalMemory);
}

void shutdown() {
    printf("\nShutting down...\n");
    for (int i = 0; i < NUM_APPS; i++) {
        if (apps[i].status == 1) {
            apps[i].status = 0;
            printf("%s closed. Memory released: %d MB\n", apps[i].name, apps[i].memory);
        }
    }
    printf("System shutdown complete.\n");
}

int main() {
    int choice;
    int running = 1;

    while (running) {
        showMenu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                openApp();
                break;
            case 2:
                closeApp();
                break;
            case 3:
                taskManager();
                break;
            case 4:
                shutdown();
                running = 0;
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}
