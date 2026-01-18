#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "../include/jobs.h"
#include "../include/colors.h"

static Job jobs[MAX_JOBS];
static int next_job_id = 1;

void init_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].job_id = 0;  // 0 means slot is empty
        jobs[i].pid = 0;
        jobs[i].status = JOB_DONE;
        jobs[i].command[0] = '\0';
    }
}

void cleanup_jobs(void) {
    // Kill all running/stopped jobs
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0 && 
            (jobs[i].status == JOB_RUNNING || jobs[i].status == JOB_STOPPED)) {
            kill(jobs[i].pid, SIGTERM);
        }
    }
}

int add_job(pid_t pid, const char *command) {
    // Find empty slot
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == 0) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            jobs[i].status = JOB_RUNNING;
            strncpy(jobs[i].command, command, MAX_CMD_LEN - 1);
            jobs[i].command[MAX_CMD_LEN - 1] = '\0';
            return jobs[i].job_id;
        }
    }
    return -1;  // Job table full
}

int remove_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            jobs[i].job_id = 0;
            return 0;
        }
    }
    return -1;
}

Job* get_job_by_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

Job* get_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0 && jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return NULL;
}

void update_job_status(int job_id, JobStatus status) {
    Job *job = get_job_by_id(job_id);
    if (job) {
        job->status = status;
    }
}

void print_jobs(void) {
    int found = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0) {
            found = 1;
            const char *status_str;
            const char *color;
            
            switch (jobs[i].status) {
                case JOB_RUNNING:
                    status_str = "Running";
                    color = COLOR_BOLD_GREEN;
                    break;
                case JOB_STOPPED:
                    status_str = "Stopped";
                    color = COLOR_BOLD_YELLOW;
                    break;
                case JOB_DONE:
                    status_str = "Done";
                    color = COLOR_DIM;
                    break;
                default:
                    status_str = "Unknown";
                    color = COLOR_RESET;
            }
            
            printf("[%d]  %s%s%s\t\t%s\n\r", 
                   jobs[i].job_id, 
                   color, 
                   status_str, 
                   COLOR_RESET,
                   jobs[i].command);
        }
    }
    
    if (!found) {
        printf("No active jobs\n\r");
    }
}

void check_jobs(void) {
    int status;
    pid_t pid;
    
    // Check all jobs without blocking (WNOHANG)
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        Job *job = get_job_by_pid(pid);
        
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // Job terminated
                printf("[%d]  Done\t\t%s\n\r", job->job_id, job->command);
                remove_job(job->job_id);
            } else if (WIFSTOPPED(status)) {
                // Job stopped (Ctrl+Z)
                job->status = JOB_STOPPED;
                printf("\n[%d]  Stopped\t\t%s\n\r", job->job_id, job->command);
            } else if (WIFCONTINUED(status)) {
                // Job continued
                job->status = JOB_RUNNING;
            }
        }
    }
}

int foreground_job(int job_id) {
    Job *job = get_job_by_id(job_id);
    
    if (!job) {
        fprintf(stderr, "fg: job %d not found\n\r", job_id);
        return -1;
    }
    
    if (job->status == JOB_DONE) {
        fprintf(stderr, "fg: job %d has already completed\n\r", job_id);
        return -1;
    }
    
    // If stopped, continue it
    if (job->status == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
    }
    
    job->status = JOB_RUNNING;
    printf("%s\n\r", job->command);
    
    // Wait for the job to finish or stop
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    
    if (WIFSTOPPED(status)) {
        job->status = JOB_STOPPED;
        printf("\n[%d]  Stopped\t\t%s\n\r", job->job_id, job->command);
    } else {
        // Job finished
        remove_job(job_id);
    }
    
    return 0;
}

int background_job(int job_id) {
    Job *job = get_job_by_id(job_id);
    
    if (!job) {
        fprintf(stderr, "bg: job %d not found\n\r", job_id);
        return -1;
    }
    
    if (job->status == JOB_DONE) {
        fprintf(stderr, "bg: job %d has already completed\n\r", job_id);
        return -1;
    }
    
    if (job->status == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
        job->status = JOB_RUNNING;
        printf("[%d]  %s &\n\r", job->job_id, job->command);
    } else {
        printf("bg: job %d already running in background\n\r", job_id);
    }
    
    return 0;
}
