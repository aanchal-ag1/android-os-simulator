#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_APPS 10
#define MAX_FILES 20
#define MAX_MEMORY_BLOCKS 20
#define MAX_QUEUE 20
#define RAM_TOTAL 1024

typedef enum { NEW, READY, RUNNING, PAUSED, BACKGROUND, TERMINATED } State;

typedef struct {
    int pid;
    char name[32];
    int memory;
    int priority;
    int burst;
    int remaining;
    State state;
    int camera, storage, internet, location;
} App;

typedef struct {
    char name[40];
    int size;
    int ownerPid;
} SimFile;

typedef struct {
    int start;
    int size;
    int pid;
} MemoryBlock;

App apps[MAX_APPS];
SimFile files[MAX_FILES];
MemoryBlock mem[MAX_MEMORY_BLOCKS];
int appCount = 0, fileCount = 0, memCount = 0, nextPid = 1001;
int ramUsed = 0;

const char *stateName(State s) {
    switch (s) {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case PAUSED: return "PAUSED";
        case BACKGROUND: return "BACKGROUND";
        case TERMINATED: return "TERMINATED";
    }
    return "UNKNOWN";
}

int findApp(int pid) {
    for (int i = 0; i < appCount; i++)
        if (apps[i].pid == pid && apps[i].state != TERMINATED) return i;
    return -1;
}

void showHeader(const char *title) {
    printf("\n============================================================\n");
    printf("                 ANDROID OS SIMULATOR\n");
    printf("              %s\n", title);
    printf("============================================================\n");
}

void showApps(void) {
    showHeader("Process & Application Manager");
    printf("%-7s %-18s %-12s %-8s %-8s\n", "PID", "APP", "STATE", "MEM(MB)", "PRIORITY");
    printf("------------------------------------------------------------\n");
    int active = 0;
    for (int i = 0; i < appCount; i++) {
        if (apps[i].state == TERMINATED) continue;
        printf("%-7d %-18s %-12s %-8d %-8d\n", apps[i].pid, apps[i].name,
               stateName(apps[i].state), apps[i].memory, apps[i].priority);
        active++;
    }
    if (!active) printf("No active applications.\n");
}

void showMemory(void) {
    showHeader("Memory Manager");
    printf("Total RAM: %d MB | Used: %d MB | Free: %d MB\n", RAM_TOTAL, ramUsed, RAM_TOTAL - ramUsed);
    printf("\n%-10s %-10s %-10s %-18s\n", "START", "SIZE", "PID", "OWNER");
    printf("-----------------------------------------------\n");
    for (int i = 0; i < memCount; i++) {
        int idx = findApp(mem[i].pid);
        printf("%-10d %-10d %-10d %-18s\n", mem[i].start, mem[i].size, mem[i].pid,
               idx >= 0 ? apps[idx].name : "[terminated]");
    }
    if (memCount == 0) printf("No allocated blocks.\n");
    printf("\nAndroid concept simulated: each running application consumes RAM;\n"
           "when memory pressure rises, background processes are candidates for termination.\n");
}

int allocateMemory(int pid, int size) {
    if (ramUsed + size > RAM_TOTAL) return 0;
    int start = 0;
    for (int i = 0; i < memCount; i++) {
        int end = mem[i].start + mem[i].size;
        if (end > start) start = end;
    }
    mem[memCount++] = (MemoryBlock){start, size, pid};
    ramUsed += size;
    return 1;
}

void releaseMemory(int pid) {
    for (int i = 0; i < memCount; i++) {
        if (mem[i].pid == pid) {
            ramUsed -= mem[i].size;
            for (int j = i; j < memCount - 1; j++) mem[j] = mem[j + 1];
            memCount--;
            i--;
        }
    }
}

void lowMemoryCheck(void) {
    if (ramUsed <= 800) return;
    printf("\n[LMK] Memory pressure detected. Looking for background processes...\n");
    for (int i = 0; i < appCount && ramUsed > 700; i++) {
        if (apps[i].state == BACKGROUND) {
            printf("[LMK] Terminating background app: %s (PID %d)\n", apps[i].name, apps[i].pid);
            releaseMemory(apps[i].pid);
            apps[i].state = TERMINATED;
        }
    }
}

void launchApp(void) {
    if (appCount >= MAX_APPS) { printf("Application table full.\n"); return; }
    char name[32];
    int memory, priority;
    printf("Enter application name: "); scanf("%31s", name);
    printf("Memory required (MB): "); scanf("%d", &memory);
    printf("Priority (1=highest, 5=lowest): "); scanf("%d", &priority);
    if (memory <= 0 || memory > RAM_TOTAL || priority < 1 || priority > 5) {
        printf("Invalid values.\n"); return;
    }
    App *a = &apps[appCount];
    a->pid = nextPid++;
    strcpy(a->name, name);
    a->memory = memory;
    a->priority = priority;
    a->burst = 0;
    a->remaining = 0;
    a->state = READY;
    a->camera = a->storage = a->internet = a->location = 0;
    if (!allocateMemory(a->pid, memory)) {
        printf("Not enough RAM. Launch denied.\n");
        a->state = TERMINATED;
        return;
    }
    appCount++;
    printf("Application launched: %s | PID=%d | %d MB allocated.\n", name, a->pid, memory);
    lowMemoryCheck();
}

void changeState(void) {
    int pid, choice;
    printf("Enter PID: "); scanf("%d", &pid);
    int i = findApp(pid);
    if (i < 0) { printf("PID not found.\n"); return; }
    printf("1. Foreground (RUNNING)\n2. Background\n3. Pause\n4. Resume (READY)\nChoice: ");
    scanf("%d", &choice);
    if (choice == 1) apps[i].state = RUNNING;
    else if (choice == 2) apps[i].state = BACKGROUND;
    else if (choice == 3) apps[i].state = PAUSED;
    else if (choice == 4) apps[i].state = READY;
    else { printf("Invalid choice.\n"); return; }
    printf("%s -> %s\n", apps[i].name, stateName(apps[i].state));
}

void killApp(void) {
    int pid;
    printf("Enter PID to terminate: "); scanf("%d", &pid);
    int i = findApp(pid);
    if (i < 0) { printf("PID not found.\n"); return; }
    releaseMemory(pid);
    apps[i].state = TERMINATED;
    printf("Process %d (%s) terminated and memory released.\n", pid, apps[i].name);
}

void scheduler(void) {
    showHeader("CPU Scheduler - Round Robin");
    int quantum;
    printf("Enter time quantum: "); scanf("%d", &quantum);
    if (quantum <= 0) { printf("Invalid quantum.\n"); return; }
    int q[MAX_QUEUE], front = 0, rear = 0;
    for (int i = 0; i < appCount; i++) {
        if (apps[i].state == READY || apps[i].state == RUNNING) {
            printf("Burst time for %s (PID %d): ", apps[i].name, apps[i].pid);
            scanf("%d", &apps[i].burst);
            apps[i].remaining = apps[i].burst;
            if (apps[i].burst > 0) q[rear++] = i;
        }
    }
    printf("\nExecution order:\n");
    int time = 0;
    while (front < rear) {
        int i = q[front++];
        if (apps[i].remaining <= 0) continue;
        apps[i].state = RUNNING;
        int slice = apps[i].remaining < quantum ? apps[i].remaining : quantum;
        printf("t=%-3d -> %-15s PID=%d ran for %d unit(s)\n", time, apps[i].name, apps[i].pid, slice);
        time += slice;
        apps[i].remaining -= slice;
        if (apps[i].remaining > 0) {
            apps[i].state = READY;
            q[rear++] = i;
        } else {
            apps[i].state = BACKGROUND;
        }
    }
    printf("Scheduler finished. Total simulated CPU time = %d.\n", time);
}

void permissionManager(void) {
    int pid, choice, allow;
    printf("Enter PID: "); scanf("%d", &pid);
    int i = findApp(pid);
    if (i < 0) { printf("PID not found.\n"); return; }
    printf("\n1. Camera\n2. Storage\n3. Internet\n4. Location\nChoose permission: ");
    scanf("%d", &choice);
    printf("Allow permission? (1=yes, 0=no): "); scanf("%d", &allow);
    int *p = NULL; const char *name = "";
    if (choice == 1) { p = &apps[i].camera; name = "Camera"; }
    else if (choice == 2) { p = &apps[i].storage; name = "Storage"; }
    else if (choice == 3) { p = &apps[i].internet; name = "Internet"; }
    else if (choice == 4) { p = &apps[i].location; name = "Location"; }
    else { printf("Invalid permission.\n"); return; }
    *p = allow ? 1 : 0;
    printf("[%s] %s permission for %s.\n", allow ? "GRANTED" : "DENIED", name, apps[i].name);
}

void showPermissions(void) {
    showHeader("Application Permissions");
    printf("%-7s %-18s %-8s %-9s %-9s %-9s\n", "PID", "APP", "CAMERA", "STORAGE", "INTERNET", "LOCATION");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < appCount; i++) if (apps[i].state != TERMINATED)
        printf("%-7d %-18s %-8s %-9s %-9s %-9s\n", apps[i].pid, apps[i].name,
            apps[i].camera ? "ALLOW" : "DENY", apps[i].storage ? "ALLOW" : "DENY",
            apps[i].internet ? "ALLOW" : "DENY", apps[i].location ? "ALLOW" : "DENY");
}

void fileManager(void) {
    int choice;
    printf("\n1. Create file\n2. List files\n3. Delete file\nChoice: "); scanf("%d", &choice);
    if (choice == 1) {
        if (fileCount >= MAX_FILES) { printf("File table full.\n"); return; }
        int pid, size; char name[40];
        printf("Owner PID: "); scanf("%d", &pid);
        if (findApp(pid) < 0) { printf("Invalid owner PID.\n"); return; }
        printf("File name: "); scanf("%39s", name);
        printf("Size (KB): "); scanf("%d", &size);
        strcpy(files[fileCount].name, name); files[fileCount].size = size; files[fileCount].ownerPid = pid;
        fileCount++;
        printf("File created in simulated app storage.\n");
    } else if (choice == 2) {
        printf("\n%-20s %-10s %-10s\n", "FILE", "SIZE(KB)", "OWNER PID");
        for (int i = 0; i < fileCount; i++) printf("%-20s %-10d %-10d\n", files[i].name, files[i].size, files[i].ownerPid);
        if (!fileCount) printf("No files.\n");
    } else if (choice == 3) {
        char name[40]; printf("File name to delete: "); scanf("%39s", name);
        for (int i = 0; i < fileCount; i++) if (strcmp(files[i].name, name) == 0) {
            for (int j = i; j < fileCount - 1; j++) files[j] = files[j + 1];
            fileCount--; printf("File deleted.\n"); return;
        }
        printf("File not found.\n");
    } else printf("Invalid choice.\n");
}

void ipc(void) {
    int from, to; char msg[100];
    printf("Sender PID: "); scanf("%d", &from);
    printf("Receiver PID: "); scanf("%d", &to);
    if (findApp(from) < 0 || findApp(to) < 0) { printf("Both PIDs must be active.\n"); return; }
    printf("Message: "); scanf(" %99[^\n]", msg);
    printf("\n[IPC] %d -> %d\nMessage delivered through simulated Binder-style IPC: %s\n", from, to, msg);
}

void lifecycleDemo(void) {
    int pid; printf("Enter PID: "); scanf("%d", &pid);
    int i = findApp(pid);
    if (i < 0) { printf("PID not found.\n"); return; }
    printf("\nActivity lifecycle simulation for %s:\n", apps[i].name);
    printf("ON_CREATE -> ON_START -> ON_RESUME\n");
    printf("Current state: FOREGROUND/RUNNING\n");
    printf("Move to background? (1=yes): "); int x; scanf("%d", &x);
    if (x == 1) { printf("ON_PAUSE -> ON_STOP\n"); apps[i].state = BACKGROUND; }
}

void systemInfo(void) {
    showHeader("System Information");
    printf("OS Model        : Android OS (educational simulation)\n");
    printf("Kernel concept  : Linux-based kernel\n");
    printf("Runtime concept : ART / managed application runtime\n");
    printf("RAM             : %d MB simulated\n", RAM_TOTAL);
    printf("Active apps     : ");
    int c = 0; for (int i = 0; i < appCount; i++) if (apps[i].state != TERMINATED) c++;
    printf("%d\n", c);
    printf("Memory used     : %d MB\n", ramUsed);
    printf("Security model  : sandbox + per-app permissions (simulated)\n");
}

void seedApps(void) {
    strcpy(apps[0].name, "Launcher"); apps[0].pid = nextPid++; apps[0].memory = 180; apps[0].priority = 1; apps[0].state = RUNNING; allocateMemory(apps[0].pid, 180); appCount++;
    strcpy(apps[1].name, "Music"); apps[1].pid = nextPid++; apps[1].memory = 160; apps[1].priority = 3; apps[1].state = BACKGROUND; allocateMemory(apps[1].pid, 160); appCount++;
    strcpy(apps[2].name, "Browser"); apps[2].pid = nextPid++; apps[2].memory = 240; apps[2].priority = 2; apps[2].state = READY; allocateMemory(apps[2].pid, 240); appCount++;
}

int main(void) {
    seedApps();
    int choice;
    while (1) {
        printf("\n\n================ ANDROID OS SIMULATOR ================\n");
        printf("1. Launch Application\n");
        printf("2. View Running Processes\n");
        printf("3. CPU Scheduler (Round Robin)\n");
        printf("4. Memory Manager / Low Memory Killer\n");
        printf("5. Application Lifecycle\n");
        printf("6. Permission Manager\n");
        printf("7. File Manager\n");
        printf("8. IPC Message Demo\n");
        printf("9. Terminate Application\n");
        printf("10. System Information\n");
        printf("11. View Permissions\n");
        printf("0. Exit\n");
        printf("=======================================================\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: launchApp(); break;
            case 2: showApps(); break;
            case 3: scheduler(); break;
            case 4: showMemory(); lowMemoryCheck(); break;
            case 5: lifecycleDemo(); break;
            case 6: permissionManager(); break;
            case 7: fileManager(); break;
            case 8: ipc(); break;
            case 9: killApp(); break;
            case 10: systemInfo(); break;
            case 11: showPermissions(); break;
            case 0: printf("Shutting down simulator...\n"); return 0;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}
